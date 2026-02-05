// =======================================
// ESP32 pH Meter
// 2-Point Calibration + ADC Correction
// =======================================

#define PH_PIN 4
#define VREF 3.3
#define ADC_MAX 4095.0
#define ADC_CORRECTION 1.06   // Koreksi ADC (3.19 / 3.30)

// ===== HASIL KALIBRASI NYATA =====
float V7 = 2.6810;    // Tegangan buffer pH 7 (REAL)
float V4 = 3.3150;    // Tegangan buffer pH 4 (REAL)
float slope;          // V per pH

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  // Hitung slope REAL
  slope = (V4 - V7) / (7.0 - 4.0);

  Serial.println("=== ESP32 pH READY (2-POINT + ADC FIX) ===");
  Serial.print("V7 = "); Serial.println(V7, 4);
  Serial.print("V4 = "); Serial.println(V4, 4);
  Serial.print("Slope = "); Serial.print(slope, 5);
  Serial.println(" V/pH");
}

void loop() {
  float voltage = readVoltage();
  float pH = 7.0 - (voltage - V7) / slope;

  Serial.print("Voltage: ");
  Serial.print(voltage, 4);
  Serial.print(" V | pH: ");
  Serial.println(pH, 2);

  delay(1000);
}

float readVoltage() {
  long sum = 0;
  for (int i = 0; i < 20; i++) {
    sum += analogRead(PH_PIN);
    delay(5);
  }

  float adc = sum / 20.0;

  // ADC → Voltage + Koreksi
  float voltage = adc * (VREF / ADC_MAX);
  voltage *= ADC_CORRECTION;

  return voltage;
}
