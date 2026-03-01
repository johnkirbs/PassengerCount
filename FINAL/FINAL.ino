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
#define PHONE_NUMBER "+639682237420"  // Replace with your number

const int buttonPin = 18;   // Button pin
int buttonState = 1;        // Current button state
int lastButtonState = 1;    // Previous button state
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50; // 50ms debounce

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

void setup() {
  Serial.begin(115200);

  pinMode(buttonPin, INPUT_PULLUP); // Button with pull-up

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
  simSerial.println("AT+CGPSPWR=1"); // Turn on GPS
  delay(1000);
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

// --- Read GPS coordinates ---
String getGPSLocation() {
  simSerial.println("AT+CGPSINF=0"); // Get location
  delay(1000);
  String gpsData = "";
  while (simSerial.available()) {
    gpsData += char(simSerial.read());
  }
  // gpsData format: +CGPSINF: 0,latitude,N,longitude,E,...
  int latIndex = gpsData.indexOf(",") + 1;
  int latEnd = gpsData.indexOf(",", latIndex);
  int lonIndex = gpsData.indexOf(",", latEnd + 1) + 1;
  int lonEnd = gpsData.indexOf(",", lonIndex);
  if (latIndex > 0 && lonIndex > 0) {
    String lat = gpsData.substring(latIndex, latEnd);
    String lon = gpsData.substring(lonIndex, lonEnd);
    return lat + "," + lon;
  }
  return "N/A";
}

// --- Send SMS ---
void sendSMS(int passengerCount) {
  String location = getGPSLocation();
  simSerial.print("AT+CMGS=\"");
  simSerial.print(PHONE_NUMBER);
  simSerial.println("\"");
  delay(500);
  simSerial.print("Passenger Count: ");
  simSerial.println(passengerCount);
  simSerial.print("Location: ");
  simSerial.print(location);
  simSerial.write(26); // Ctrl+Z to send
  delay(2000); // Give SIM808 time to send
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

  // --- Button read with debounce ---
  int reading = digitalRead(buttonPin);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && buttonState == HIGH) { // Button pressed event
      sendSMS(count);
      Serial.println("Button Pressed! SMS sent.");
    }
    buttonState = reading;
  }

  lastButtonState = reading;

  delay(200); // Loop smooth
}