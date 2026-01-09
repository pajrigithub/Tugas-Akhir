#include <AccelStepper.h>

/* ===================== PIN CONFIG ===================== */
#define STEP_PIN    16
#define DIR_PIN     15
#define ENABLE_PIN  14

/* ===================== OBJECT ===================== */
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

/* ===================== SETUP ===================== */
void setup() {
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, LOW);   // Enable DRV8825

  stepper.setMaxSpeed(2000);       // step/detik
  stepper.setAcceleration(1000);
}

/* ===================== LOOP ===================== */
void loop() {

  // ====== MAJU 100 STEP ======
  stepper.moveTo(100);             // posisi target +100
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }

  delay(1000);                     // jeda 1 detik

  // ====== MUNDUR 100 STEP ======
  stepper.moveTo(0);               // balik ke posisi awal
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }

  delay(2000);                     // jeda sebelum ulang
}
