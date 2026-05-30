#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include "RTClib.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Preferences.h> 

/* ===================== PIN CONFIGURATION ===================== */
#define I2C_SDA 8
#define I2C_SCL 9

const int relayPematik = 14; 
const int valve = 48;
const int agitator = 47;
const int pompaSampling = 21;

const int pompa = 43;
const int pompaPendingin = 44;
const int pompaPHUP = 5;
const int pompaPHDOWN = 6;

const int suhuT2 = 15;
const int suhuT1 = 7;

const int STEP_PIN   = 38;
const int DIR_PIN    = 42;
const int ENABLE_PIN = 2;

int sck = 36, miso = 37, mosi = 35, cs = 39;

/* ===================== SETTINGS & VARIABLES ===================== */
float ph7Volt = 1.390;    
float ph4Volt = 1.820;    
float offsetT1 = 0.0;
float offsetT2 = 0.0;
float slope;
float lastPHValue = 7.0; 

Preferences preferences;

// --- STATE MACHINE SAMPLING pH ---
enum SamplingState { IDLE, POMPA_MENYALA, JEDA_DIAM, KUMPUL_DATA };
SamplingState statusSampling = IDLE;
unsigned long waktuMulaiSampling = 0;

const unsigned long DURASI_POMPA = 20000;    
const unsigned long DURASI_JEDA = 5000;      
const unsigned long DURASI_KUMPUL = 60000;   

const int MAX_SAMPLES = 60; 
float phSamples[MAX_SAMPLES];
int sampleCount = 0;
unsigned long lastSampleTime = 0;

const int frekuensi = 5000;
const int resolusi = 8; 
const int stepDelay = 800; 
const int MAX_LANGKAH_FISIK = 55;
int posisiLangkahSekarang = 0;

bool isPematikMenyala = false;
unsigned long waktuMulaiPematik = 0;
const unsigned long DURASI_PEMATIK = 10000; 

OneWire oneWire1(suhuT2);
OneWire oneWire2(suhuT1);
DallasTemperature sensors1(&oneWire1);
DallasTemperature sensors2(&oneWire2);
RTC_DS3231 rtc;
Adafruit_ADS1115 ads;

unsigned long logCounter = 1;
SemaphoreHandle_t xSerialMutex;

/* ===================== PROTOTYPES ===================== */
void TaskControl(void *pvParameters);
void TaskSensor(void *pvParameters);
void gerakKeLangkah(int targetLangkah);
float readVoltagePH();
float calculateMode(float arr[], int n);
void getInstantPH();
void saveActuatorState(const char* key, int value);
void restoreMachineState();

void setup() {
  Serial.begin(115200);
  xSerialMutex = xSemaphoreCreateMutex();

  preferences.begin("sysData", false); 
  
  ph4Volt = preferences.getFloat("ph4", 1.820);
  ph7Volt = preferences.getFloat("ph7", 1.390);
  offsetT1 = preferences.getFloat("offT1", 0.0);
  offsetT2 = preferences.getFloat("offT2", 0.0);
  
  slope = (ph4Volt - ph7Volt) / (4.0 - 7.0);

  Wire.begin(I2C_SDA, I2C_SCL);

  if (!rtc.begin()) {
    Serial.println(">> [ERROR] RTC tidak ditemukan!");
  }
  
  if (!ads.begin()) {
    Serial.println(">> [ERROR] ADS1115 tidak ditemukan!");
  } else {
    ads.setGain(GAIN_ONE);
  }
  
  getInstantPH(); 

  SPI.begin(sck, miso, mosi, cs);
  if (!SD.begin(cs)) {
    Serial.println(">> [ERROR] SD Card Gagal!");
  }

  pinMode(relayPematik, OUTPUT);
  pinMode(valve, OUTPUT);
  pinMode(agitator, OUTPUT);
  pinMode(pompaSampling, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  
  digitalWrite(relayPematik, LOW); 
  digitalWrite(ENABLE_PIN, HIGH); 

  ledcAttach(pompa, frekuensi, resolusi);
  ledcAttach(pompaPendingin, frekuensi, resolusi);
  ledcAttach(pompaPHUP, frekuensi, resolusi);
  ledcAttach(pompaPHDOWN, frekuensi, resolusi);

  sensors1.begin();
  sensors2.begin();
  sensors1.setWaitForConversion(false);
  sensors2.setWaitForConversion(false);

  if (xSerialMutex != NULL) {
    xTaskCreatePinnedToCore(TaskControl, "ControlTask", 8192, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(TaskSensor, "SensorTask", 8192, NULL, 1, NULL, 0);
  }

  Serial.println("\n--- SISTEM V6 (Auto-Resume & Time Sync Ready) ---");
}

void loop() {
  vTaskDelete(NULL); 
}

/* ===================== TASK 1: KONTROL SERIAL (Core 1) ===================== */
void TaskControl(void *pvParameters) {
  restoreMachineState();

  for (;;) {
    unsigned long currentMillis = millis();

    if (isPematikMenyala && (currentMillis - waktuMulaiPematik >= DURASI_PEMATIK)) {
      digitalWrite(relayPematik, LOW); 
      isPematikMenyala = false;
    }

    if (statusSampling == POMPA_MENYALA) {
      if (currentMillis - waktuMulaiSampling >= DURASI_POMPA) {
        digitalWrite(pompaSampling, LOW); 
        statusSampling = JEDA_DIAM; 
        waktuMulaiSampling = currentMillis; 
      }
    } 
    else if (statusSampling == JEDA_DIAM) {
      if (currentMillis - waktuMulaiSampling >= DURASI_JEDA) {
        statusSampling = KUMPUL_DATA; 
        waktuMulaiSampling = currentMillis; 
        sampleCount = 0;
      }
    }
    else if (statusSampling == KUMPUL_DATA) {
      if (currentMillis - lastSampleTime >= 1000 && sampleCount < MAX_SAMPLES) {
        float rawV = readVoltagePH();
        float tempPH = 7.0 + ((rawV - ph7Volt) / slope);
        if (tempPH < 0.0) tempPH = 0.0;
        if (tempPH > 14.0) tempPH = 14.0;
        
        phSamples[sampleCount] = tempPH;
        sampleCount++;
        lastSampleTime = currentMillis;
      }
      if (currentMillis - waktuMulaiSampling >= DURASI_KUMPUL) {
        if (sampleCount > 0) lastPHValue = calculateMode(phSamples, sampleCount);
        else getInstantPH(); 
        statusSampling = IDLE; 
      }
    }

    if (Serial.available() > 0) {
      String perintahString = Serial.readStringUntil('\n');
      perintahString.trim(); 

      if (perintahString.length() > 0) {
        if (perintahString.startsWith("TIME:")) {
          int y, m, d, h, mn, s;
          if (sscanf(perintahString.c_str(), "TIME:%d,%d,%d,%d,%d,%d", &y, &m, &d, &h, &mn, &s) == 6) {
            rtc.adjust(DateTime(y, m, d, h, mn, s));
          }
        }
        else if (perintahString.startsWith("CAL:")) {
          String tipe = perintahString.substring(4, 7); 
          float nilai = perintahString.substring(8).toFloat();
          
          if (tipe == "PH4") { ph4Volt = nilai; preferences.putFloat("ph4", nilai); }
          else if (tipe == "PH7") { ph7Volt = nilai; preferences.putFloat("ph7", nilai); }
          else if (tipe == "T1,") { offsetT1 = nilai; preferences.putFloat("offT1", nilai); }
          else if (tipe == "T2,") { offsetT2 = nilai; preferences.putFloat("offT2", nilai); }
          
          slope = (ph4Volt - ph7Volt) / (4.0 - 7.0); 
        }
        else if (perintahString.equalsIgnoreCase("ON")) {
          digitalWrite(relayPematik, HIGH); 
          isPematikMenyala = true;
          waktuMulaiPematik = currentMillis;
          saveActuatorState("modeM", 3); 
          gerakKeLangkah((80 * MAX_LANGKAH_FISIK) / 100);
        }
        else if (perintahString.equalsIgnoreCase("OFF")) {
          saveActuatorState("modeM", 0);
          gerakKeLangkah(0); 
        }
        else {
          char perintah = perintahString.charAt(0);
          
          if (perintah == 'S' || perintah == 's') {
            int nilai = perintahString.substring(1).toInt();
            if (nilai == 1 && statusSampling == IDLE) {
              digitalWrite(pompaSampling, HIGH);
              statusSampling = POMPA_MENYALA;
              waktuMulaiSampling = currentMillis;
            } else if (nilai == 0) {
              digitalWrite(pompaSampling, LOW);
              statusSampling = IDLE;
            }
          }
          else if (perintah == 'K' || perintah == 'k') {
            if (perintahString.length() > 1 && perintahString.charAt(1) == '0') {
              posisiLangkahSekarang = 0; 
            }
          }
          else if (perintah == 'M' || perintah == 'm') {
            int mode = perintahString.substring(1).toInt();
            int targetPersen = -1;
            if (mode == 0) targetPersen = 0;
            else if (mode == 1) targetPersen = 20;
            else if (mode == 2) targetPersen = 25;
            else if (mode == 3) targetPersen = 80;

            if (targetPersen != -1) {
              saveActuatorState("modeM", mode); 
              int targetLangkahBesar = (80 * MAX_LANGKAH_FISIK) / 100;
              int targetLangkahAkhir = (targetPersen * MAX_LANGKAH_FISIK) / 100;

              if (targetPersen != 80 && posisiLangkahSekarang != targetLangkahBesar) {
                 gerakKeLangkah(targetLangkahBesar);
                 vTaskDelay(50 / portTICK_PERIOD_MS); 
              }
              gerakKeLangkah(targetLangkahAkhir);
            }
          } 
          else {
            int nilai = perintahString.substring(1).toInt();
            if (xSemaphoreTake(xSerialMutex, portMAX_DELAY)) {
              switch (perintah) {
                case 'V': case 'v': 
                  digitalWrite(valve, (nilai == 1) ? HIGH : LOW); 
                  saveActuatorState("vlv", nilai); 
                  break;
                case 'A': case 'a': 
                  digitalWrite(agitator, (nilai == 1) ? HIGH : LOW); 
                  saveActuatorState("agt", nilai); 
                  break;
                case 'P': case 'p': 
                  ledcWrite(pompa, constrain(nilai, 0, 255)); 
                  saveActuatorState("pmp", nilai); 
                  break;
                case 'C': case 'c': 
                  ledcWrite(pompaPendingin, constrain(nilai, 0, 255)); 
                  saveActuatorState("pmpC", nilai); 
                  break;
                case 'U': case 'u': ledcWrite(pompaPHUP, constrain(nilai, 0, 255)); break;
                case 'D': case 'd': ledcWrite(pompaPHDOWN, constrain(nilai, 0, 255)); break;
              }
              xSemaphoreGive(xSerialMutex);
            }
          }
        }
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

/* ===================== TASK 2: SENSOR MONITORING & LOGGING (Core 0) ===================== */
void TaskSensor(void *pvParameters) {
  for (;;) {
    sensors1.requestTemperatures();
    sensors2.requestTemperatures();
    
    float s1 = sensors1.getTempCByIndex(0) + offsetT1; 
    float s2 = sensors2.getTempCByIndex(0) + offsetT2; 

    float pH = lastPHValue; 

    int persenKompor = (posisiLangkahSekarang * 100) / MAX_LANGKAH_FISIK;
    String statusKomporStr = (posisiLangkahSekarang > 0) ? "ON" : "OFF"; 
    String statusPematikStr = isPematikMenyala ? "ON" : "OFF";

    String infoSampling = "IDLE";
    if(statusSampling == POMPA_MENYALA) infoSampling = "PUMP_ON";
    if(statusSampling == JEDA_DIAM) infoSampling = "CALMING";
    if(statusSampling == KUMPUL_DATA) infoSampling = "COLLECTING";

    DateTime now = rtc.now();

    char fileName[20];
    sprintf(fileName, "/%04d%02d%02d.csv", now.year(), now.month(), now.day());
    
    if (!SD.exists(fileName)) {
      File file = SD.open(fileName, FILE_WRITE);
      if (file) {
        file.println("No,Time,Set Point,T1,T2,pH,Kompor(%),Kondisi Kompor");
        file.close();
      }
    }

    char dataLine[120];
    sprintf(dataLine, "%lu,%02d:%02d:%02d,90,%.2f,%.2f,%.2f,%d,%s", 
            logCounter++, now.hour(), now.minute(), now.second(), s2, s1, pH, persenKompor, statusKomporStr.c_str());

    File file = SD.open(fileName, FILE_APPEND);
    bool saveSuccess = false;
    if (file) {
      if (file.println(dataLine)) { saveSuccess = true; }
      file.close();
    }

    if (xSemaphoreTake(xSerialMutex, portMAX_DELAY)) {
      Serial.print("[DATA] ");
      Serial.printf("T2:%.2f | T1:%.2f | ", s1, s2);
      Serial.printf("pH:%.2f [%s] | Api:%d%% | Pematik:%s | ", pH, infoSampling.c_str(), persenKompor, statusPematikStr.c_str());
      if (saveSuccess) Serial.println("SD_OK"); 
      else Serial.println("SD_GAGAL!");
      xSemaphoreGive(xSerialMutex);
    }
    
    vTaskDelay(1000 / portTICK_PERIOD_MS); 
  }
}

/* ===================== FUNGSI HELPER NVS & STATE RESUME ===================== */

void saveActuatorState(const char* key, int value) {
  int currentValue = preferences.getInt(key, -1);
  if (currentValue != value) {
    preferences.putInt(key, value);
  }
}

void restoreMachineState() {
  if(xSemaphoreTake(xSerialMutex, portMAX_DELAY)) {
    Serial.println(">> [RESUME] Mengembalikan kondisi aktuator terakhir...");
    xSemaphoreGive(xSerialMutex);
  }

  int lastValve = preferences.getInt("vlv", 0);
  int lastAgt = preferences.getInt("agt", 0);
  int lastPmp = preferences.getInt("pmp", 0);
  int lastPmpC = preferences.getInt("pmpC", 0);
  int lastModeM = preferences.getInt("modeM", 0); 

  digitalWrite(valve, (lastValve == 1) ? HIGH : LOW);
  digitalWrite(agitator, (lastAgt == 1) ? HIGH : LOW);
  ledcWrite(pompa, constrain(lastPmp, 0, 255));
  ledcWrite(pompaPendingin, constrain(lastPmpC, 0, 255));

  int targetPersen = 0;
  if (lastModeM == 1) targetPersen = 20;
  else if (lastModeM == 2) targetPersen = 25;
  else if (lastModeM == 3) targetPersen = 80;

  if (targetPersen > 0) {
    int targetLangkahAkhir = (targetPersen * MAX_LANGKAH_FISIK) / 100;
    gerakKeLangkah((80 * MAX_LANGKAH_FISIK) / 100);
    vTaskDelay(50 / portTICK_PERIOD_MS); 
    gerakKeLangkah(targetLangkahAkhir);
  }
}

/* ===================== FUNGSI HELPER LAINNYA ===================== */
float calculateMode(float arr[], int n) {
  if (n == 0) return 7.0;
  int intCounts[15] = {0}; 
  for (int i = 0; i < n; i++) {
    int intPart = (int)arr[i]; 
    if (intPart >= 0 && intPart <= 14) intCounts[intPart]++;
  }
  int maxIntCount = 0; int mostFrequentInt = 7; 
  for (int i = 0; i <= 14; i++) {
    if (intCounts[i] > maxIntCount) { maxIntCount = intCounts[i]; mostFrequentInt = i; }
  }
  float filteredArr[60]; int filteredCount = 0;
  for (int i = 0; i < n; i++) {
    if ((int)arr[i] == mostFrequentInt) { filteredArr[filteredCount++] = arr[i]; }
  }
  if (filteredCount == 0) return (float)mostFrequentInt;
  float modeValue = filteredArr[0]; int maxClusterCount = 0;
  for (int i = 0; i < filteredCount; i++) {
    int currentCount = 0; float sumCluster = 0.0;
    for (int j = 0; j < filteredCount; j++) {
      if (abs(filteredArr[j] - filteredArr[i]) <= 0.2001) { currentCount++; sumCluster += filteredArr[j]; }
    }
    if (currentCount > maxClusterCount) { maxClusterCount = currentCount; modeValue = sumCluster / currentCount; }
  }
  return modeValue;
}

void getInstantPH() {
  float currentVoltage = readVoltagePH();
  lastPHValue = 7.0 + ((currentVoltage - ph7Volt) / slope);
  if (lastPHValue < 0.0) lastPHValue = 0.0;
  if (lastPHValue > 14.0) lastPHValue = 14.0;
}

float readVoltagePH() {
  int16_t adc0 = ads.readADC_SingleEnded(0); 
  return ads.computeVolts(adc0);             
}

void gerakKeLangkah(int targetLangkah) {
  int selisih = targetLangkah - posisiLangkahSekarang;
  if (selisih == 0) return;
  digitalWrite(DIR_PIN, (selisih > 0) ? LOW : HIGH);
  digitalWrite(ENABLE_PIN, LOW); 
  int jumlahGerak = abs(selisih);
  for (int i = 0; i < jumlahGerak; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(stepDelay);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(stepDelay);
    if (i % 20 == 0) vTaskDelay(1); 
  }
  posisiLangkahSekarang = targetLangkah;
  digitalWrite(ENABLE_PIN, HIGH); 
}