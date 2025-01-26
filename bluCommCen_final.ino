#include <ArduinoBLE.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>  // Required for 16 MHz Adafruit Trinket
#endif

int pin = 12;                            // Pin connected to NeoPixels
int numPixels = 16;                      // Number of NeoPixels
int pixelFormat = NEO_GRB + NEO_KHZ800;  // NeoPixel color format

Adafruit_NeoPixel* pixels;

#define BREATH_DELAY 20    // Delay between brightness updates (in ms)
#define MAX_BRIGHTNESS 50  // Maximum brightness (0-255)

// Function prototypes
void fall();
void unbalanced();

void setup() {
  Serial.begin(9600);
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif

  pixels = new Adafruit_NeoPixel(numPixels, pin, pixelFormat);
  pixels->begin();  // Initialize NeoPixel strip
  pixels->clear();  // Clear all pixels

  while (!Serial)
    ;

  if (!BLE.begin()) {
    Serial.println("Failed to start BLE!");
    while (1)
      ;
  }

  Serial.println("BLE Central: Scanning for 'SensorPeripheral'...");
  BLE.scan();  // Start scanning
}

void breathWholeCircle() {
  static float breathStep = 0.0;    // Step variable for sine wave
  const float breathSpeed = 0.002;  // Reduced speed for slower breathing

  // Calculate brightness factor using a sine wave
  float brightnessFactor = (sin(breathStep) + 1.0) * 0.2;  // Range: 0.0 to 1.0

  // Scale RGB values using MAX_BRIGHTNESS for blue light
  int red = (int)(0 * brightnessFactor * (MAX_BRIGHTNESS / 255.0));     // No red
  int green = (int)(0 * brightnessFactor * (MAX_BRIGHTNESS / 255.0));   // No green
  int blue = (int)(255 * brightnessFactor * (MAX_BRIGHTNESS / 255.0));  // Max blue

  // Clear all pixels first
  pixels->clear();

  // Set all pixels to scaled vibrant orange color
  for (int i = 0; i < numPixels; i++) {
    pixels->setPixelColor(i, pixels->Color(red, green, blue));
  }
  pixels->show();  // Update the NeoPixel strip

  // Update breath step
  breathStep += breathSpeed;
  if (breathStep >= TWO_PI) {
    breathStep = 0.0;  // Reset for continuous breathing
  }
}

// Function to create breathing light for the right half of the NeoPixel ring
void breathRightHalf() {
  static float breathStepRight = 0.0;  // Independent step for right half
  const float breathSpeed = 0.002;     // Reduced speed for slower breathing
  const int shift = 2;                 // Clockwise shift by 2 pixels

  // Calculate brightness factor using a sine wave
  float brightnessFactor = (sin(breathStepRight) + 1.0) * 0.5;  // Range: 0.0 to 1.0

  // Scale RGB values using MAX_BRIGHTNESS for blue light
  int red = (int)(0 * brightnessFactor * (MAX_BRIGHTNESS / 255.0));     // No red
  int green = (int)(0 * brightnessFactor * (MAX_BRIGHTNESS / 255.0));   // No green
  int blue = (int)(255 * brightnessFactor * (MAX_BRIGHTNESS / 255.0));  // Max blue

  // Clear all pixels first
  pixels->clear();

  // Set right half pixels (second quarter and third quarter of the ring)
  for (int i = numPixels / 4; i < 3 * numPixels / 4; i++) {
    int shiftedIndex = (i + shift) % numPixels;  // Shift index clockwise
    pixels->setPixelColor(shiftedIndex, pixels->Color(red, green, blue));
  }
  pixels->show();  // Update the NeoPixel strip

  // Update breath step
  breathStepRight += breathSpeed;
  if (breathStepRight >= TWO_PI) {
    breathStepRight = 0.0;  // Reset for continuous breathing
  }
}

void loop() {
  BLEDevice peripheral = BLE.available();

  if (peripheral && peripheral.localName() == "SensorPeripheral") {
    Serial.println("Found 'SensorPeripheral', connecting...");
    BLE.stopScan();

    if (peripheral.connect()) {
      Serial.println("Connected to 'SensorPeripheral'!");

      if (peripheral.discoverAttributes()) {
        Serial.println("Attributes discovered!");

        BLECharacteristic dataCharacteristic = peripheral.characteristic("19B20001-E8F2-537E-4F6C-D104768A1214");

        if (dataCharacteristic) {
          Serial.println("Subscribed to notifications!");
          dataCharacteristic.subscribe();  // Enable notifications

          while (peripheral.connected()) {
            if (dataCharacteristic.valueUpdated()) {
              const uint8_t* data = dataCharacteristic.value();
              size_t length = dataCharacteristic.valueLength();

              // Convert received data to string
              char buffer[length + 1];
              memcpy(buffer, data, length);
              buffer[length] = '\0';

              Serial.print("Received: ");
              Serial.println(buffer);

              // Parse the received data
              int fallValue = 0, pressedValue = 0;
              sscanf(buffer, "Fall: %d, Pressed: %d", &fallValue, &pressedValue);

              // Call the appropriate function based on the values
              if (fallValue == 1) {
                fall();
              } else {
                // Clear all pixels first
                pixels->clear();
              }
              if (pressedValue == 1) {
                unbalanced();
              }
            }
          }
        } else {
          Serial.println("Characteristic not found!");
        }
      }
    }

    Serial.println("Disconnected from 'SensorPeripheral'. Resuming scan...");
    BLE.scan();
  }
}

void fall() {
  //Serial.println("Fall detected! Executing fall()...");
  breathWholeCircle();
}

void unbalanced() {

}
