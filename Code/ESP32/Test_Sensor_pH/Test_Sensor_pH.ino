#include <SimpleKalmanFilter.h>

// =======================================
// ESP32 pH Meter + Simple Kalman Filter
// 2-Point Calibration + ADC Correction
// =======================================

#define PH_PIN 4
#define VREF 3.3
#define ADC_MAX 4095.0
#define ADC_CORRECTION 1.06   // Koreksi ADC

// ===== HASIL KALIBRASI NYATA =====
float V7 = 2.7800;    
float V4 = 3.4500;    
float slope;          

SimpleKalmanFilter kalmanFilter(0.05, 0.05, 0.01);

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  // Hitung slope REAL
  slope = (V4 - V7) / (7.0 - 4.0);

  Serial.println("=== ESP32 pH READY (KALMAN FILTER ACTIVE) ===");
}

void loop() {
  // 1. Ambil tegangan mentah (masih dengan multisampling 20x)
  float rawVoltage = readVoltage();

  // 2. Terapkan Kalman Filter pada nilai tegangan
  float filteredVoltage = kalmanFilter.updateEstimate(rawVoltage);

  // 3. Hitung pH berdasarkan tegangan yang sudah difilter
  float pH = 7.0 - (filteredVoltage - V7) / slope;

  // Output ke Serial Monitor / Plotter
  Serial.print("Raw_V:");
  Serial.print(rawVoltage, 4);
  Serial.print(",");
  Serial.print("Filtered_V:");
  Serial.print(filteredVoltage, 4);
  Serial.print(",");
  Serial.print("pH:");
  Serial.println(pH, 2);

  delay(500); // Delay bisa dipercepat karena Kalman sudah membantu menstabilkan
}

float readVoltage() {
  long sum = 0;
  for (int i = 0; i < 20; i++) {
    sum += analogRead(PH_PIN);
    delay(2); // Dipercepat sedikit agar loop filter lebih responsif
  }

  float adc = sum / 20.0;

  // ADC → Voltage + Koreksi
  float voltage = adc * (VREF / ADC_MAX);
  voltage *= ADC_CORRECTION;

  return voltage;
}