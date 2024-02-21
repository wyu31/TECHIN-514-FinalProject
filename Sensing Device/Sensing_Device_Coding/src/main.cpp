#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <MPU6050_6Axis_MotionApps20.h>
#include <Adafruit_APDS9960.h>

MPU6050 mpu;
Adafruit_APDS9960 apds;
File myFile;

const int chipSelect = D7; 

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  // Initialize MPU6050
  mpu.initialize();
  if (!mpu.testConnection()) Serial.println("MPU6050 connection failed");
  else Serial.println("MPU6050 connection successful");
  
  // Initialize APDS9960
  if (!apds.begin()) Serial.println("APDS9960 connection failed");
  else {
    Serial.println("APDS9960 connection successful");
    apds.enableProximity(true);
    apds.enableGesture(true);
  }
  
  // Initialize SD Card
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    return;
  } else {
    Serial.println("SD card initialized successfully");
  }
  
  // Create or open the file
  myFile = SD.open("data.txt", FILE_WRITE);
  if (!myFile) Serial.println("Error opening data.txt");
}

void loop() {
  // Read acceleration from MPU6050
  VectorInt16 aa;
  mpu.getAcceleration(&aa.x, &aa.y, &aa.z);
  Serial.print("MPU6050 - aX = "); Serial.print(aa.x); Serial.print(" | aY = "); Serial.print(aa.y); Serial.print(" | aZ = "); Serial.println(aa.z);
  
  // Log acceleration data to SD card
  if (myFile) {
    myFile.print("MPU6050 - aX = "); myFile.print(aa.x); myFile.print(" | aY = "); myFile.print(aa.y); myFile.print(" | aZ = "); myFile.println(aa.z);
    myFile.close(); // Close to save data
    Serial.println("Data written to SD card");
  } else {
    Serial.println("Error writing to data.txt");
  }
  
  // Reopen the file for the next write
  myFile = SD.open("data.txt", FILE_WRITE);
  
  // Check for gesture from APDS9960
  if (apds.gestureValid()) {
    uint8_t gesture = apds.readGesture();
    Serial.print("APDS9960 Gesture Detected: ");
    switch (gesture) {
      case APDS9960_DOWN: Serial.println("DOWN"); break;
      case APDS9960_UP: Serial.println("UP"); break;
      case APDS9960_LEFT: Serial.println("LEFT"); break;
      case APDS9960_RIGHT: Serial.println("RIGHT"); break;
      default: Serial.println("UNKNOWN");
    }
  }

  delay(1000); // Delay to manage the pace of data logging and serial output
}