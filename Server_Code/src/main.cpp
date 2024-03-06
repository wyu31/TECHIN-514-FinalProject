#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <Arduino.h>
#include <Arduino_APDS9960.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>

#define SERVICE_UUID        "89d32ebb-e935-47d3-9877-817c75c12572"
#define CHARACTERISTIC_UUID "10c5f81b-952e-4e3d-84a7-ab9e8ec83683"

struct DrumBeatData {
  int gesture;
  float AcclZ;
};

Adafruit_MPU6050 mpu;
float alpha = 0.5; // Initial filter constant
float filteredAccelZ = 0;
float previousAccelZ = 0; // Store the previous raw AccelZ for dynamic alpha adjustment

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long previousMillis = 0;
const long interval = 200; // Reduced interval for more frequent updates
int lastGestureId = 0;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

void setup() {
    Serial.begin(115200);
    Serial.print("start");
    Wire.begin();
    
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) {
            delay(10);
        }
    } else {
        Serial.println("MPU6050 is connected locally");
    }

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("MPU6050 initialized");

    if (!APDS.begin()) {
        Serial.println("Failed to initialize APDS9960");
    } else {
        Serial.println("APDS9960 is connected locally");
    }

    BLEDevice::init("Drumstick");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Drumstick is now advertising. Waiting for Player to connect...");
}

void loop() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Dynamically adjust alpha based on the change in acceleration
    float deltaAccelZ = fabs(a.acceleration.z - previousAccelZ);
    if (deltaAccelZ > 1.0) { // Threshold for significant movement, adjust as necessary
        alpha = 0.8; // More responsive to sudden changes
    } else {
        alpha = 0.2; // Smoother for minor changes
    }
    previousAccelZ = a.acceleration.z; // Update previousAccelZ

    filteredAccelZ = alpha * filteredAccelZ + (1.0 - alpha) * a.acceleration.z;
    Serial.println(filteredAccelZ);

    int gestureId = 0;
    if (APDS.gestureAvailable()) {
        switch (APDS.readGesture()) {
            case GESTURE_UP: gestureId = 1; break;
            case GESTURE_DOWN: gestureId = 2; break;
            case GESTURE_LEFT: gestureId = 3; break;
            case GESTURE_RIGHT: gestureId = 4; break;
            default: gestureId = 0;
        }
        Serial.print("Detected Gesture ID: ");
        Serial.println(gestureId);

        if (gestureId != 0) {
            lastGestureId = gestureId;
        }
    }

    if (deviceConnected) {
        DrumBeatData drumData = {lastGestureId, filteredAccelZ};
        uint8_t message[sizeof(DrumBeatData)];
        memcpy(message, &drumData, sizeof(DrumBeatData));

        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
            pCharacteristic->setValue(message,sizeof(DrumBeatData));
            pCharacteristic->notify();
            Serial.print("Notifying Player: Gesture ID: ");
            Serial.println(lastGestureId);
            Serial.println(filteredAccelZ);
            previousMillis = currentMillis;
        }
    }

    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        pServer->startAdvertising();
        oldDeviceConnected = deviceConnected;
        Serial.println("Drumstick starts advertising again");
    }

    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    delay(100);
}