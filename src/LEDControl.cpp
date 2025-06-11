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

// Helper function to convert hex string to RGB values
void hexToRGB(const String& hexColor, uint8_t& r, uint8_t& g, uint8_t& b) {
    // Ensure we have a 6-character hex string
    String processedHex = hexColor;
    if (processedHex.length() != 6) {
        Serial.println("Invalid hex color length in hexToRGB");
        r = g = b = 0;
        return;
    }
    
    // Convert hex string to RGB values (0-255 range)
    r = strtol(processedHex.substring(0, 2).c_str(), nullptr, 16);
    g = strtol(processedHex.substring(2, 4).c_str(), nullptr, 16);
    b = strtol(processedHex.substring(4, 6).c_str(), nullptr, 16);
}

// Helper function to update the physical LED with current color
void updateLEDColor() {
    if (lightState == LED_STATE_ON) {
        uint8_t r, g, b;
        hexToRGB(ledColorHex, r, g, b);
        
        // Set the pixel color - NeoPixel library will handle brightness scaling
        strip.setPixelColor(0, r, g, b);
        strip.show();
        
        Serial.printf("LED updated: R:%d G:%d B:%d (Brightness: %d%%)\n", r, g, b, ledBrightness);
    }
}

// Updated setLEDState function to use current color correctly
void setLEDState(uint8_t state) {
    lightState = state;
    
    if (lightState == LED_STATE_ON) {
        // Use the current stored color instead of hardcoded values
        updateLEDColor();
        Serial.println("LED turned ON");
    } else {
        // Turn off the LED
        strip.setPixelColor(0, 0, 0, 0);
        strip.show();
        Serial.println("LED turned OFF");
    }
    
    // Save the state change
    saveLEDState(lightState);
}

uint8_t getLEDState() {
    return lightState;
}

// Updated setLEDBrightness to ensure color is maintained and handle low values properly
void setLEDBrightness(uint8_t brightness) {
    // Ensure brightness is in valid range (0-100)
    if (brightness > MAX_BRIGHTNESS) {
        brightness = MAX_BRIGHTNESS;
    }
    
    // Update the brightness
    ledBrightness = brightness;
    
    // Update the physical LED if it's on and brightness > 0
    if (lightState == LED_STATE_ON && brightness > 0) {
        // Scale brightness from 1-100 to 10-255 for NeoPixel (better linear perception)
        // This avoids the problematic low values that can cause issues
        uint8_t scaledBrightness = map(ledBrightness, 1, 100, 10, 255);
        strip.setBrightness(scaledBrightness);
        updateLEDColor(); // This will show the color with new brightness
        Serial.printf("LED brightness set to: %d%% (scaled to %d/255)\n", ledBrightness, scaledBrightness);
    } else {
        // If brightness is 0 or LED is off, just store the brightness value
        // The actual LED state is handled elsewhere
        Serial.printf("LED brightness set to: %d%%\n", ledBrightness);
    }
    
    // Save only the brightness setting
    saveLEDBrightness(ledBrightness);
}

uint8_t getLEDBrightness() {
    return ledBrightness;
}

void setLEDColor(String colorHex) {
    String rgbColor;
    
    // Process the incoming color
    if (colorHex.length() == 8) {  
        // Has alpha channel (e.g., "FF0019FF") - remove first 2 characters
        rgbColor = colorHex.substring(2);
        Serial.printf("8-char HEX received, extracted RGB: %s\n", rgbColor.c_str());
    } 
    else if (colorHex.length() == 6) {  
        // Already a 6-digit hex
        rgbColor = colorHex;
        Serial.printf("6-char HEX received: %s\n", rgbColor.c_str());
    }
    else {
        Serial.printf("Invalid color format! Expected 6 or 8 character hex string, got %d characters: %s\n", 
                     colorHex.length(), colorHex.c_str());
        return;
    }
    
    // Validate hex characters
    for (int i = 0; i < rgbColor.length(); i++) {
        char c = rgbColor.charAt(i);
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
            Serial.printf("Invalid hex character '%c' in color string: %s\n", c, rgbColor.c_str());
            return;
        }
    }
    
    // Convert and validate RGB values
    uint8_t r, g, b;
    hexToRGB(rgbColor, r, g, b);
    
    // Store the new color (convert to uppercase for consistency)
    ledColorHex = rgbColor;
    ledColorHex.toUpperCase();
    
    // Update the physical LED if it's currently on
    updateLEDColor();
    
    Serial.printf("LED color set to R:%d G:%d B:%d (HEX: %s)\n", r, g, b, ledColorHex.c_str());
    
    // Save the color setting to persistent storage
    saveLEDColor(ledColorHex);
}

// Get LED color
String getLEDColor() {
    return ledColorHex;
}