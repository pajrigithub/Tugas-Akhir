// Definisi Pin Motor Peristaltik
const int motorPin1 = 43;
const int motorPin2 = 44;


// Konfigurasi PWM (Sesuai standar ESP32 3.x)
const int frekuensi = 5000;  // 5 kHz
const int resolusi = 8;      // 8-bit (0-255)

void setup() {
  Serial.begin(115200);

  // Perbaikan: Di versi 3.x, ledcSetup & ledcAttachPin DIGANTI menjadi ledcAttach
  ledcAttach(motorPin1, frekuensi, resolusi);
  ledcAttach(motorPin2, frekuensi, resolusi);
}

void loop() {
  // Contoh: Menjalankan motor dengan kecepatan 200 (sekitar 80%)
  ledcWrite(motorPin1, 255);
  ledcWrite(motorPin2, 255);


  Serial.println("Motor ON - Kecepatan 255");
  delay(5000);  // Jalan 5 detik

  // Mematikan motor
  ledcWrite(motorPin1, 0);
  ledcWrite(motorPin2, 0);


  Serial.println("Motor OFF");
  delay(2000);  // Berhenti 2 detik
}