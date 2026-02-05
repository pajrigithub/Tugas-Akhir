// Definisi Pin Motor Peristaltik
const int motorPin1 = 5;
const int motorPin2 = 6;
const int motorPin3 = 10;

// Konfigurasi PWM (Sesuai standar ESP32 3.x)
const int frekuensi = 5000;    // 5 kHz
const int resolusi = 8;        // 8-bit (0-255)

void setup() {
  Serial.begin(115200);

  // Perbaikan: Di versi 3.x, ledcSetup & ledcAttachPin DIGANTI menjadi ledcAttach
  ledcAttach(motorPin1, frekuensi, resolusi);
  ledcAttach(motorPin2, frekuensi, resolusi);
  ledcAttach(motorPin3, frekuensi, resolusi);

  Serial.println("Sistem Motor Peristaltik Siap (Versi 3.3.3)");
}

void loop() {
  // Contoh: Menjalankan motor dengan kecepatan 200 (sekitar 80%)
  ledcWrite(motorPin1, 255);
  ledcWrite(motorPin2, 255);
  ledcWrite(motorPin3, 200);
  
  Serial.println("Motor ON - Kecepatan 200");
  delay(5000); // Jalan 5 detik

  // Mematikan motor
  ledcWrite(motorPin1, 0);
  ledcWrite(motorPin2, 0);
  ledcWrite(motorPin3, 0);
  
  Serial.println("Motor OFF");
  delay(2000); // Berhenti 2 detik
}