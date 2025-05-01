/*
Sole Pod Control System
Hardware compatibility:
- Control Board v1.4
- LED Board v1.2
- Hall Sensor Board v1.0

Description:
This firmware controls the Sole Pod system, managing door motors,
tray motors, LED lighting, safety sensors, and BLE connectivity.
*/

#include <Arduino.h>
#include <FastLED.h>
#include "MotorControl.h"
#include "Sensors.h"
#include "LEDControl.h"
#include "VoltageReader.h"
#include "BLEControl.h" // Add BLEControl header

// Configuration settings
#define DEBUG_MODE true       // Enable/disable debug messages
#define LOOP_DELAY_MS 10      // Main loop delay in milliseconds

// System state flags
bool childLockOn = false;     // Child lock status (true = locked)
bool podOpenFlag = false;     // Pod position flag (true = open, false = closed)

// Create BLE Control instance
BLEControl bleControl(&podOpenFlag); // Pass a reference to podOpenFlag

// Function prototypes
void setupSystem();
void runDoorControl();
void runLEDControl();
void printDebugInfo();

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    // Wait for serial connection when in debug mode
    if (DEBUG_MODE) {
        delay(1000);  // Give time for serial monitor to connect
        Serial.println("Sole Pod System Starting...");
    }
    
    // Initialize all subsystems
    setupSystem();
    
    if (DEBUG_MODE) {
        Serial.println("System initialization complete!");
        Serial.print("Motor stall detection threshold set to: ");
        Serial.println(STALL_VOLTAGE_THRESHOLD);
    }
}

void loop() {
    // Run safety checks
    //runSafetyChecks();
    
    // Handle door control functionality
    runDoorControl();
    
    // Handle LED control functionality
    runLEDControl();
    
    // Print debug information if enabled
    if (DEBUG_MODE) {
        printDebugInfo();
    }
    
    // Small delay to stabilize loop timing
    delay(LOOP_DELAY_MS);
}

// Initialize all subsystems
void setupSystem() {
    // Initialize sensors
    initSwitches();
    
    // Initialize motor control
    initMotors();
    
    // Initialize voltage monitoring
    initVoltageReader();
    
    // Initialize LED control
    initLEDs();
    
    // Initialize BLE control
    bleControl.begin();
}

// Handle door related functionality
void runDoorControl() {
    static uint8_t prevState = POD_STATE_UNDEFINED;
    static bool prevOpenFlag = false;
    
    // Process door button input
    handleDoorButton(podOpenFlag, childLockOn);
    manageMotors(podOpenFlag);
    
    // Read current door state
    uint8_t currentState = readState();
    
    // Update BLE door status if:
    // 1. The physical door state has changed OR
    // 2. The target state (podOpenFlag) has changed
    if (prevState != currentState || prevOpenFlag != podOpenFlag) {
        // Determine if door is considered "open" for BLE status
        // Door is "open" when:
        // - It's physically open (POD_STATE_DOOR_OPEN or further)
        // - OR it's in transition and the target is "open" (podOpenFlag = true)
        bool doorIsOpenForBLE = false;
        
        if (currentState == POD_STATE_DOOR_OPEN || 
            currentState == POD_STATE_TRAY_MIDWAY || 
            currentState == POD_STATE_OPEN) {
            // Door is physically open or further in open state
            doorIsOpenForBLE = true;
        } 
        else if (currentState == POD_STATE_DOOR_MIDWAY) {
            // Door is in transition - use the target state
            doorIsOpenForBLE = podOpenFlag;
        }
        
        // Update BLE status
        bleControl.updateDoorStatus(doorIsOpenForBLE);
        
        // Update previous values for next iteration
        prevState = currentState;
        prevOpenFlag = podOpenFlag;
    }
}

void runLEDControl() {
    static uint8_t prevLEDState = 255; // Initialize with an invalid state to force first update
    static uint8_t prevLEDBrightness = 101; // Track previous brightness
    
    // Process LED button input
    handleLEDButton(childLockOn);
    
    // Read current LED state and brightness
    uint8_t currentLEDState = getLEDState();
    uint8_t currentLEDBrightness = getLEDBrightness();
    
    // Update BLE LED status if the LED state has changed
    if (prevLEDState != currentLEDState) {
        // Update BLE
        bleControl.updateLEDStatus(currentLEDState);
        
        // Update previous state for next iteration
        prevLEDState = currentLEDState;
    }
    
    // Update BLE LED brightness if the brightness has changed
    if (prevLEDBrightness != currentLEDBrightness) {
        // Brightness is already in 0-100 range, no scaling needed
        bleControl.updateLEDBrightness(currentLEDBrightness);
        
        // Update previous brightness for next iteration
        prevLEDBrightness = currentLEDBrightness;
    }
}

// Print debug information to serial console
void printDebugInfo() {
    static unsigned long lastDebugTime = 0;
    
    // Only update debug info every 500ms to avoid flooding serial
    if (millis() - lastDebugTime > 1000) {
        uint8_t currentState = readState();
        float voltage = readAverageVoltage();
        
        Serial.println("--- System Status ---");
        
        Serial.print("Pod State: ");
        Serial.print(getStateDescription(currentState));
        Serial.print(" (");
        Serial.print(currentState);
        Serial.print("LED State: ");
        Serial.print(getLEDState() == LED_STATE_ON ? "ON" : "OFF");
        Serial.print("LED Brightness: ");
        Serial.println(getLEDBrightness());
        Serial.println(")");
        Serial.print("Target: ");
        Serial.println(podOpenFlag ? "OPENING/OPEN" : "CLOSING/CLOSED");
        Serial.print("Child Lock: ");
        Serial.println(childLockOn ? "ON" : "OFF");
        Serial.print("Door Position: ");
        Serial.println(getDoorPosition());
        Serial.print("LED State: ");
        Serial.println(getLEDState() == LED_STATE_ON ? "ON" : "OFF");
        Serial.print("LED Brightness: ");
        Serial.println(getLEDBrightness());
        
        // Print individual sensor states
        Serial.println("Sensors:");
        Serial.print("  Door Closed: ");
        Serial.println(isDoorClosed() ? "ACTIVE" : "INACTIVE");
        Serial.print("  Door Open: ");
        Serial.println(isDoorOpen() ? "ACTIVE" : "INACTIVE");
        Serial.print("  Tray Closed: ");
        Serial.println(isTrayClose() ? "ACTIVE" : "INACTIVE");
        Serial.print("  Tray Open: ");
        Serial.println(isTrayOpen() ? "ACTIVE" : "INACTIVE");
        Serial.println("-------------------");
        
        lastDebugTime = millis();
    }
}