#include <Arduino.h>
#include "Wire.h"
#include <Adafruit_SSD1306.h>

#define I2C_SDA_PIN 14
#define I2C_SCL_PIN 15
#define SCREEN_ADDRESS 0x3C
#define OLED_RESET    -1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 testDisplay(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET);

void setup(){
  Serial.begin(115200);
    // Initialize a quick, local SSD1306 test display on Wire1 (GP14=SDA, GP15=SCL)
    // Note: your display is powered from VBUS (5V). Ensure the display breakout
    // has a proper regulator and that I2C pull-ups are to 3.3V or the module
    // is 5V-tolerant on the I2C pins.
    Wire1.setSDA(I2C_SDA_PIN);
    Wire1.setSCL(I2C_SCL_PIN);
    Wire1.begin();
    Wire1.setClock(400000);

    // Quick I2C scan on Wire1
    Serial.println(F("\nScanning I2C (Wire1)..."));
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire1.beginTransmission(addr);
        uint8_t err = Wire1.endTransmission();
        if (err == 0) {
            Serial.print(F("I2C device found at 0x"));
            if (addr < 16) Serial.print('0');
            Serial.println(addr, HEX);
        }
    }

    Serial.print(F("Initializing test SSD1306 at 0x"));
    Serial.println(SCREEN_ADDRESS, HEX);
    if (!testDisplay.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("Test SSD1306 allocation failed"));
    } else {
        delay(50);
        testDisplay.clearDisplay();
        testDisplay.setTextSize(2);
        testDisplay.setTextColor(SSD1306_WHITE);
        testDisplay.setCursor(10, 20);
        testDisplay.println(F("ChoCo"));
        testDisplay.setTextSize(1);
        testDisplay.setCursor(10, 45);
        testDisplay.println(F("Display test"));
        testDisplay.display();
    }
}

void loop(){

}