#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <Arduino.h>
#include <Arduino_APDS9960.h>

// Assuming APDS9960 initialization is correct as per your setup
Adafruit_MPU6050 mpu;

// Low-pass filter parameters for MPU6050
float alpha = 0.5; // Filter constant
float filteredAccelX = 0, filteredAccelY = 0, filteredAccelZ = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for Serial monitor to open

  // Initialize APDS9960
  if (!APDS.begin()) {
    Serial.println("Error initializing APDS-9960 sensor!");
  } else {
    Serial.println("APDS-9960 sensor initialized.");
  }

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10); // Hang if MPU6050 is not found
      Serial.print('.');
    }
  } else {
    Serial.println("MPU6050 Found!");
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("MPU6050 sensor initialized.");
  }

  Serial.println("Starting data collection...");
}

void loop() {
  // Gesture detection with APDS9960
  if (APDS.gestureAvailable()) {
    int gesture = APDS.readGesture();
    switch (gesture) {
      case GESTURE_UP:
        Serial.println("Detected UP gesture");
        break;
      case GESTURE_DOWN:
        Serial.println("Detected DOWN gesture");
        break;
      case GESTURE_LEFT:
        Serial.println("Detected LEFT gesture");
        break;
      case GESTURE_RIGHT:
        Serial.println("Detected RIGHT gesture");
        break;
      default:
        // If no recognizable gesture was detected
        break;
    }
  }

  // MPU6050 data acquisition
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Apply a simple low-pass filter to the accelerometer data
  filteredAccelX = alpha * filteredAccelX + (1.0 - alpha) * a.acceleration.x;
  filteredAccelY = alpha * filteredAccelY + (1.0 - alpha) * a.acceleration.y;
  filteredAccelZ = alpha * filteredAccelZ + (1.0 - alpha) * a.acceleration.z;

  // Print filtered accelerometer data along with gyroscope data
  Serial.print("Accel (X, Y, Z): ");
  Serial.print(filteredAccelX); Serial.print(", ");
  Serial.print(filteredAccelY); Serial.print(", ");
  Serial.print(filteredAccelZ); Serial.print(" | Gyro (X, Y, Z): ");
  Serial.print(g.gyro.x); Serial.print(", ");
  Serial.print(g.gyro.y); Serial.print(", ");
  Serial.println(g.gyro.z);

  delay(100); // Short delay to manage the loop timing
}