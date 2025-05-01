#include "SystemSettings.h"

// Create a preferences object
Preferences preferences;

void initSettings() {
    // Initialize the settings module
    preferences.begin(SETTINGS_NAMESPACE, false);
    preferences.end();
    
    Serial.println("Settings module initialized");
}

// Individual save functions

void saveLEDColor(String ledColor) {
    preferences.begin(SETTINGS_NAMESPACE, false);
    preferences.putString("ledColor", ledColor);
    preferences.end();
    
    Serial.print("LED Color saved: ");
    Serial.println(ledColor);
}

void saveLEDBrightness(uint8_t ledBrightness) {
    preferences.begin(SETTINGS_NAMESPACE, false);
    preferences.putUChar("ledBright", ledBrightness);
    preferences.end();
    
    Serial.print("LED Brightness saved: ");
    Serial.println(ledBrightness);
}

void saveDoorPosition(uint8_t doorPosition) {
    preferences.begin(SETTINGS_NAMESPACE, false);
    preferences.putUChar("doorPos", doorPosition);
    preferences.end();
    
    Serial.print("Door Position saved: ");
    Serial.println(doorPosition);
}

void saveLEDState(uint8_t ledState) {
    preferences.begin(SETTINGS_NAMESPACE, false);
    preferences.putUChar("ledState", ledState);
    preferences.end();
    
    Serial.print("LED State saved: ");
    Serial.println(ledState == 1 ? "ON" : "OFF");
}


// Renamed getter functions to avoid naming conflicts

String getSavedLEDColor(String defaultColor) {
    preferences.begin(SETTINGS_NAMESPACE, true);
    String color = preferences.getString("ledColor", defaultColor);
    preferences.end();
    return color;
}

uint8_t getSavedLEDBrightness(uint8_t defaultBrightness) {
    preferences.begin(SETTINGS_NAMESPACE, true);
    uint8_t brightness = preferences.getUChar("ledBright", defaultBrightness);
    preferences.end();
    return brightness;
}

uint8_t getSavedDoorPosition(uint8_t defaultPosition) {
    preferences.begin(SETTINGS_NAMESPACE, true);
    uint8_t position = preferences.getUChar("doorPos", defaultPosition);
    preferences.end();
    return position;
}

uint8_t getSavedLEDState(uint8_t defaultState) {
    preferences.begin(SETTINGS_NAMESPACE, true);
    uint8_t state = preferences.getUChar("ledState", defaultState);
    preferences.end();
    return state;
}

// Function to load all settings at once during startup
bool loadAllSettings(String &ledColor, uint8_t &ledBrightness, 
                    uint8_t &doorPosition, uint8_t &ledState) {
    bool settingsExist = false;
    
    preferences.begin(SETTINGS_NAMESPACE, true);
    
    // Check if we have saved settings
    settingsExist = preferences.isKey("ledColor");
    
    // Load settings
    ledColor = preferences.getString("ledColor", "0000FF");
    ledBrightness = preferences.getUChar("ledBright", 100);
    doorPosition = preferences.getUChar("doorPos", 100);
    ledState = preferences.getUChar("ledState", 0); // Default to OFF (0)
    
    preferences.end();
    
    if (settingsExist) {
        Serial.println("Settings loaded from flash memory:");
        Serial.print("LED Color: ");
        Serial.println(ledColor);
        Serial.print("LED Brightness: ");
        Serial.println(ledBrightness);
        Serial.print("Door Position: ");
        Serial.println(doorPosition);
        Serial.print("LED State: ");
        Serial.println(ledState == 1 ? "ON" : "OFF");
    } else {
        Serial.println("No saved settings found, using defaults");
    }
    
    return settingsExist;
}