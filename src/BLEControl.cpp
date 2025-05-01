#include "BLEControl.h"
#include "MotorControl.h"
#include "LEDControl.h"  // Include LED Control to access functions
#include <Arduino.h>

// Implement callback functions from DoorStatusCallbacks class
void DoorStatusCallbacks::onWrite(BLECharacteristic* characteristic) {
    if (bleControl) {
        bleControl->handleDoorStatusWrite(characteristic);
    }
}

void DoorStatusCallbacks::onRead(BLECharacteristic* characteristic) {
    // Log when a client reads the door status characteristic
    if (characteristic->getUUID().toString() == UUID_DOOR_STATUS) {
        std::string value = characteristic->getValue();
        Serial.print("BLE Client read door status: ");
        Serial.println(value.c_str());
    }
}

// Implement callback functions from LEDStatusCallbacks class
void LEDStatusCallbacks::onWrite(BLECharacteristic* characteristic) {
    if (bleControl) {
        bleControl->handleLEDStatusWrite(characteristic);
    }
}

void LEDStatusCallbacks::onRead(BLECharacteristic* characteristic) {
    // Log when a client reads the LED status characteristic
    if (characteristic->getUUID().toString() == UUID_LIGHTS) {
        std::string value = characteristic->getValue();
        Serial.print("BLE Client read LED status: ");
        Serial.println(value.c_str());
    }
}

// BLEControl constructor that takes a reference to the podOpenFlag
BLEControl::BLEControl(bool* podOpenFlag) 
    : pServer(nullptr), pDoorStatus(nullptr), pLEDStatus(nullptr), podOpenFlagRef(podOpenFlag) {
}

void BLEControl::begin() {
    // Initialize BLE
    BLEDevice::init("Sole Pod");
    pServer = BLEDevice::createServer();

    // Create BLE Service
    BLEService* pService = pServer->createService(UUID_SERVICE);

    // Create Door Status Characteristic
    pDoorStatus = pService->createCharacteristic(
        UUID_DOOR_STATUS, 
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ
    );
    pDoorStatus->setCallbacks(new DoorStatusCallbacks(this));
    
    // Create LED Status Characteristic
    pLEDStatus = pService->createCharacteristic(
        UUID_LIGHTS,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ
    );
    pLEDStatus->setCallbacks(new LEDStatusCallbacks(this));
    
    // Set initial values based on current states
    updateDoorStatus(*podOpenFlagRef);
    updateLEDStatus(getLEDState());  // Get the current LED state from LEDControl

    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(UUID_SERVICE);
    pAdvertising->start();

    Serial.println("BLE Control initialized and advertising");
}

void BLEControl::handleDoorStatusWrite(BLECharacteristic* characteristic) {
    if (characteristic == pDoorStatus) {
        std::string value = characteristic->getValue();
        
        // Convert received value to string for easier processing
        String doorStatus = String(value.c_str());
        
        if (doorStatus == "1") {
            // Open the pod
            Serial.println("BLE Command: Open Pod");
            *podOpenFlagRef = true;
        } 
        else if (doorStatus == "0") {
            // Close the pod
            Serial.println("BLE Command: Close Pod");
            *podOpenFlagRef = false;
        }
        else {
            Serial.println("Invalid Door Status value received!");
        }
    }
}

void BLEControl::handleLEDStatusWrite(BLECharacteristic* characteristic) {
    if (characteristic == pLEDStatus) {
        std::string value = characteristic->getValue();
        
        // Convert received value to string for easier processing
        String ledStatus = String(value.c_str());
        
        if (ledStatus == "1") {
            // Turn on the LED
            Serial.println("BLE Command: Turn LED ON");
            setLEDState(LED_STATE_ON);
        } 
        else if (ledStatus == "0") {
            // Turn off the LED
            Serial.println("BLE Command: Turn LED OFF");
            setLEDState(LED_STATE_OFF);
        }
        else {
            Serial.println("Invalid LED Status value received!");
        }
    }
}

void BLEControl::updateDoorStatus(bool isOpen) {
    // Update the BLE characteristic with the current door state
    if (pDoorStatus) {
        String status = isOpen ? "1" : "0";
        pDoorStatus->setValue(status.c_str());
        Serial.print("BLE Door Status updated: ");
        Serial.println(status);
    }
}

void BLEControl::updateLEDStatus(uint8_t ledState) {
    // Update the BLE characteristic with the current LED state
    if (pLEDStatus) {
        String status = (ledState == LED_STATE_ON) ? "1" : "0";
        pLEDStatus->setValue(status.c_str());
        Serial.print("BLE LED Status updated: ");
        Serial.println(status);
    }
}