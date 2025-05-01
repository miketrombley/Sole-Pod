#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <FastLED.h>

// Define LED data pins and configuration
#define LED_DATA_PIN 14        // Data pin for the LEDs
#define NUM_LEDS 1             // Number of LEDs in the strip
#define LED_BTN 21             // Button pin for controlling the LED

// LED state constants
#define LED_STATE_OFF 0
#define LED_STATE_ON 1

// LED brightness constants
#define MAX_BRIGHTNESS 100

// Function prototypes
void initLEDs();
void handleLEDButton(bool childLockOn);
void setLEDState(uint8_t state);
uint8_t getLEDState();
void setLEDBrightness(uint8_t brightness);
uint8_t getLEDBrightness();
void setLEDColor(String colorHex);
String getLEDColor();

extern uint8_t ledBrightness;
extern String ledColorHex;

#endif // LED_CONTROL_H