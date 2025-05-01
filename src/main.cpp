/*
Sole Pod Control System
Hardware compatibility:
- Control Board v1.4
- LED Board v1.2
- Hall Sensor Board v1.0

Description:
This firmware controls the Sole Pod system, managing door motors,
tray motors, LED lighting, and safety sensors.
*/

#include <Arduino.h>
#include <FastLED.h>
#include "MotorControl.h"
#include "Sensors.h"
#include "LEDControl.h"
#include "VoltageReader.h"
#include "SafetyController.h"

// Configuration settings
#define DEBUG_MODE true       // Enable/disable debug messages
#define LOOP_DELAY_MS 10      // Main loop delay in milliseconds
#define SAFETY_CHECK_INTERVAL 50 // Check safety every 50ms

// System state flags
bool childLockOn = false;     // Child lock status (true = locked)
bool podOpenFlag = false;     // Pod position flag (true = open, false = closed)
unsigned long lastSafetyCheckTime = 0; // For periodic safety checks

// Function prototypes
void setupSystem();
void runDoorControl();
void runLEDControl();
void runSafetyChecks();
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
    runSafetyChecks();
    
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
    
    // Initialize safety controller
    initSafetyController();
}

// Perform regular safety checks
void runSafetyChecks() {
    unsigned long currentTime = millis();
    
    // Run safety checks at regular intervals
    if (currentTime - lastSafetyCheckTime >= SAFETY_CHECK_INTERVAL) {
        // Check if it's safe to operate
        bool safeToOperate = isSafeToOperate();
        
        // If not safe, stop all motors and notify
        if (!safeToOperate) {
            // Just to be extra sure, stop all motors again
            stopAllMotors();
            
            // Disable the door and LED control buttons when locked
            // This ensures the user can't try to operate the unit
            childLockOn = true;
            
            if (DEBUG_MODE) {
                Serial.println("SAFETY CHECK FAILED: Motors stopped and controls disabled");
                Serial.println("Power cycle required to reset system");
            }
        }
        
        lastSafetyCheckTime = currentTime;
    }
}

// Handle door related functionality
void runDoorControl() {
    // Process door button input
    handleDoorButton(podOpenFlag, childLockOn);
    
    // Check if it's safe to operate the pod
    if (isSafeToOperate()) {
        // Manage motors based on current pod state
        manageMotors(podOpenFlag);
    } else {
        // Safety check failed, ensure motors are stopped
        stopAllMotors();
    }
}

// Handle LED related functionality
void runLEDControl() {
    // Process LED button input
    handleLEDButton(childLockOn);
}

// Print debug information to serial console
void printDebugInfo() {
    static unsigned long lastDebugTime = 0;
    
    // Only update debug info every 500ms to avoid flooding serial
    if (millis() - lastDebugTime > 500) {
        uint8_t currentState = readState();
        float voltage = readAverageVoltage();
        uint8_t safetyStatus = getSafetyStatus();
        
        Serial.println("--- System Status ---");
        
        // If system is locked, display a prominent warning
        if (systemLocked) {
            Serial.println("!!! SYSTEM LOCKED - POWER CYCLE REQUIRED !!!");
            Serial.println("!!! MOTOR STALL DETECTED - CHECK HARDWARE !!!");
        }
        
        Serial.print("Pod State: ");
        Serial.print(getStateDescription(currentState));
        Serial.print(" (");
        Serial.print(currentState);
        Serial.println(")");
        Serial.print("Target: ");
        Serial.println(podOpenFlag ? "OPENING/OPEN" : "CLOSING/CLOSED");
        Serial.print("Child Lock: ");
        Serial.println(childLockOn ? "ON" : "OFF");
        Serial.print("Door Position: ");
        Serial.println(getDoorPosition());
        Serial.print("LED State: ");
        Serial.println(getLEDState() == LED_STATE_ON ? "ON" : "OFF");
        Serial.print("Voltage: ");
        Serial.print(voltage);
        Serial.print("V (Stall Threshold: ");
        Serial.print(STALL_VOLTAGE_THRESHOLD);
        Serial.println("V)");
        Serial.print("Safety Status: ");
        
        // Show detailed safety status
        switch (safetyStatus) {
            case SAFETY_STATUS_OK:
                Serial.println("OK");
                break;
            case SAFETY_STATUS_MOTOR_STALL:
                Serial.println("CRITICAL FAULT: MOTOR STALL");
                break;
            case SAFETY_STATUS_OBSTACLE_DETECTED:
                Serial.println("FAULT: OBSTACLE DETECTED");
                break;
            default:
                Serial.print("FAULT CODE: ");
                Serial.println(safetyStatus);
                break;
        }
        
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