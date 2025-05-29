#include "LEDControl.h"

// Create the NeoPixel strip object
Adafruit_NeoPixel strip(NUM_LEDS, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);

// Previous state of the LED button
bool previousLedBtnState = HIGH;

// Light Status
uint8_t lightState = LED_STATE_OFF; // 0 = Light Off, 1 = Light On
uint8_t ledBrightness = MAX_BRIGHTNESS; // Default to full brightness (100%)
String ledColorHex = "0000FF"; // Default to blue color

void initLEDs() {
    // Initialize NeoPixel strip
    strip.begin();
    strip.setBrightness(255); // Set global brightness to 100% (255 is max)
    strip.clear(); // Ensure the LED is off initially
    strip.show();

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

// Modify the existing setLEDState function to use the current color
void setLEDState(uint8_t state) {
    lightState = state;
    
    if (lightState == LED_STATE_ON) {
        // Use the current color instead of hardcoded blue
        uint8_t r = strtol(ledColorHex.substring(0, 2).c_str(), nullptr, 16);
        uint8_t g = strtol(ledColorHex.substring(2, 4).c_str(), nullptr, 16);
        uint8_t b = strtol(ledColorHex.substring(4, 6).c_str(), nullptr, 16);
        
        // Set the pixel color
        //strip.setPixelColor(0, r, g, b);
        strip.setPixelColor(0, 0, 255, 255);
        
        // Scale brightness from 0-100 to 0-255 for NeoPixel
        uint8_t scaledBrightness = map(ledBrightness, 0, 100, 0, 255);
        strip.setBrightness(scaledBrightness);
        Serial.println("LED turned ON");
    } else {
        // Turn off the LED
        strip.setPixelColor(0, 0, 0, 0);
        Serial.println("LED turned OFF");
    }
    
    // Update the physical LED
    strip.show();

    // Save the state change
    saveLEDState(lightState);
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
        // Scale brightness from 0-100 to 0-255 for NeoPixel
        uint8_t scaledBrightness = map(ledBrightness, 0, 100, 0, 255);
        strip.setBrightness(scaledBrightness);
        strip.show();
    }
    
    Serial.print("LED brightness set to: ");
    Serial.print(ledBrightness);
    Serial.println("%");

    // Save only the brightness setting
    saveLEDBrightness(ledBrightness);
}

uint8_t getLEDBrightness() {
    return ledBrightness;
}

void setLEDColor(String colorHex) {
    // Process the incoming color
    if (colorHex.length() == 8) {  // Expecting HEX value with alpha channel (e.g., "FF0019FF")
        // Truncate the first two characters (alpha channel) to keep RGB FF = 100% opacity
        // This is a 6-digit hex color code 
        String rgbColor = colorHex.substring(2);  // Get substring from the 3rd character onward
        ledColorHex = rgbColor; // Save the RGB part for tracking
        
        // Debug output
        Serial.printf("6-Bit HEX Color: %s\n", rgbColor.c_str());
        
        // Convert the RGB hex string to individual R, G, B values
        uint8_t r = strtol(rgbColor.substring(0, 2).c_str(), nullptr, 16);
        uint8_t g = strtol(rgbColor.substring(2, 4).c_str(), nullptr, 16);
        uint8_t b = strtol(rgbColor.substring(4, 6).c_str(), nullptr, 16);
        
        // Set the LED color if it's on
        if (lightState == LED_STATE_ON) {
            strip.setPixelColor(0, r, g, b);
            strip.show();
        }
        
        Serial.printf("LED color set to R:%d G:%d B:%d\n", r, g, b);
    } 
    else if (colorHex.length() == 6) {  // Already a 6-digit hex
        ledColorHex = colorHex; // Save for tracking
        
        // Convert the RGB hex string to individual R, G, B values
        uint8_t r = strtol(colorHex.substring(0, 2).c_str(), nullptr, 16);
        uint8_t g = strtol(colorHex.substring(2, 4).c_str(), nullptr, 16);
        uint8_t b = strtol(colorHex.substring(4, 6).c_str(), nullptr, 16);
        
        // Set the LED color if it's on
        if (lightState == LED_STATE_ON) {
            strip.setPixelColor(0, r, g, b);
            strip.show();
        }
        
        Serial.printf("LED color set to R:%d G:%d B:%d\n", r, g, b);
    }
    else {
        Serial.println("Invalid color format! Expected 6 or 8 character hex string.");
    }

    // Save only the color setting
    saveLEDColor(ledColorHex);
}

// Add this new function to get LED color
String getLEDColor() {
    return ledColorHex;
}