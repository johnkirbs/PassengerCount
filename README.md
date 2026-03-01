# PassengerCount
Counts Passenger in a Bus and send gps and number of passenger inside the Bus.

# People Counter with SMS Notification and GPS

## Project Overview
This project implements a **people counting system** using a **TF-Mini LiDAR sensor** and an **Ultrasonic sensor**. It tracks the number of people entering or exiting a space (e.g., a bus or room) and displays the count on an **I2C LCD**.  

When the **maximum number of passengers (10)** is reached, the system:  
- Shows a warning message on the LCD: `"MAX PASSENGERS! Drop some to go"`  
- Automatically sends an **SMS** with the current passenger count and GPS location to a pre-configured phone number using a **SIM808 module**.  

Additionally, a **manual button** allows sending an SMS anytime with the current passenger count.

---

## Components Required
- **Arduino Board** (ESP32 or compatible)  
- **TF-Mini LiDAR sensor** (for precise distance detection)  
- **HC-SR04 Ultrasonic sensor** (optional secondary detection)  
- **SIM808 GSM/GPS module** (for SMS and GPS)  
- **16x2 I2C LCD**  
- **Push button** (optional, for manual SMS)  
- Jumper wires, breadboard, and power supply  

---

## Wiring

| Component       | Arduino Pin |
|-----------------|------------|
| TF-Mini TX      | 33         |
| TF-Mini RX      | 32         |
| Ultrasonic TRIG | 5          |
| Ultrasonic ECHO | 12         |
| LCD SDA         | 22         |
| LCD SCL         | 21         |
| SIM808 TX       | 17         |
| SIM808 RX       | 4          |
| Button          | 18         |
| LCD VCC/GND     | 5V / GND   |
| TF-Mini VCC/GND | 5V / GND   |
| Ultrasonic VCC/GND | 5V / GND |
| SIM808 VCC/GND  | 5V / GND (or separate 4V-5V supply) |

---

## Installation

1. **Install Arduino IDE**
   - Download from [Arduino Official Website](https://www.arduino.cc/en/software).

2. **Install Libraries**
   - Install **LiquidCrystal_I2C** library:  
     `Sketch -> Include Library -> Manage Libraries -> Search "LiquidCrystal_I2C" -> Install`  

3. **Upload Code**
   - Connect the Arduino/ESP32 board.  
   - Open `PeopleCounter.ino` (your main code).  
   - Configure the **phone number**:  
     ```cpp
     #define PHONE_NUMBER "+639XXXXXXXXX"
     ```
   - Click **Upload**.  

4. **Power the System**
   - Ensure the SIM808 module is properly powered (requires 4-5V, high current).  
   - Ensure TF-Mini, Ultrasonic, and LCD are connected.  

5. **Test the System**
   - Observe the LCD.  
   - Walk in front of the sensors to increment/decrement the count.  
   - When the count reaches **10**, the LCD will display a warning and an **SMS** will be sent automatically.  
   - Press the button to manually send an SMS anytime.  

---

## Features
- Counts passengers entering and exiting using **dual sensors**.  
- Maximum passenger limit enforcement.  
- **Automatic SMS** with GPS when maximum reached.  
- LCD display showing:  
  - TF-Mini distance  
  - Ultrasonic distance  
  - Current passenger count  
- Manual SMS button override.  

---

## Notes
- Ensure your SIM card has SMS and GPS enabled.  
- Use a reliable power source for SIM808, as it can draw up to **2A** during transmission.  
- The threshold distances for the sensors can be adjusted in the code:  
```cpp
const int threshold = 30;      // cm
const int resetDistance = 40;  // cm
