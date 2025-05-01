#ifndef BLECONTROL_H
#define BLECONTROL_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLECharacteristic.h>
#include <BLEService.h>
#include <BLEAdvertising.h>

// Define Service and Characteristic UUIDs
#define UUID_SERVICE           "7d840001-11eb-4c13-89f2-246b6e0b0000"
#define UUID_DOOR_POSITION    "7d840003-11eb-4c13-89f2-246b6e0b0002"
#define UUID_DOOR_STATUS       "7d840002-11eb-4c13-89f2-246b6e0b0001"
#define UUID_LIGHTS            "7d840004-11eb-4c13-89f2-246b6e0b0003"
#define UUID_LIGHTS_BRIGHTNESS "7d840005-11eb-4c13-89f2-246b6e0b0004"

// Valid ranges for BLE characteristics
#define MIN_BRIGHTNESS 0
#define MAX_BRIGHTNESS 100

// Forward declaration
class BLEControl;

// Generic callback class for BLE characteristics
class BLECharacteristicCallback : public BLECharacteristicCallbacks {
private:
    BLEControl* bleControl;
    std::string characteristicUUID;

public:
    BLECharacteristicCallback(BLEControl* control, const std::string& uuid) 
        : bleControl(control), characteristicUUID(uuid) {}
    
    void onWrite(BLECharacteristic* characteristic) override;
    void onRead(BLECharacteristic* characteristic) override;
};

class BLEControl {
private:
    BLEServer* pServer;
    BLECharacteristic* pDoorStatus;
    BLECharacteristic* pDoorPosition;
    BLECharacteristic* pLEDStatus;
    BLECharacteristic* pLEDBrightness;
    
    // Reference to external state flag to control door state
    bool* podOpenFlagRef;

public:
    BLEControl(bool* podOpenFlag);
    void begin();
    
    // Handler for door status characteristic changes
    void handleDoorStatusWrite(BLECharacteristic* characteristic);

    // Handler for door position characteristic changes
    void handleDoorPositionWrite(BLECharacteristic* characteristic);
    
    // Handler for LED status characteristic changes
    void handleLEDStatusWrite(BLECharacteristic* characteristic);
    
    // Handler for LED brightness characteristic changes
    void handleLEDBrightnessWrite(BLECharacteristic* characteristic);
    
    // Updates the BLE characteristic based on the current door state
    void updateDoorStatus(bool isOpen);

    // Updates the BLE characteristic based on the current door position
    void updateDoorPosition(uint8_t position);
    
    // Updates the BLE characteristic based on the current LED state
    void updateLEDStatus(uint8_t ledState);
    
    // Updates the BLE characteristic based on the current LED brightness
    void updateLEDBrightness(uint8_t brightness);
};

#endif // BLECONTROL_H