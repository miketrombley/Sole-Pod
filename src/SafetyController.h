#ifndef SAFETY_CONTROLLER_H
#define SAFETY_CONTROLLER_H

#include <Arduino.h>

// Safety status codes
#define SAFETY_STATUS_OK 0
#define SAFETY_STATUS_MOTOR_STALL 1
#define SAFETY_STATUS_OBSTACLE_DETECTED 2
#define SAFETY_STATUS_OVERCURRENT 3
#define SAFETY_STATUS_SYSTEM_ERROR 4

// External system lock flag - only power cycle resets after motor stall
extern bool systemLocked;

// Function prototypes
void initSafetyController();
bool isSafeToOperate();
uint8_t getSafetyStatus();
void logSafetyEvent(uint8_t eventType, const char* message);
void resetSafetyStatus();

#endif // SAFETY_CONTROLLER_H