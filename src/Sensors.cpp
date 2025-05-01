#include "Sensors.h"

// Array of switch pins for easy reference
const uint8_t SwitchPins[4] = {
    SW_DOOR_CLOSED,   // Index 0: Door closed switch
    SW_DOOR_OPENED,   // Index 1: Door opened switch
    SW_TRAY_CLOSED,   // Index 2: Tray closed switch
    SW_TRAY_OPENED    // Index 3: Tray opened switch
};

// Names of pod states for debugging
const char* StateNames[] = {
    "CLOSED",           // State 0: Door closed, tray closed
    "DOOR_MIDWAY",      // State 1: Door midway, tray closed
    "DOOR_OPEN",        // State 2: Door open, tray closed
    "TRAY_MIDWAY",      // State 3: Door open, tray midway
    "OPEN",             // State 4: Door open, tray open
    "UNDEFINED"         // State 5: Undefined state
};

void initSwitches() {
    // Initialize all switches as inputs with pull-up resistors
    pinMode(SW_DOOR_CLOSED, INPUT_PULLUP);
    pinMode(SW_DOOR_OPENED, INPUT_PULLUP);
    pinMode(SW_TRAY_CLOSED, INPUT_PULLUP);
    pinMode(SW_TRAY_OPENED, INPUT_PULLUP);
    
    Serial.println("Sensors Initialized!");
}

uint8_t readState() {
    // Read the current state of all switches
    uint8_t doorClosedState = digitalRead(SwitchPins[0]);
    uint8_t doorOpenedState = digitalRead(SwitchPins[1]);
    uint8_t trayClosedState = digitalRead(SwitchPins[2]);
    uint8_t trayOpenedState = digitalRead(SwitchPins[3]);
    
    // Determine pod state based on switch readings
    // Remember: LOW = switch activated (due to pull-up resistors)
    
    // State 0: Door Closed, Tray Closed
    if (doorClosedState == LOW && doorOpenedState == HIGH && 
        trayClosedState == LOW && trayOpenedState == HIGH) {
        return POD_STATE_CLOSED;
    }
    
    // State 1: Door Midway, Tray Closed
    else if (doorClosedState == HIGH && doorOpenedState == HIGH && 
             trayClosedState == LOW && trayOpenedState == HIGH) {
        return POD_STATE_DOOR_MIDWAY;
    }
    
    // State 2: Door Open, Tray Closed
    else if (doorClosedState == HIGH && doorOpenedState == LOW && 
             trayClosedState == LOW && trayOpenedState == HIGH) {
        return POD_STATE_DOOR_OPEN;
    }
    
    // State 3: Door Open, Tray Midway
    else if (doorClosedState == HIGH && doorOpenedState == LOW && 
             trayClosedState == HIGH && trayOpenedState == HIGH) {
        return POD_STATE_TRAY_MIDWAY;
    }
    
    // State 4: Door Open, Tray Open
    else if (doorClosedState == HIGH && doorOpenedState == LOW && 
             trayClosedState == HIGH && trayOpenedState == LOW) {
        return POD_STATE_OPEN;
    }
    
    // Undefined state
    else {
        return POD_STATE_UNDEFINED;
    }
}

const char* getStateDescription(uint8_t state) {
    // Return a descriptive name for the current state
    if (state <= POD_STATE_UNDEFINED) {
        return StateNames[state];
    } else {
        return "INVALID";
    }
}

bool isDoorClosed() {
    // Return true if door closed switch is activated
    return (digitalRead(SW_DOOR_CLOSED) == LOW);
}

bool isDoorOpen() {
    // Return true if door open switch is activated
    return (digitalRead(SW_DOOR_OPENED) == LOW);
}

bool isTrayOpen() {
    // Return true if tray open switch is activated
    return (digitalRead(SW_TRAY_OPENED) == LOW);
}

bool isTrayClose() {
    // Return true if tray closed switch is activated
    return (digitalRead(SW_TRAY_CLOSED) == LOW);
}