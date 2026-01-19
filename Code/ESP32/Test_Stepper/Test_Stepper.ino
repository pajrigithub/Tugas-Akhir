/* ===================== PIN CONFIG ===================== */
const int STEP_PIN   = 18;
const int DIR_PIN    = 17;
const int ENABLE_PIN = 4;

/* ===================== SETTING & VAR ===================== */
const int stepDelay = 5000;      // Kecepatan
const int MAX_LANGKAH_FISIK = 55; // Batas gerak motor sebenarnya
int posisiLangkahSekarang = 0;   // Menyimpan posisi dalam satuan LANGKAH (0-50)

void setup() {
  Serial.begin(115200);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);

  digitalWrite(ENABLE_PIN, HIGH); // Standby (OFF)
  
  Serial.println("--- KONTROL VALVE (INPUT 0-100% -> GERAK 0-50 STEP) ---");
  Serial.println("Ketik 0-100 untuk posisi, 'K' untuk kalibrasi titik 0.");
  Serial.println("-------------------------------------------------------");
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input == "K" || input == "k") {
      posisiLangkahSekarang = 0;
      Serial.println(">>>> KALIBRASI: Posisi sekarang dianggap 0% (Tertutup).");
    } 
    else {
      int inputPersen = input.toInt();

      // Batasi input user agar tetap di 0-100
      if (inputPersen < 0) inputPersen = 0;
      if (inputPersen > 100) inputPersen = 100;

      // PEMETAAN (MAPPING): Mengubah 0-100 menjadi 0-50
      // Rumus: (input * MAX_LANGKAH) / 100
      int targetLangkah = (inputPersen * MAX_LANGKAH_FISIK) / 100;

      Serial.print(">>>> Input: "); Serial.print(inputPersen); Serial.print("%");
      Serial.print(" | Target Fisik: "); Serial.print(targetLangkah); Serial.println(" step");

      gerakKeLangkah(targetLangkah);
    }
  }
}

/* ===================== FUNGSI GERAK FISIK ===================== */
void gerakKeLangkah(int targetLangkah) {
  int selisih = targetLangkah - posisiLangkahSekarang;

  if (selisih == 0) {
    Serial.println("Posisi sudah sesuai.");
    return;
  }

  // Tentukan Arah
  digitalWrite(DIR_PIN, (selisih > 0) ? HIGH : LOW);

  // Aktifkan Driver
  digitalWrite(ENABLE_PIN, LOW);
  delay(5); 

  // Jalankan Motor
  int jumlahGerak = abs(selisih);
  for (int i = 0; i < jumlahGerak; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(stepDelay);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(stepDelay);
  }

  // Simpan posisi langkah terakhir
  posisiLangkahSekarang = targetLangkah;

  // Matikan Driver agar dingin
  digitalWrite(ENABLE_PIN, HIGH);
  
  Serial.println("Gerakan Selesai. Driver Standby.");
}