#include <Wire.h>
#include <Arduino.h>
#include <SPI.h>
#include "Hi_Hat_data.h" // Make sure this correctly points to your header file

const int csPin = 44; // Chip Select pin for MCP4921 DAC
const int sckPin = 7;  // SPI Clock pin
const int mosiPin = 9; // SPI MOSI (Master Out Slave In) pin
const int misoPin = 8; // SPI MISO (Master In Slave Out) pin, not used in this setup
const int potPin = A1;  // Analog pin for reading the potentiometer

unsigned long lastPotReadTime = 0;             // Time of the last potentiometer read
const unsigned long potReadInterval = 100;     // Interval between potentiometer reads (milliseconds)
float volumeMultiplier = 1.0;                  // Multiplier for adjusting volume
float baseVolumeLevel = 0.25;                  // Base volume level reduced to make sound quieter

SPIClass *hspi = NULL; // Pointer for the SPI class

void sendValueToDAC(uint16_t value);

bool soundPlayed = false; // Flag to check if the sound has been played

void setup() {
    Serial.begin(115200); // Start serial communication at 115200 baud rate
    analogReadResolution(12); // Set ADC resolution to 12-bit (0-4095)
    pinMode(potPin, INPUT); // Set the potentiometer pin as input

    // Initialize SPI for DAC
    hspi = new SPIClass(HSPI);
    hspi->begin(sckPin, misoPin, mosiPin, csPin); // Begin SPI with specified pins
    pinMode(csPin, OUTPUT);
    digitalWrite(csPin, HIGH); // Deselect the DAC
}

void loop() {
    if (!soundPlayed) {
        if (millis() - lastPotReadTime >= potReadInterval) {
            // Read potentiometer value, adjust with base volume level, and update volume multiplier
            uint16_t potValue = analogRead(potPin);
            volumeMultiplier = (static_cast<float>(potValue) / 4095.0) * baseVolumeLevel; // Apply base volume level
            lastPotReadTime = millis();
        }

        size_t audioDataLength = sizeof(hiHatData) / sizeof(hiHatData[0]);
        int sampleDelay = round(1000000.0 / 44100 / 2); // Calculate delay for 44100 Hz sample rate

        for (size_t i = 0; i < audioDataLength; i++) {
            // Adjust audio sample by volume and ensure it's within 12-bit range
            int adjustedSample = static_cast<int>((static_cast<float>(hiHatData[i] - 2048) * volumeMultiplier) + 2048);
            adjustedSample = max(0, min(adjustedSample, 4095));
            sendValueToDAC(static_cast<uint16_t>(adjustedSample));
            delayMicroseconds(sampleDelay); // Delay between samples for correct playback rate
        }
        soundPlayed = true; // Set the flag to true after playing the sound
    }
    // Optionally, add a delay or other logic here if needed when not playing sound
    // For example, enter a low power mode, perform other tasks, or simply do nothing
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
