#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Stepper.h>

#include "Hi_Hat_data.h"
#include "Snare_Drum_data.h"
#include "Bass_Drum_data.h"
#include "Crash_Cymbal_data.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

// Declaration for two SSD1306 displays connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Global flag to ensure sound is played once per gesture + drum beat combination
bool soundPlayed = false;

// BLE UUIDs
#define SERVICE_UUID        "89d32ebb-e935-47d3-9877-817c75c12572"
#define CHARACTERISTIC_UUID "10c5f81b-952e-4e3d-84a7-ab9e8ec83683"

// Stepper motor setup
const int stepsPerRevolution = 400; // Change this per your motor's specification
// Example GPIO pins; replace D6, D3, D1, D2 with actual GPIO numbers used
Stepper myStepper(stepsPerRevolution, D6, D3, D1, D2);

// Sound Playback variables
const int csPin = 44; // Chip Select pin for MCP4921 DAC
const int sckPin = 7;  // SPI Clock pin
const int mosiPin = 9; // SPI MOSI (Master Out Slave In) pin
const int misoPin = 8; // Or set to -1 if you prefer to signify it's not used
const int potPin = A1;  // Analog pin for reading the potentiometer
SPIClass *hspi = new SPIClass(HSPI);

float volumeMultiplier = 0.25;

// Function prototypes
void updateDisplay1WithGesture(int gesture);
void updateDisplay2WithAcclZ(float AcclZ);
void sendValueToDAC(uint16_t value);
void playSound(const String& soundName);

struct DrumBeatData {
  int gesture;
  float AcclZ;
};

BLEUUID serviceUUID(SERVICE_UUID);
BLEUUID charUUID(CHARACTERISTIC_UUID);
boolean doConnect = false;
boolean connected = false;
boolean doScan = false;
BLERemoteCharacteristic* pRemoteCharacteristic;
BLEAdvertisedDevice* myDevice;


String currentGestureText = "";
boolean drumBeatActive = false;

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) override {
    connected = true;
    String text = "READY";
    display2.clearDisplay();
    display2.setTextSize(2); // Choose an appropriate text size

    // Dynamically calculate the width of the text at the current font size
    int16_t textWidth = 6 * text.length() * 2; // 6 is the approx width of a character at textSize=1, *2 for textSize=2
    int16_t x = (SCREEN_WIDTH - textWidth) / 2;
    int16_t y = (SCREEN_HEIGHT / 2) - 8; // Half the height of the display minus half the height of text
    
    display2.setCursor(x,y);
    display2.print(text);
    display2.display();
  }

  void onDisconnect(BLEClient* pclient) override {
    connected = false;
    // Optionally, you can also update the display upon disconnection
    display2.clearDisplay();
    display2.setTextSize(1); // Choose an appropriate text size
    display2.setCursor(0,0); // Start at top-left corner
    display2.print("Disconnected");
    display2.display();
  }
};


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      Serial.println("Found our device!");
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = false;
    }
  }
};

void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    if (length == sizeof(DrumBeatData)) {
        DrumBeatData receivedData;
        memcpy(&receivedData, pData, sizeof(DrumBeatData));
        updateDisplay1WithGesture(receivedData.gesture); // Update display1 with gesture info
        updateDisplay2WithAcclZ(receivedData.AcclZ); // Update display2 based on AcclZ
    }
}

bool connectToServer() {
    Serial.println("Connecting to server...");
    BLEClient* pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());

    if (pClient->connect(myDevice)) {
        Serial.println("Connected to server");
        BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
        if (pRemoteService == nullptr) {
          Serial.println("Failed to find our service UUID");
          return false;
        }
        pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
        if (pRemoteCharacteristic == nullptr) {
          Serial.println("Failed to find our characteristic UUID");
          return false;
        }
        if (pRemoteCharacteristic->canNotify())
          pRemoteCharacteristic->registerForNotify(notifyCallback);

        return true;
    } else {
        Serial.println("Failed to connect");
        return false;
    }
}

// Global variables to manage the animation state
bool displayDrumBeat = false; // Global variable to track display state for "DrumBeat"
unsigned long previousMillis = 0; // Stores the last time the display was updated
const long interval = 500; // Interval at which to update the animation (milliseconds)
bool isDisplayOn = false; // Tracks the display state for toggling text
int animationFrame = 0; // Tracks the current frame of the animatione

void updateDisplay1WithGesture(int gesture) {
    String gestureText;
    switch (gesture) {
        case 1: gestureText = "HI HAT"; break;
        case 2: gestureText = "SNARE DRUM"; break;
        case 3: gestureText = "BASS DRUM"; break;
        case 4: gestureText = "CRASH CYMBAL"; break;
        default: gestureText = "INTERVAL";
    }

    display1.clearDisplay();
    display1.setTextSize(2); // Assuming this makes each character approximately 12 pixels wide

    // Dynamically calculate the width of the text at the current font size
    int16_t textWidth = 6 * gestureText.length() * 2; // 6 is the approx width of a character at textSize=1, *2 for textSize=2
    int16_t x = (SCREEN_WIDTH - textWidth) / 2;
    int16_t y = (SCREEN_HEIGHT / 2) - 8; // Half the height of the display minus half the height of text

    display1.setCursor(x, y);
    display1.println(gestureText);
    currentGestureText = gestureText;
    display1.display();
}

void updateDisplay2WithAcclZ(float AcclZ) {
    unsigned long currentMillis = millis();

    // Check if the display should blink
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        isDisplayOn = !isDisplayOn;
    }

    if (AcclZ < 0) {
        if (!displayDrumBeat) { // If entering the "DRUM BEAT" phase
            myStepper.step(stepsPerRevolution); // Rotate stepper motor one revolution forward
            displayDrumBeat = true; // Mark that we are in the "DRUM BEAT" phase
        }
        drumBeatActive = true;
        // Blink "DRUM BEAT" text
        display2.clearDisplay();
        display2.setTextSize(2);
        if (isDisplayOn) {
            display2.setCursor((SCREEN_WIDTH - 9 * 12) / 2, (SCREEN_HEIGHT / 2) - 8);
            display2.print("DRUM BEAT");
        }
        display2.display();
    } else {
        if (displayDrumBeat) { // If exiting the "DRUM BEAT" phase
            myStepper.step(-stepsPerRevolution); // Rotate stepper motor one revolution backward
            displayDrumBeat = false; // Reset for the next phase
            isDisplayOn = true; // Make sure "STICK UP" is displayed without blinking
        }

        // Display "STICK UP" text without blinking
        drumBeatActive = false;
        soundPlayed = false;
        display2.clearDisplay();
        display2.setTextSize(2);
        display2.setCursor((SCREEN_WIDTH - 8 * 12) / 2, (SCREEN_HEIGHT / 2) - 8);
        display2.print("STICK UP");
        display2.display();
    }
}

void setup() {
  Serial.begin(115200);
  BLEDevice::init("");


  // Initialize first OLED with I2C addr 0x3C (128x64)
  if (!display1.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed for display 1"));
    for (;;); // Don't proceed, loop forever
  }

  // Initialize second OLED with I2C addr 0x3D (128x64)
  if (!display2.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    Serial.println(F("SSD1306 allocation failed for display 2"));
    for (;;); // Don't proceed, loop forever
  }

  // Clear the buffer for both displays
  display1.clearDisplay();
  display2.clearDisplay();

  // Setup display1 to show "Scanning..."
  display1.setTextSize(1);
  display1.setTextColor(SSD1306_WHITE);
  display1.setCursor(0, 0);
  display1.println("Scanning...");
  display1.display();

  // Setup display2 to show "Scanning..." as well
  display2.setTextSize(1);
  display2.setTextColor(SSD1306_WHITE);
  display2.setCursor(0, 0);
  display2.println("Scanning...");
  display2.display();

  // Setup stepper motor
  myStepper.setSpeed(60); // RPM

  // Initialize SPI for DAC
  hspi = new SPIClass(HSPI);
  hspi->begin(sckPin, misoPin, mosiPin, csPin); // Begin SPI with specified pins
  pinMode(csPin, OUTPUT);
  digitalWrite(csPin, HIGH); // Deselect the DAC
  

  // Start BLE scanning
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  if (doConnect) {
    doConnect = false;
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothing more we will do.");
    }
  }


  if (connected) {
    // Play sound based on display content, if not already played
    if (!soundPlayed && drumBeatActive) {
      // Determine the sound to play based on the gesture
      if (currentGestureText == "HI HAT") {
        playSound("HI HAT");
      } else if (currentGestureText == "SNARE DRUM") {
        playSound("SNARE DRUM");
      } else if (currentGestureText == "BASS DRUM") {
        playSound("BASS DRUM");
      } else if (currentGestureText == "CRASH CYMBAL") {
        playSound("CRASH CYMBAL");
      }
      soundPlayed = true; // Prevent the sound from playing repeatedly
    }
  } else if (doScan) {
    BLEDevice::getScan()->start(0);  // 0 means continuous scanning
  }

  if (connected) {
    // If connected, this is where you can interact with the BLE server
  } else if (doScan) {
    BLEDevice::getScan()->start(0);  // 0 means continuous scanning
  }
}

void playSound(const String &soundName) {
    Serial.println("Playing Sound: " + soundName);
    size_t audioDataLength = 0;
    const uint16_t* audioData = nullptr;

    if (soundName == "HI HAT") {
        audioData = hiHatData;
        audioDataLength = sizeof(hiHatData) / sizeof(hiHatData[0]);
    } else if (soundName == "SNARE DRUM") {
        audioData = snareDrumData;
        audioDataLength = sizeof(snareDrumData) / sizeof(snareDrumData[0]);
    } else if (soundName == "BASS DRUM") {
        audioData = bassDrumData;
        audioDataLength = sizeof(bassDrumData) / sizeof(bassDrumData[0]);
    } else if (soundName == "CRASH CYMBAL") {
        audioData = crashCymbalData;
        audioDataLength = sizeof(crashCymbalData) / sizeof(crashCymbalData[0]);
    } else {
        return; // No matching sound
    }

    int sampleDelay = round(1000000.0 / 44100 / 2); // Assuming a sample rate of 44100 Hz

    for (size_t i = 0; i < audioDataLength; i++) {
        int adjustedSample = static_cast<int>((static_cast<float>(audioData[i] - 2048) * volumeMultiplier) + 2048);
        adjustedSample = max(0, min(adjustedSample, 4095));
        sendValueToDAC(static_cast<uint16_t>(adjustedSample));
        delayMicroseconds(sampleDelay);
    }
}

void sendValueToDAC(uint16_t value) {
    digitalWrite(csPin, LOW); // Select DAC
    value &= 0x0FFF; // Ensure value is within 12-bit range
    uint16_t dataToSend = 0x3000 | value; // Format data for DAC
    
    // Begin SPI transaction, send value, then end transaction
    hspi->beginTransaction(SPISettings(5000000, MSBFIRST, SPI_MODE0));
    hspi->transfer16(dataToSend);
    hspi->endTransaction();
    digitalWrite(csPin, HIGH); // Deselect DAC
}
