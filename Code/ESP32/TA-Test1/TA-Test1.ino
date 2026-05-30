#include <OneWire.h>
#include <DallasTemperature.h>
#include <SimpleKalmanFilter.h>

/* ===================== PIN CONFIGURATION ===================== */
// Relay & Digital
const int valve = 48;
const int agitator = 47;
const int pompaSampling = 21;

// PWM Output
const int pompa = 43;
const int pompaPendingin = 44;
const int pompaPHUP = 5;
const int pompaPHDOWN = 6;

// Sensor Suhu
const int suhuT2 = 15;
const int suhuT1 = 7;

// Sensor pH
#define PH_PIN 4
#define VREF 3.3
#define ADC_MAX 4095.0
#define ADC_CORRECTION 1.06 

// Stepper Motor
const int STEP_PIN   = 38;
const int DIR_PIN    = 42;
const int ENABLE_PIN = 2;

/* ===================== SETTINGS & VARIABLES ===================== */
// pH Calibration (Hasil Nyata)
float V7 = 2.7800;    
float V4 = 3.4500;    
float slope;

// Kalman Filter Settings
SimpleKalmanFilter kalmanFilter(0.05, 0.05, 0.01);

const int frekuensi = 5000;
const int resolusi = 8; 
const int stepDelay = 2500;
const int MAX_LANGKAH_FISIK = 55;
int posisiLangkahSekarang = 0;

// Sensor Objects
OneWire oneWire1(suhuT2);
OneWire oneWire2(suhuT1);
DallasTemperature sensors1(&oneWire1);
DallasTemperature sensors2(&oneWire2);

// Mutex untuk Serial Monitor
SemaphoreHandle_t xSerialMutex;

/* ===================== PROTOTYPES ===================== */
void TaskControl(void *pvParameters);
void TaskSensor(void *pvParameters);
void gerakKeLangkah(int targetLangkah);
float readVoltagePH();

void setup() {
  Serial.begin(115200);
  xSerialMutex = xSemaphoreCreateMutex();

  // Inisialisasi ADC pH
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  slope = (V4 - V7) / (7.0 - 4.0);

  // Inisialisasi Output Digital
  pinMode(valve, OUTPUT);
  pinMode(agitator, OUTPUT);
  pinMode(pompaSampling, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  
  digitalWrite(valve, LOW); 
  digitalWrite(agitator, LOW);
  digitalWrite(pompaSampling, LOW);
  digitalWrite(ENABLE_PIN, HIGH); // Standby

  // Inisialisasi PWM
  ledcAttach(pompa, frekuensi, resolusi);
  ledcAttach(pompaPendingin, frekuensi, resolusi);
  ledcAttach(pompaPHUP, frekuensi, resolusi);
  ledcAttach(pompaPHDOWN, frekuensi, resolusi);

  // Inisialisasi Sensor Suhu
  sensors1.begin();
  sensors2.begin();

  if (xSerialMutex != NULL) {
    // Task Control di Core 1 (Prioritas Tinggi untuk Respon Serial)
    xTaskCreatePinnedToCore(TaskControl, "ControlTask", 4096, NULL, 2, NULL, 1);
    // Task Sensor di Core 0 (Sampling Data)
    xTaskCreatePinnedToCore(TaskSensor, "SensorTask", 4096, NULL, 1, NULL, 0);
  }

  Serial.println("\n--- SISTEM TERINTEGRASI V4.1 (pH Kalman Filter) ---");
  Serial.println("V[0/1]: Valve | A[0/1]: Agitator | S[0/1]: Sampling");
  Serial.println("P[0-255]: Pompa Utama | C[0-255]: P. Pendingin");
  Serial.println("U[0-255]: PH UP | D[0-255]: PH DOWN");
  Serial.println("M[0-100]: Stepper % | MK: Kalibrasi Stepper");
  Serial.println("---------------------------------------------------");
}

void loop() {
  vTaskDelete(NULL); 
}

/* ===================== TASK 1: KONTROL SERIAL (Core 1) ===================== */
void TaskControl(void *pvParameters) {
  for (;;) {
    if (Serial.available() > 0) {
      char perintah = Serial.read();
      
      if (perintah == 'M' || perintah == 'm') {
        char subPerintah = Serial.peek();
        if (subPerintah == 'K' || subPerintah == 'k') {
          Serial.read(); 
          posisiLangkahSekarang = 0;
          if (xSemaphoreTake(xSerialMutex, portMAX_DELAY)) {
            Serial.println(">>>> [STEPPER] Kalibrasi: Posisi 0% (Tertutup).");
            xSemaphoreGive(xSerialMutex);
          }
        } else {
          int nilai = Serial.parseInt();
          int inputPersen = constrain(nilai, 0, 100);
          int targetLangkah = (inputPersen * MAX_LANGKAH_FISIK) / 100;
          
          if (xSemaphoreTake(xSerialMutex, portMAX_DELAY)) {
            Serial.printf(">>>> [STEPPER] Target: %d%% (%d step)\n", inputPersen, targetLangkah);
            xSemaphoreGive(xSerialMutex);
          }
          gerakKeLangkah(targetLangkah);
        }
      } 
      else {
        int nilai = Serial.parseInt();
        if (xSemaphoreTake(xSerialMutex, portMAX_DELAY)) {
          switch (perintah) {
            case 'V': case 'v':
              digitalWrite(valve, (nilai == 1) ? HIGH : LOW);
              Serial.printf(">> [CTRL] Valve: %s\n", (nilai == 1 ? "ON" : "OFF"));
              break;
            case 'A': case 'a':
              digitalWrite(agitator, (nilai == 1) ? HIGH : LOW);
              Serial.printf(">> [CTRL] Agitator: %s\n", (nilai == 1 ? "ON" : "OFF"));
              break;
            case 'S': case 's':
              digitalWrite(pompaSampling, (nilai == 1) ? HIGH : LOW);
              Serial.printf(">> [CTRL] Pompa Sampling: %s\n", (nilai == 1 ? "ON" : "OFF"));
              break;
            case 'P': case 'p':
              ledcWrite(pompa, constrain(nilai, 0, 255));
              Serial.printf(">> [CTRL] Pompa Utama: %d\n", nilai);
              break;
            case 'C': case 'c':
              ledcWrite(pompaPendingin, constrain(nilai, 0, 255));
              Serial.printf(">> [CTRL] Pompa Pendingin: %d\n", nilai);
              break;
            case 'U': case 'u':
              ledcWrite(pompaPHUP, constrain(nilai, 0, 255));
              Serial.printf(">> [CTRL] PH UP: %d\n", nilai);
              break;
            case 'D': case 'd':
              ledcWrite(pompaPHDOWN, constrain(nilai, 0, 255));
              Serial.printf(">> [CTRL] PH DOWN: %d\n", nilai);
              break;
          }
          xSemaphoreGive(xSerialMutex);
        }
      }
      while(Serial.available() > 0) Serial.read(); 
    }
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

/* ===================== TASK 2: SENSOR MONITORING (Core 0) ===================== */
void TaskSensor(void *pvParameters) {
  for (;;) {
    // 1. Baca Suhu
    sensors1.requestTemperatures();
    sensors2.requestTemperatures();
    float s1 = sensors1.getTempCByIndex(0);
    float s2 = sensors2.getTempCByIndex(0);

    // 2. Baca pH dengan Kalman Filter
    float rawV = readVoltagePH();
    float filteredV = kalmanFilter.updateEstimate(rawV);
    float pH = 7.0 - (filteredV - V7) / slope;

    // 3. Output Gabungan
    if (xSemaphoreTake(xSerialMutex, portMAX_DELAY)) {
      Serial.print("[DATA] ");
      Serial.printf("T2:%.2f | T1:%.2f | ", s1, s2);
      Serial.printf("pH:%.2f (V:%.4f)\n", pH, filteredV);
      xSemaphoreGive(xSerialMutex);
    }
    
    vTaskDelay(2000 / portTICK_PERIOD_MS); 
  }
}

/* ===================== FUNGSI HELPER pH ===================== */
float readVoltagePH() {
  long sum = 0;
  for (int i = 0; i < 20; i++) {
    sum += analogRead(PH_PIN);
    delay(2); 
  }
  float adc = sum / 20.0;
  float voltage = adc * (VREF / ADC_MAX) * ADC_CORRECTION;
  return voltage;
}

/* ===================== FUNGSI GERAK STEPPER ===================== */
void gerakKeLangkah(int targetLangkah) {
  int selisih = targetLangkah - posisiLangkahSekarang;
  if (selisih == 0) return;

  digitalWrite(DIR_PIN, (selisih > 0) ? LOW : HIGH);
  digitalWrite(ENABLE_PIN, LOW); 
  delay(5); 

  int jumlahGerak = abs(selisih);
  for (int i = 0; i < jumlahGerak; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(stepDelay);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(stepDelay);
    if (i % 10 == 0) vTaskDelay(1); 
  }

  posisiLangkahSekarang = targetLangkah;
  digitalWrite(ENABLE_PIN, HIGH); 
  
  if (xSemaphoreTake(xSerialMutex, portMAX_DELAY)) {
    Serial.println(">> [STEPPER] Gerakan Selesai.");
    xSemaphoreGive(xSerialMutex);
  }
}