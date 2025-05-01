#ifndef SYSTEM_SETTINGS_H
#define SYSTEM_SETTINGS_H

#include <Arduino.h>
#include <Preferences.h>

// Define settings namespace
#define SETTINGS_NAMESPACE "solepod"

// Function prototypes
void initSettings();

// Individual save functions for each setting
void saveLEDColor(String ledColor);
void saveLEDBrightness(uint8_t ledBrightness);
void saveDoorPosition(uint8_t doorPosition);
void saveLEDState(uint8_t ledState); // New function

// Renamed getter functions to avoid naming conflicts
String getSavedLEDColor(String defaultColor = "0000FF");
uint8_t getSavedLEDBrightness(uint8_t defaultBrightness = 100);
uint8_t getSavedDoorPosition(uint8_t defaultPosition = 100);
uint8_t getSavedLEDState(uint8_t defaultState = 0); // Use raw value 0 instead of LED_STATE_OFF

// Function to load all settings at once during startup
bool loadAllSettings(String &ledColor, uint8_t &ledBrightness, 
                    uint8_t &doorPosition, uint8_t &ledState);

#endif // SYSTEM_SETTINGS_H