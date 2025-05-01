#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

// Define sensor pins
#define SW_DOOR_CLOSED 45    // Switch indicating door is fully closed
#define SW_DOOR_OPENED 48    // Switch indicating door is fully opened
#define SW_TRAY_CLOSED 35    // Switch indicating tray is fully closed
#define SW_TRAY_OPENED 36    // Switch indicating tray is fully opened

// Pod state definitions
#define POD_STATE_CLOSED 0           // Door closed, tray closed
#define POD_STATE_DOOR_MIDWAY 1      // Door opening/closing, tray closed
#define POD_STATE_DOOR_OPEN 2        // Door open, tray closed
#define POD_STATE_TRAY_MIDWAY 3      // Door open, tray opening/closing
#define POD_STATE_OPEN 4             // Door open, tray open
#define POD_STATE_UNDEFINED 5        // Undefined state (error condition)

// Function prototypes
void initSwitches();
uint8_t readState();
const char* getStateDescription(uint8_t state);
bool isDoorClosed();
bool isDoorOpen();
bool isTrayOpen();
bool isTrayClose();

#endif // SENSORS_H