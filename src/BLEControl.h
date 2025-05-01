#ifndef BLECONTROL_H
#define BLECONTROL_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLECharacteristic.h>
#include <BLEService.h>
#include <BLEAdvertising.h>

// Define Service and Characteristic UUIDs
#define UUID_SERVICE        "7d840001-11eb-4c13-89f2-246b6e0b0000"
#define UUID_DOOR_STATUS    "7d840002-11eb-4c13-89f2-246b6e0b0001"
#define UUID_LIGHTS         "7d840004-11eb-4c13-89f2-246b6e0b0003"

// Forward declaration
class BLEControl;

// Callback class for BLE characteristics
class DoorStatusCallbacks : public BLECharacteristicCallbacks {
private:
    BLEControl* bleControl;

public:
    explicit DoorStatusCallbacks(BLEControl* control) : bleControl(control) {}
    void onWrite(BLECharacteristic* characteristic) override;
    void onRead(BLECharacteristic* characteristic) override;
};

// Callback class for LED control
class LEDStatusCallbacks : public BLECharacteristicCallbacks {
private:
    BLEControl* bleControl;

public:
    explicit LEDStatusCallbacks(BLEControl* control) : bleControl(control) {}
    void onWrite(BLECharacteristic* characteristic) override;
    void onRead(BLECharacteristic* characteristic) override;
};

class BLEControl {
private:
    BLEServer* pServer;
    BLECharacteristic* pDoorStatus;
    BLECharacteristic* pLEDStatus;
    
    // Reference to external state flag to control door state
    bool* podOpenFlagRef;

public:
    BLEControl(bool* podOpenFlag);
    void begin();
    
    // Handler for door status characteristic changes
    void handleDoorStatusWrite(BLECharacteristic* characteristic);
    
    // Handler for LED status characteristic changes
    void handleLEDStatusWrite(BLECharacteristic* characteristic);
    
    // Updates the BLE characteristic based on the current door state
    void updateDoorStatus(bool isOpen);
    
    // Updates the BLE characteristic based on the current LED state
    void updateLEDStatus(uint8_t ledState);
};

#endif // BLECONTROL_H