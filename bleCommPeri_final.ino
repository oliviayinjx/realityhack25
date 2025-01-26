#include <ArduinoBLE.h>
#include <Arduino_LSM6DS3.h>  // IMU library for Nano 33 IoT

// Define BLE service and characteristic UUIDs
BLEService sensorService("19B20000-E8F2-537E-4F6C-D104768A1214");
BLEStringCharacteristic dataCharacteristic("19B20001-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify, 50);

const int analogPin0 = A0;  // Pin to read voltage
const int analogPin1 = A1;  // Pin to read voltage
const int analogPin2 = A2;  // Pin to read voltage
const float VL1_ref = 3.3;   // Voltage supply (3.3V for Nano 33 IoT)
const float RL1_ref = 1000;  // Reference resistor value in ohms
const float VL2_ref = 3.3;   // Voltage supply (3.3V for Nano 33 IoT)
const float RL2_ref = 1000;  // Reference resistor value in ohms
const float VL3_ref = 3.3;   // Voltage supply (3.3V for Nano 33 IoT)
const float RL3_ref = 1000;  // Reference resistor value in ohms

bool accoFall = false;
bool allPressed = false;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (!BLE.begin()) {
    Serial.println("Failed to start BLE!");
    while (1);
  }

  // Initialize the IMU
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
  Serial.println("IMU initialized.");

  // Configure analog pins as input
  pinMode(analogPin0, INPUT);
  pinMode(analogPin1, INPUT);
  pinMode(analogPin2, INPUT);

  // Set up BLE
  BLE.setLocalName("SensorPeripheral");
  BLE.setAdvertisedService(sensorService);
  sensorService.addCharacteristic(dataCharacteristic);
  BLE.addService(sensorService);
  BLE.advertise();

  Serial.println("BLE Peripheral: Advertising sensor data...");
}

void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    Serial.println("Central connected.");

    while (central.connected()) {
      // Read Y-axis data from IMU
      float x, y, z;
      if (IMU.accelerationAvailable()) {
        IMU.readAcceleration(x, y, z);
      }

      // Read voltage from analog pins
      int rawValue0 = analogRead(analogPin0);
      int rawValue1 = analogRead(analogPin1);
      int rawValue2 = analogRead(analogPin2);
      float V_A0 = (rawValue0 / 1023.0) * VL1_ref;
      float V_A1 = (rawValue1 / 1023.0) * VL2_ref;
      float V_A2 = (rawValue2 / 1023.0) * VL3_ref;

      // Check conditions for falling
      accoFall = (y > 0.4 || y < -0.4);

      // Check conditions for allPressed
      allPressed = (V_A0 > 0.5 || V_A1 > 0.5 || V_A2 > 0.5);

      // Format data as a string
      char buffer[50];
      snprintf(buffer, sizeof(buffer), "Fall: %d, Pressed: %d",accoFall, allPressed);

      // Send data via BLE
      dataCharacteristic.writeValue(buffer);

      // Debug prints
      Serial.print("Sent: ");
      Serial.println(buffer);

      // Additional debug for booleans
      if (accoFall) {
        Serial.println("Falling detected!");
      }
      if (allPressed) {
        Serial.println("All pressed!");
      }

      delay(500);  // Send data every 0.5 seconds
    }

    Serial.println("Central disconnected.");
  }
}
