// Mendefinisikan pin yang digunakan
const int relayPins[] = {14, 21, 47, 48};
const int jumlahRelay = 4;

void setup() {
  Serial.begin(115200);
  
  // Inisialisasi semua pin sebagai OUTPUT
  for (int i = 0; i < jumlahRelay; i++) {
    pinMode(relayPins[i], OUTPUT);
    // Kondisi awal MATI (LOW pada ESP32 = High Impedance pada ULN2003)
    digitalWrite(relayPins[i], LOW);
  }
  
  Serial.println("Sistem 4 Relay ESP32-S3 Siap.");
}

void loop() {
  // Menyalakan relay satu per satu secara berurutan
  for (int i = 0; i < jumlahRelay; i++) {
    Serial.print("Menyalakan Relay pada Pin: ");
    Serial.println(relayPins[i]);
    
    digitalWrite(relayPins[i], HIGH); // Output ULN2003 jadi ~0.60V (Relay ON)
    delay(1000); // Tunggu 1 detik
    
    digitalWrite(relayPins[i], LOW);  // Relay OFF
    delay(500);  // Jeda antar relay
  }
  
  Serial.println("--- Siklus Selesai ---");
  delay(2000);
}