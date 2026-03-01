#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD I2C pins
#define SDA_LCD 22
#define SCL_LCD 21

// Ultrasonic pins
#define TRIG_PIN 5
#define ECHO_PIN 12

// TF-Mini pins
#define TF_RX 32
#define TF_TX 33

// SIM808 pins
#define SIM_TX 17
#define SIM_RX 4
#define PHONE_NUMBER "+639604199481"  // Replace with your number

LiquidCrystal_I2C lcd(0x27, 16, 2);
HardwareSerial tfSerial(2);   // TF-Mini
HardwareSerial simSerial(1);  // SIM808

uint8_t tfBuffer[9];
uint8_t tfIndex = 0;

long tfDistance = -1;
long usDistance = -1;

int count = 0;           // People count
int firstSensor = 0;     // 0 = none, 1 = TF-Mini, 2 = Ultrasonic

bool sensor1Active = false;
bool sensor2Active = false;

// Hysteresis thresholds
const int threshold = 30;     // cm
const int resetDistance = 40; // cm to reset active state

unsigned long lastSmsTime = 0;  // For SMS interval

void setup() {
  Serial.begin(115200);

  // TF-Mini Serial
  tfSerial.begin(115200, SERIAL_8N1, TF_RX, TF_TX);

  // SIM808 Serial
  simSerial.begin(9600, SERIAL_8N1, SIM_RX, SIM_TX);

  // LCD I2C
  Wire.begin(SDA_LCD, SCL_LCD);
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("People Counter");
  delay(1000);
  lcd.clear();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Initialize SIM808
  initializeSIM808();
}

// --- Initialize SIM808 ---
void initializeSIM808() {
  simSerial.println("AT");           // Check communication
  delay(500);
  simSerial.println("AT+CMGF=1");    // SMS text mode
  delay(500);
}

// --- Read TF-Mini distance ---
void readTFMini() {
  while (tfSerial.available()) {
    uint8_t c = tfSerial.read();
    if (tfIndex == 0 && c != 0x59) continue;
    tfBuffer[tfIndex++] = c;
    if (tfIndex == 9) { // full frame
      if (tfBuffer[0] == 0x59 && tfBuffer[1] == 0x59) {
        tfDistance = tfBuffer[2] + tfBuffer[3] * 256;
      }
      tfIndex = 0;
    }
  }
}

// --- Read Ultrasonic distance ---
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

// --- Update counting logic ---
void updateCount() {
  sensor1Active = (tfDistance > 0 && tfDistance < threshold);
  if (tfDistance > resetDistance) sensor1Active = false;

  sensor2Active = (usDistance > 0 && usDistance < threshold);
  if (usDistance > resetDistance) sensor2Active = false;

  if (firstSensor == 0) {
    if (sensor1Active) firstSensor = 1;
    else if (sensor2Active) firstSensor = 2;
  } else if (firstSensor == 1 && sensor2Active) {
    count++;        // sensor1 → sensor2 = in
    firstSensor = 0;
  } else if (firstSensor == 2 && sensor1Active) {
    count--;        // sensor2 → sensor1 = out
    firstSensor = 0;
  }

  if (!sensor1Active && !sensor2Active) firstSensor = 0;
}

// --- Send SMS ---
void sendSMS(int passengerCount) {
  simSerial.print("AT+CMGS=\"");
  simSerial.print(PHONE_NUMBER);
  simSerial.println("\"");
  delay(500);
  simSerial.print("Passenger Count: ");
  simSerial.print(passengerCount);
  simSerial.write(26); // Ctrl+Z to send
  delay(1000); // Give SIM808 time to send
}

void loop() {
  readTFMini();
  usDistance = readUltrasonic();

  updateCount();

  // --- Display on LCD ---
  lcd.setCursor(0,0);
  lcd.print("TF:");
  if (tfDistance > 0) lcd.print(tfDistance); else lcd.print("N/A");
  lcd.print("cm ");

  lcd.setCursor(0,1);
  lcd.print("US:");
  if (usDistance > 0) lcd.print(usDistance); else lcd.print("N/A");
  lcd.print("cm Count:");
  lcd.print(count);

  // --- Serial Monitor ---
  Serial.print("TF-Mini: "); Serial.print(tfDistance); Serial.print(" cm | ");
  Serial.print("Ultrasonic: "); Serial.print(usDistance); Serial.print(" cm | ");
  Serial.print("Count: "); Serial.println(count);

  // --- Send SMS every 20 seconds ---
  if (millis() - lastSmsTime >= 20000) {  // 20,000 ms = 20 seconds
    sendSMS(count);
    lastSmsTime = millis();
  }

  delay(200); // small delay for smooth updates
}