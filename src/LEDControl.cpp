#include "LEDControl.h"

// Create the RGB Object
CRGB leds[NUM_LEDS];

// Previous state of the LED button
bool previousLedBtnState = HIGH;

// Light Status
uint8_t lightState = LED_STATE_OFF; // 0 = Light Off, 1 = Light On
uint8_t ledBrightness = MAX_BRIGHTNESS; // Default to full brightness (100%)

void initLEDs() {
    // Initialize FastLED with the LED strip configuration
    FastLED.addLeds<WS2812, LED_DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(255); // Set global brightness to 100% (255 is max)
    FastLED.clear(); // Ensure the LED is off initially
    FastLED.show();

    // Initialize LED button pin as input with internal pull-up resistor
    pinMode(LED_BTN, INPUT_PULLUP);
    
    Serial.println("LED Control Initialized!");
}

void handleLEDButton(bool childLockOn) {
    // Skip processing if child lock is active
    if (childLockOn) {
        return;
    }
    
    // Read the current state of the LED button (active LOW with pull-up)
    bool ledBtnState = !digitalRead(LED_BTN);

    // If the button was just pressed (transition from not pressed to pressed)
    if (ledBtnState == LOW && previousLedBtnState == HIGH) {
        Serial.println("LED Button Pressed");
        
        // Toggle the LED state
        setLEDState(lightState == LED_STATE_OFF ? LED_STATE_ON : LED_STATE_OFF);
    }
    
    // Update the previous button state for next iteration
    previousLedBtnState = ledBtnState;
}

void setLEDState(uint8_t state) {
    lightState = state;
    
    if (lightState == LED_STATE_ON) {
        // Turn on the LED (white color)
        leds[0] = CRGB::Blue;
        
        // Scale brightness from 0-100 to 0-255 for FastLED
        uint8_t scaledBrightness = map(ledBrightness, 0, 100, 0, 255);
        FastLED.setBrightness(scaledBrightness);
        Serial.println("LED turned ON");
    } else {
        // Turn off the LED
        leds[0] = CRGB::Black;
        Serial.println("LED turned OFF");
    }
    
    // Update the physical LED
    FastLED.show();
}

uint8_t getLEDState() {
    return lightState;
}

void setLEDBrightness(uint8_t brightness) {
    // Ensure brightness is in valid range (0-100)
    if (brightness > MAX_BRIGHTNESS) {
        brightness = MAX_BRIGHTNESS;
    }
    
    // Update the brightness
    ledBrightness = brightness;
    
    // Update the physical LED if it's on
    if (lightState == LED_STATE_ON) {
        // Scale brightness from 0-100 to 0-255 for FastLED
        uint8_t scaledBrightness = map(ledBrightness, 0, 100, 0, 255);
        FastLED.setBrightness(scaledBrightness);
        FastLED.show();
    }
    
    Serial.print("LED brightness set to: ");
    Serial.print(ledBrightness);
    Serial.println("%");
}

uint8_t getLEDBrightness() {
    return ledBrightness;
}
