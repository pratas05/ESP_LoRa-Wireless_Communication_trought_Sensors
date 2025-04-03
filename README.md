# Lora Sensor Project

## Overview
This project uses two ESP32 devices with LoRa integrated for wireless communication via ESP-NOW (using macaddress), where a transmitter device sends the joystick values (press button information and a password defined by the owner) and the distance measured by an ultrasonic sensor HC-SR04 to a receiver device. 
The receiver displays the data on an OLED screen and activates a buzzer and LEDs if the distance is below certain thresholds.

## Objectives
- Send joystick and ultrasonic sensor data between ESP32s using ESP-NOW.

- Display the information on an OLED screen.

- Trigger a buzzer and LEDs based on proximity.

- Efficient implementation without the need for traditional Wi-Fi.

## Components Used

## 🔹 ESP32 Transmitter

- Ultrasonic Sensor HC-SR04

- XY Joystick with button

- OLED Display SSD1306

## 🔹 ESP32 Receiver

- OLED Display SSD1306

- Buzzer

- Red LED (Distance < 10cm)

- Blue LED (Distance < 20cm)

## Installation & Usage

## 🔹 1. Install the required libraries in Arduino IDE

- ESP-NOW (already included in the ESP32 Core)

- Adafruit SSD1306 (for the OLED display)

- Adafruit GFX (for OLED graphics)

## 🔹 2. Configure the Transmitter code

- Upload the transmitter code to the first ESP32.

- Replace the receiver's MAC address.

## 🔹 3. Configure the Receiver code

- Upload the receiver code to the second ESP32.

## 🔹 4. Power both ESP32s and watch the data appear on the receiver's OLED display

## How It Works

- The Transmitter measures distance and reads joystick values.

- Sends this information via ESP-NOW to the Receiver.

- The Receiver displays the data on the OLED and triggers buzzer and LEDs if necessary.

## Contributing
Contributions are welcome! Please fork the repository and submit a pull request with your changes.

## License
This project is licensed under the [MIT License](LICENSE).
