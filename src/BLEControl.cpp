#include "BLEControl.h"
#include "MotorControl.h"
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

// BLEControl constructor that takes a reference to the podOpenFlag
BLEControl::BLEControl(bool* podOpenFlag) 
    : pServer(nullptr), pDoorStatus(nullptr), podOpenFlagRef(podOpenFlag) {
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
    
    // Set initial value based on current door state
    updateDoorStatus(*podOpenFlagRef);

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

void BLEControl::updateDoorStatus(bool isOpen) {
    // Update the BLE characteristic with the current door state
    if (pDoorStatus) {
        String status = isOpen ? "1" : "0";
        pDoorStatus->setValue(status.c_str());
        Serial.print("BLE Door Status updated: ");
        Serial.println(status);
    }
}