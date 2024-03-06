#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Stepper.h>

// Change this to the number of steps per revolution for your motor
const int stepsPerRevolution = 200;

// Initialize the stepper library on the pins it is connected to
// Adjust the pin numbers according to your board's specific GPIO mapping
Stepper myStepper(stepsPerRevolution, D6, D3, D1, D2); // Example GPIOs for D6, D3, D1, D2

void setup() {
  // Set the speed of the motor (RPMs)
  myStepper.setSpeed(60);
}

void loop() {
  // Step one revolution in one direction:
  myStepper.step(stepsPerRevolution);
  delay(1000); // Wait for a second

  // Step one revolution in the opposite direction:
  myStepper.step(-stepsPerRevolution);
  delay(1000); // Wait for a second
}