#include <SPI.h>
#include <MFRC522.h>
#include "HX711.h"
#include <Servo.h>

// ----- Pin Defines -----
#define HX_DOUT 3
#define HX_SCK  2
#define RFID_SS_PIN 10
#define RFID_RST_PIN 8
#define SERVO_PIN 9

// ----- Objects -----
HX711 scale;
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
Servo myServo;

// ----- State Variables -----
bool servoActive = false;
unsigned long servoOnTime = 0;

void setup() {
  Serial.begin(9600);

  // RFID INIT
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("System ready - scan RFID tag...");

  // HX711 INIT
  scale.begin(HX_DOUT, HX_SCK);
  scale.set_scale();
  scale.tare();

  // SERVO INIT
  myServo.attach(SERVO_PIN);
  myServo.write(0);  // OFF position
}

void loop() {
  unsigned long currentMillis = millis();

  // Servo OFF timing
  if (servoActive && (currentMillis - servoOnTime >= 5000)) {
    myServo.write(0);
    servoActive = false;
    Serial.println("{\"event\":\"servo_off\"}");
  }

  // RFID SCAN LOGIC
  if (!servoActive && rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    // Read RFID UID
    String tagUID = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] < 0x10) tagUID += "0";
      tagUID += String(rfid.uid.uidByte[i], HEX);
    }
    tagUID.toUpperCase();

    // Read Load Cell
    float weightVal = 0.0;
    if (scale.is_ready()) {
      weightVal = scale.get_units(3); // average 3 samples
    } else {
      Serial.println("{\"error\":\"hx711_not_ready\"}");
      return;
    }

    // Print JSON-formatted data (RFID + weight)
    Serial.print("{\"rfid\":\"");
    Serial.print(tagUID);
    Serial.print("\",\"weight\":");
    Serial.print(weightVal, 2);
    Serial.println("}");

    // Activate servo for 5 seconds
    myServo.write(90);
    servoOnTime = currentMillis;
    servoActive = true;

    // Stop reading the same card repeatedly
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    delay(1000);
  }
  else if (!servoActive) {
    // Print no cattle message every 5 seconds when no tag detected
    static unsigned long lastNoCattlePrint = 0;
    if (currentMillis - lastNoCattlePrint >= 5000) {
      Serial.println("{\"message\":\"no_cattle_at_gate\"}");
      lastNoCattlePrint = currentMillis;
    }
  }
}
