#include "MotorControl.h"
#include "Sensors.h"
#include "VoltageReader.h"

// Door position tracking
uint8_t doorPosition = 100; // 0 = Door Closed, 100 = Door Fully Open

// Previous state of the Door button
bool previousDoorBtnState = HIGH;

void initMotors() {
    // Set motor control pins as outputs
    pinMode(DOOR_MOTOR, OUTPUT);
    pinMode(DOOR_DIRECTION, OUTPUT);
    pinMode(TRAY_MOTOR, OUTPUT);
    pinMode(TRAY_DIRECTION, OUTPUT);

    // Initialize all motors to OFF state
    digitalWrite(DOOR_MOTOR, LOW);
    digitalWrite(TRAY_MOTOR, LOW);

    // Initialize door button pin as input with internal pull-up resistor
    pinMode(DOOR_BTN, INPUT_PULLUP);
    
    Serial.println("Motor Control Initialized!");
}

void manageMotors(bool podOpenFlag) {
    // Read the current state of the pod from sensors
    uint8_t currentState = readState();
    
    // Determine whether to open or close pod based on flag
    if (podOpenFlag) {
        podOpen();
    } else {
        podClose();
    }
}

void handleDoorButton(bool &podOpenFlag, bool childLockOn) {
    // Skip processing if child lock is active
    if (childLockOn) {
        return;
    }
    
    // Read the current state of the Door button (active LOW with pull-up)
    bool doorBtnState = !digitalRead(DOOR_BTN);

    // If the button was just pressed (transition from not pressed to pressed)
    if (doorBtnState == LOW && previousDoorBtnState == HIGH) {
        Serial.println("Door Button Pressed");
        podOpenFlag = !podOpenFlag; // Toggle the pod open/close state
    }
    
    // Update the previous button state for next iteration
    previousDoorBtnState = doorBtnState;
}

void stopAllMotors() {
    digitalWrite(DOOR_MOTOR, LOW);
    digitalWrite(TRAY_MOTOR, LOW);
}

// Pod opening sequence
void podOpen() {
    uint8_t currentState = readState();
    
    // Door Opening, Tray Closed (State 0->1->2)
    if (currentState == POD_STATE_CLOSED || currentState == POD_STATE_DOOR_MIDWAY) {
        setPodState(DOOR_OPENING);
    }
    // Door Open, Tray Opening (State 2->3->4)
    else if (currentState == POD_STATE_DOOR_OPEN || currentState == POD_STATE_TRAY_MIDWAY) {
        // Only open tray if door is fully open
        if (doorPosition == 100) {
            setPodState(TRAY_OPENING);
        }
    }
    // Door Open, Tray Open (State 4 - complete)
    else if (currentState == POD_STATE_OPEN) {
        setPodState(MOTORS_OFF);
    }
}

// Pod closing sequence
void podClose() {
    uint8_t currentState = readState();
    
    // Door Open, Tray Open or Closing (State 4->3->2)
    if (currentState == POD_STATE_OPEN || currentState == POD_STATE_TRAY_MIDWAY) {
        // Remove the doorPosition check when closing
        // Always close the tray when closing the pod
        setPodState(TRAY_CLOSING);
    }
    // Door Open or Closing, Tray Closed (State 2->1->0)
    else if (currentState == POD_STATE_DOOR_OPEN || currentState == POD_STATE_DOOR_MIDWAY) {
        setPodState(DOOR_CLOSING);
    }
    // Door Closed, Tray Closed (State 0 - complete)
    else if (currentState == POD_STATE_CLOSED) {
        setPodState(MOTORS_OFF);
    }
}

// Update motor states based on transition type
void setPodState(uint8_t transition) {
    switch (transition) {
        // All motors off
        case MOTORS_OFF:
            digitalWrite(DOOR_MOTOR, LOW);
            digitalWrite(TRAY_MOTOR, LOW);
            break;
            
        // Door opening
        case DOOR_OPENING:
            digitalWrite(DOOR_MOTOR, HIGH);
            //digitalWrite(DOOR_DIRECTION, LOW);  // Direction for opening (old)
            digitalWrite(DOOR_DIRECTION, HIGH);  // Direction for opening
            digitalWrite(TRAY_MOTOR, LOW);
            break;
            
        // Tray opening
        case TRAY_OPENING:
            digitalWrite(DOOR_MOTOR, LOW);
            digitalWrite(TRAY_MOTOR, HIGH);
            digitalWrite(TRAY_DIRECTION, LOW);  // Direction for opening
            break;
            
        // Tray closing
        case TRAY_CLOSING:
            digitalWrite(DOOR_MOTOR, LOW);
            digitalWrite(TRAY_MOTOR, HIGH);
            digitalWrite(TRAY_DIRECTION, HIGH); // Direction for closing
            break;
            
        // Door closing
        case DOOR_CLOSING:
            digitalWrite(DOOR_MOTOR, HIGH);
            //digitalWrite(DOOR_DIRECTION, HIGH); // Direction for closing (old)
            digitalWrite(DOOR_DIRECTION, LOW);  // Direction for opening
            digitalWrite(TRAY_MOTOR, LOW);
            break;
            
        default:
            // Invalid transition - stop all motors for safety
            stopAllMotors();
            break;
    }
}

uint8_t getDoorPosition() {
    return doorPosition;
}

void setDoorPosition(uint8_t position) {
    // Only allow values of 50 or 100
    if (position == 50 || position == 100) {
        doorPosition = position;
        Serial.print("Door position set to: ");
        Serial.println(doorPosition);
        
        // Save only the door position setting
        saveDoorPosition(doorPosition);
    } else {
        Serial.println("Invalid door position! Only 50 or 100 allowed.");
    }
}