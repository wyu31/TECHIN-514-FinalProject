#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "MPU6050_6Axis_MotionApps20.h"
#include <Adafruit_APDS9960.h>

MPU6050 mpu;
Adafruit_APDS9960 apds;
File myFile;

const int chipSelect = 7; // Ensure this is the correct pin for your setup

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Initialize MPU6050
  Serial.println("Initializing MPU6050...");
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed. Check connections.");
  } else {
    Serial.println("MPU6050 connection successful.");
  }

  // Initialize APDS9960
  Serial.println("Initializing APDS9960...");
  if (!apds.begin()) {
    Serial.println("Failed to initialize APDS9960. Check connections.");
  } else {
    Serial.println("APDS9960 initialized successfully.");
    apds.enableProximity(true);
    apds.enableGesture(true);
  }

  // Initialize SD Card
  Serial.println("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed! Check connections and ensure the card is inserted.");
  } else {
    Serial.println("SD card is ready for use.");
  }

  // Open or create 'data.txt' for writing
  myFile = SD.open("data.txt", FILE_WRITE);
  if (!myFile) {
    Serial.println("Error opening 'data.txt'.");
  } else {
    Serial.println("'data.txt' opened successfully.");
  }
}

void loop() {
  // MPU6050 reading
  VectorInt16 aa;
  if (mpu.getAcceleration(&aa.x, &aa.y, &aa.z)) {
    Serial.print("MPU6050 - aX = "); Serial.print(aa.x); Serial.print(" | aY = "); Serial.print(aa.y); Serial.print(" | aZ = "); Serial.println(aa.z);

    // Log to SD card
    if (myFile) {
      myFile.print("MPU6050 - aX = "); myFile.print(aa.x); myFile.print(" | aY = "); myFile.print(aa.y); myFile.print(" | aZ = "); myFile.println(aa.z);
      myFile.flush(); // Ensure data is written without needing to close the file
    } else {
      Serial.println("Error writing to 'data.txt'.");
    }
  } else {
    Serial.println("Error reading from MPU6050.");
  }

  // APDS9960 gesture detection
  if (apds.gestureValid()) {
    uint8_t gesture = apds.readGesture();
    Serial.print("APDS9960 Gesture Detected: ");
    switch (gesture) {
      case APDS9960_DOWN: Serial.println("DOWN"); break;
      case APDS9960_UP: Serial.println("UP"); break;
      case APDS9960_LEFT: Serial.println("LEFT"); break;
      case APDS9960_RIGHT: Serial.println("RIGHT"); break;
      default: Serial.println("UNKNOWN"); break;
    }
  }

  delay(1000); // Manage pace of readings and logging
}

void endSD() {
  if (myFile) {
    myFile.close();
    Serial.println("'data.txt' closed successfully.");
  }
}
