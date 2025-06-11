#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <FastLED.h>
#include "SystemSettings.h"

// Define LED data pins and configuration
#define LED_DATA_PIN 14        // Data pin for the LEDs
#define NUM_LEDS 1             // Number of LEDs in the strip
#define LED_BTN 21             // Button pin for controlling the LED

// LED state constants
#define LED_STATE_OFF 0
#define LED_STATE_ON 1

// LED brightness constants
#define MAX_BRIGHTNESS 100

// Main function prototypes
void initLEDs();
void handleLEDButton(bool childLockOn);
void setLEDState(uint8_t state);
uint8_t getLEDState();
void setLEDBrightness(uint8_t brightness);
uint8_t getLEDBrightness();
void setLEDColor(String colorHex);
String getLEDColor();

// Helper function prototypes
void hexToRGB(const String& hexColor, uint8_t& r, uint8_t& g, uint8_t& b);
void updateLEDColor();

// External variable declarations
extern uint8_t ledBrightness;
extern String ledColorHex;
extern CRGB leds[NUM_LEDS];  // FastLED array

#endif // LED_CONTROL_H