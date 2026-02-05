#include <Wire.h>
#include "RTClib.h"

RTC_DS3231 rtc;

char daysOfTheWeek[7][12] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu"};

void setup() {
  Serial.begin(115200);

  // Inisialisasi komunikasi I2C
  // Jika pakai ESP32 pin default: SDA = 21, SCL = 22
  if (!rtc.begin()) {
    Serial.println("Gak nemu modul RTC-nya nih. Cek kabel!");
    while (1);
  }

  // JIKA JAMNYA NGGA VALID (Balik ke tahun 2000/200)
  if (rtc.lostPower()) {
    Serial.println("RTC kehilangan daya, mari kita set ulang jamnya!");
    
    // Baris di bawah ini otomatis mengambil waktu dari Laptop saat kamu klik 'Upload'
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  // Opsional: Buka baris di bawah ini SEKALI saja kalau mau paksa set jam manual
  // rtc.adjust(DateTime(2026, 2, 1, 22, 45, 0)); // (Tahun, Bulan, Tgl, Jam, Menit, Detik)
}

void loop() {
    DateTime now = rtc.now();

    // Memperbaiki tampilan tanggal agar rapi (DD/MM/YYYY)
    Serial.print("Tanggal: ");
    Serial.print(now.day());
    Serial.print('/');
    Serial.print(now.month());
    Serial.print('/');
    Serial.print(now.year()); // Ini yang bikin jadi '200' kalau gak di-adjust
    
    Serial.print("  Jam: ");
    if(now.hour() < 10) Serial.print('0'); // Biar ada angka 0 di depan jika < 10
    Serial.print(now.hour());
    Serial.print(':');
    if(now.minute() < 10) Serial.print('0');
    Serial.print(now.minute());
    Serial.print(':');
    if(now.second() < 10) Serial.print('0');
    Serial.println(now.second());

    delay(1000);
}