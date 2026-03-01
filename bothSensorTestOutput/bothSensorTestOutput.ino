#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD I2C pins
#define SDA_LCD 22
#define SCL_LCD 21

// HC-SR04 pins
#define TRIG_PIN 5
#define ECHO_PIN 12

// TF-Mini pins
#define TF_RX 32
#define TF_TX 33

LiquidCrystal_I2C lcd(0x27, 16, 2);
HardwareSerial tfSerial(2);

uint8_t tfBuffer[9];
uint8_t tfIndex = 0;

long tfDistance = -1;
long usDistance = -1;

void setup() {
  Serial.begin(115200);

  // TF-Mini Serial
  tfSerial.begin(115200, SERIAL_8N1, TF_RX, TF_TX);

  // LCD I2C
  Wire.begin(SDA_LCD, SCL_LCD);
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("System Ready");
  delay(1000);
  lcd.clear();

  // Ultrasonic pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

// Read HC-SR04
long readUltrasonic() {
  long duration, distance_cm;

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);
  distance_cm = duration * 0.034 / 2;

  return distance_cm;
}

// Read TF-Mini continuously
void readTFMini() {
  while (tfSerial.available()) {
    uint8_t c = tfSerial.read();

    if (tfIndex == 0 && c != 0x59) continue;   // Wait for first 0x59
    tfBuffer[tfIndex++] = c;

    if (tfIndex == 9) { // Full frame
      if (tfBuffer[0] == 0x59 && tfBuffer[1] == 0x59) {
        tfDistance = tfBuffer[2] + tfBuffer[3] * 256;
      }
      tfIndex = 0; // Reset for next frame
    }
  }
}

void loop() {
  // --- Update TF-Mini ---
  readTFMini();

  // --- Update Ultrasonic ---
  usDistance = readUltrasonic();

  // --- Display TF-Mini ---
  lcd.setCursor(0,0);
  lcd.print("TF-Mini:      ");
  if (tfDistance >= 0) {
    lcd.setCursor(10,0);
    lcd.print(tfDistance);
    lcd.print("cm");
  } else {
    lcd.setCursor(10,0);
    lcd.print("N/A");
  }

  // --- Display Ultrasonic ---
  lcd.setCursor(0,1);
  lcd.print("US:           ");
  lcd.setCursor(10,1);
  lcd.print(usDistance);
  lcd.print("cm");

  // --- Serial Monitor ---
  Serial.print("TF-Mini: ");
  if (tfDistance >= 0) Serial.print(tfDistance); else Serial.print("N/A");
  Serial.print(" cm | Ultrasonic: ");
  Serial.print(usDistance);
  Serial.println(" cm");

  delay(200); // small delay for readability
}