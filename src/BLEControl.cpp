#include "BLEControl.h"
#include "MotorControl.h"
#include "LEDControl.h"
#include <Arduino.h>

// Implement generic callback functions
void BLECharacteristicCallback::onWrite(BLECharacteristic* characteristic) {
    if (!bleControl) return;
    
    std::string uuid = characteristic->getUUID().toString();
    
    if (uuid == UUID_DOOR_STATUS) {
        bleControl->handleDoorStatusWrite(characteristic);
    }
    else if (uuid == UUID_DOOR_POSITION) {
        bleControl->handleDoorPositionWrite(characteristic);
    }
    else if (uuid == UUID_LIGHTS) {
        bleControl->handleLEDStatusWrite(characteristic);
    }
    else if (uuid == UUID_LIGHTS_BRIGHTNESS) {
        bleControl->handleLEDBrightnessWrite(characteristic);
    }

}

void BLECharacteristicCallback::onRead(BLECharacteristic* characteristic) {
    std::string uuid = characteristic->getUUID().toString();
    std::string value = characteristic->getValue();
    
    if (uuid == UUID_DOOR_STATUS) {
        Serial.print("BLE Client read door status: ");
        Serial.println(value.c_str());
    }
    else if (uuid == UUID_DOOR_POSITION) {
        Serial.print("BLE Client read door position: ");
        Serial.println(value.c_str());
    }
    else if (uuid == UUID_LIGHTS) {
        Serial.print("BLE Client read LED status: ");
        Serial.println(value.c_str());
    }
    else if (uuid == UUID_LIGHTS_BRIGHTNESS) {
        Serial.print("BLE Client read LED brightness: ");
        Serial.println(value.c_str());
    }
}

// BLEControl constructor that takes a reference to the podOpenFlag
BLEControl::BLEControl(bool* podOpenFlag) 
    : pServer(nullptr), pDoorStatus(nullptr), pDoorPosition(nullptr), pLEDStatus(nullptr), pLEDBrightness(nullptr), podOpenFlagRef(podOpenFlag) {
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
    pDoorStatus->setCallbacks(new BLECharacteristicCallback(this, UUID_DOOR_STATUS));

    // Create Door Position Characteristic
    pDoorPosition = pService->createCharacteristic(
        UUID_DOOR_POSITION, 
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ
    );
    pDoorPosition->setCallbacks(new BLECharacteristicCallback(this, UUID_DOOR_POSITION));
    
    // Create LED Status Characteristic
    pLEDStatus = pService->createCharacteristic(
        UUID_LIGHTS,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ
    );
    pLEDStatus->setCallbacks(new BLECharacteristicCallback(this, UUID_LIGHTS));
    
    // Create LED Brightness Characteristic
    pLEDBrightness = pService->createCharacteristic(
        UUID_LIGHTS_BRIGHTNESS,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ
    );
    pLEDBrightness->setCallbacks(new BLECharacteristicCallback(this, UUID_LIGHTS_BRIGHTNESS));
    
    // Set initial values based on current states
    updateDoorStatus(*podOpenFlagRef);
    updateDoorPosition(getDoorPosition());
    updateLEDStatus(getLEDState());
    
    // Convert internal brightness (0-255) to BLE brightness (0-100)
    uint8_t scaledBrightness = map(getLEDBrightness(), 0, 255, 0, 100);
    updateLEDBrightness(scaledBrightness);

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
        
        // Only accept valid values (0 or 1)
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
            Serial.println("Invalid Door Status value received! Only 0 or 1 allowed.");
        }
    }
}

void BLEControl::handleLEDStatusWrite(BLECharacteristic* characteristic) {
    if (characteristic == pLEDStatus) {
        std::string value = characteristic->getValue();
        
        // Convert received value to string for easier processing
        String ledStatus = String(value.c_str());
        
        // Only accept valid values (0 or 1)
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
            Serial.println("Invalid LED Status value received! Only 0 or 1 allowed.");
        }
    }
}

void BLEControl::handleLEDBrightnessWrite(BLECharacteristic* characteristic) {
    if (characteristic == pLEDBrightness) {
        std::string value = characteristic->getValue();
        
        // Convert received value to integer
        int brightness = 0;
        try {
            brightness = std::stoi(value);
            
            // Validate brightness range (0-100)
            if (brightness >= 0 && brightness <= 100) {
                Serial.print("BLE Command: Set LED Brightness to ");
                Serial.print(brightness);
                Serial.println("%");
                
                // Pass directly to LED control (already in 0-100 range)
                setLEDBrightness(brightness);
            } else {
                Serial.print("Invalid brightness value received: ");
                Serial.print(brightness);
                Serial.println(". Value must be between 0-100!");
            }
        } catch (...) {
            Serial.println("Invalid LED Brightness value received! Must be a number between 0-100.");
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

void BLEControl::updateLEDBrightness(uint8_t brightness) {
    // Ensure value is within allowed range (0-100)
    if (brightness > MAX_BRIGHTNESS) {
        brightness = MAX_BRIGHTNESS;
    }
    
    // Update the BLE characteristic with the current LED brightness
    if (pLEDBrightness) {
        String value = String(brightness);
        pLEDBrightness->setValue(value.c_str());
        Serial.print("BLE LED Brightness updated: ");
        Serial.print(brightness);
        Serial.println(" (0-100 scale)");
    }
}

void BLEControl::handleDoorPositionWrite(BLECharacteristic* characteristic) {
    if (characteristic == pDoorPosition) {
        std::string value = characteristic->getValue();
        
        // Convert received value to integer
        int position = 0;
        try {
            position = std::stoi(value);
            
            // Validate position (only 50 or 100 allowed)
            if (position == 50 || position == 100) {
                Serial.print("BLE Command: Set Door Position to ");
                Serial.println(position);
                
                // Update the door position
                setDoorPosition(position);
                
                // Update the BLE characteristic
                updateDoorPosition(position);
            } else {
                Serial.print("Invalid door position value received: ");
                Serial.print(position);
                Serial.println(". Value must be either 50 or 100!");
            }
        } catch (...) {
            Serial.println("Invalid Door Position value received! Must be either 50 or 100.");
        }
    }
}

// Add function to update the BLE characteristic:
void BLEControl::updateDoorPosition(uint8_t position) {
    // Ensure position is valid (50 or 100)
    if (position != 50 && position != 100) {
        position = 100; // Default to 100 if invalid
    }
    
    // Update the BLE characteristic
    if (pDoorPosition) {
        String value = String(position);
        pDoorPosition->setValue(value.c_str());
        Serial.print("BLE Door Position updated: ");
        Serial.println(position);
    }
}