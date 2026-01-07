#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED pins
#define SDA_PIN 21
#define SCL_PIN 22

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(115200);
  Serial.println("OLED Test Starting...");

  // Start I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize OLED (address 0x3C)
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED INIT FAILED!");
    while (true);  // Stop here if failed
  }

  Serial.println("OLED Initialized Successfully!");

  // Show simple test message
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.println("WORKING!");
  display.display();
}

void loop() {
  // Keep showing the same message
}
