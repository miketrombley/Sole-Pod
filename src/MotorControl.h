#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>
#include "Sensors.h" // Include Sensors.h to access state definitions
#include "SystemSettings.h"

// Define motor control pins
#define DOOR_MOTOR 46          // Door motor control pin (1=on, 0=off)
#define DOOR_DIRECTION 3       // Door motor direction (0=open, 1=close)
#define TRAY_MOTOR 10          // Tray motor control pin (1=on, 0=off)
#define TRAY_DIRECTION 11      // Tray motor direction (0=open, 1=close)

// Define button pins
#define DOOR_BTN 47            // Door button pin

// Define motor states (for motor action, not pod position)
#define MOTORS_OFF 0
#define DOOR_OPENING 1
#define TRAY_OPENING 2
#define TRAY_CLOSING 3
#define DOOR_CLOSING 4

// Function prototypes
void initMotors();
void handleDoorButton(bool &podOpenFlag, bool childLockOn);
void manageMotors(bool podOpenFlag);
void stopAllMotors();
void setPodState(uint8_t transition);
void podOpen();
void podClose();
uint8_t getDoorPosition();
void setDoorPosition(uint8_t position);

// Make externally available so other modules can check
extern bool systemLocked;
extern uint8_t doorPosition;

#endif // MOTOR_CONTROL_H