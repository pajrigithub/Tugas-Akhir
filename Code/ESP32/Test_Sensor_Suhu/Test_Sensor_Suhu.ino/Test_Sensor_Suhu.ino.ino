
#include <OneWire.h>
#include <DallasTemperature.h>

// Definisi Pin
const int pinSensor1 = 7;
const int pinSensor2 = 15;

// Setup instance OneWire untuk masing-masing pin
OneWire oneWire1(pinSensor1);
OneWire oneWire2(pinSensor2);

// Pasang instance ke Dallas Temperature
DallasTemperature sensors1(&oneWire1);
DallasTemperature sensors2(&oneWire2);

void setup() {
  Serial.begin(115200);
  Serial.println("DS18B20 Temperature Sensor Test");

  // Memulai sensor
  sensors1.begin();
  sensors2.begin();
  
  Serial.println("Inisialisasi selesai.");
}

void loop() {
  // Meminta data suhu dari sensor
  Serial.print("Membaca suhu...");
  sensors1.requestTemperatures();
  sensors2.requestTemperatures();
  Serial.println(" Selesai.");

  // Membaca suhu dalam Celsius
  float suhu1 = sensors1.getTempCByIndex(0);
  float suhu2 = sensors2.getTempCByIndex(0);

  // Menampilkan hasil ke Serial Monitor
  Serial.print("Sensor Pin 7  : ");
  if (suhu1 == DEVICE_DISCONNECTED_C) {
    Serial.println("Error: Sensor tidak terdeteksi!");
  } else {
    Serial.print(suhu1);
    Serial.println(" °C");
  }

  Serial.print("Sensor Pin 15 : ");
  if (suhu2 == DEVICE_DISCONNECTED_C) {
    Serial.println("Error: Sensor tidak terdeteksi!");
  } else {
    Serial.print(suhu2);
    Serial.println(" °C");
  }

  Serial.println("------------------------------------");
  delay(2000); // Tunggu 2 detik sebelum pembacaan berikutnya
}