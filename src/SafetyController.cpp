#include "SafetyController.h"
#include "VoltageReader.h"
#include "Sensors.h"
#include "MotorControl.h" // Added for access to stopAllMotors()

// Current safety status
uint8_t currentSafetyStatus = SAFETY_STATUS_OK;

// System lockout flag - requires power cycle to reset
bool systemLocked = false;

void initSafetyController() {
    // Initialize safety controller
    currentSafetyStatus = SAFETY_STATUS_OK;
    systemLocked = false;
    
    Serial.println("Safety Controller Initialized!");
}

bool isSafeToOperate() {
    // Check for system lockout - this is the "master kill switch"
    // Once locked, only a power cycle (restart) will reset it
    if (systemLocked) {
        Serial.println("SYSTEM LOCKED: Power cycle required to reset");
        stopAllMotors(); // Ensure motors are stopped
        return false;
    }
    
    // Check for motor stall condition
    if (isStallDetected()) {
        // Set the system to locked state - only power cycle will reset
        systemLocked = true;
        currentSafetyStatus = SAFETY_STATUS_MOTOR_STALL;
        
        Serial.println("CRITICAL FAULT: Motor stall detected!");
        Serial.println("System is now LOCKED - power cycle required to reset");
        
        // Immediately stop all motors when stall is detected
        stopAllMotors();
        
        return false;
    }
    
    // All checks passed
    return true;
}

uint8_t getSafetyStatus() {
    return currentSafetyStatus;
}

void logSafetyEvent(uint8_t eventType, const char* message) {
    // Print safety event to serial
    Serial.print("SAFETY EVENT: ");
    Serial.print(message);
    Serial.print(" (Code: ");
    Serial.print(eventType);
    Serial.println(")");
    
    // For motor stall events, automatically lock the system
    if (eventType == SAFETY_STATUS_MOTOR_STALL) {
        systemLocked = true;
    }
}

// This function is now for testing only - in production, a power cycle is required
// to reset the system after a motor stall
void resetSafetyStatus() {
    // In production, this function would not be used - only kept for testing
    currentSafetyStatus = SAFETY_STATUS_OK;
    systemLocked = false;
    Serial.println("Safety status manually reset - FOR TESTING ONLY");
    Serial.println("In production, power cycle is required after a motor stall");
}