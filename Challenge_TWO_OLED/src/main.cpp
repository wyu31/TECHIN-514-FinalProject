#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED display TWI (I2C) settings
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

// Declaration for two SSD1306 displays connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);

  // Initialize first OLED with I2C addr 0x3C (128x64)
  if(!display1.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed for display 1"));
    for(;;); // Don't proceed, loop forever
  }

  // Initialize second OLED with I2C addr 0x3D (128x64)
  if(!display2.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    Serial.println(F("SSD1306 allocation failed for display 2"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display1.clearDisplay();
  display2.clearDisplay();

  // Setup for display 1 ("Hello, World!")
  display1.setTextSize(1);      // Normal 1:1 pixel scale
  display1.setTextColor(SSD1306_WHITE); // Draw white text
  display1.setCursor(0,0);     // Start at top-left corner
  display1.println(F("Hello, World!"));
  display1.display();

  // No need to draw anything on display 2 here; animation will be handled in loop()
}

void loop() {
  static int x = 0; // Horizontal position of the animation dot
  static int y = (SCREEN_HEIGHT / 2); // Vertical position of the animation dot, centered
  display2.clearDisplay(); // Clear the display buffer

  // Draw a dot moving across the screen
  display2.fillCircle(x, y, 5, SSD1306_WHITE); // Draw a dot with radius 5
  display2.display(); // Update the display with the drawn dot
  
  x += 5; // Move the dot to the right on each loop
  if(x > SCREEN_WIDTH) x = 0; // Reset position to loop the animation

  delay(100); // Delay to slow down the animation speed
}