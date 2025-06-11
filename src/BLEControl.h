#ifndef BLECONTROL_H
#define BLECONTROL_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLECharacteristic.h>
#include <BLEService.h>
#include <BLEAdvertising.h>
#include <ArduinoJson.h>  // Add this for JSON support
#include "WiFiControl.h"

// Define Service and Characteristic UUIDs
#define UUID_SERVICE           "7d840001-11eb-4c13-89f2-246b6e0b0000"
#define UUID_DOOR_STATUS       "7d840002-11eb-4c13-89f2-246b6e0b0001"
#define UUID_DOOR_POSITION     "7d840003-11eb-4c13-89f2-246b6e0b0002"
#define UUID_LIGHTS            "7d840004-11eb-4c13-89f2-246b6e0b0003"
#define UUID_LIGHTS_BRIGHTNESS "7d840005-11eb-4c13-89f2-246b6e0b0004"
#define UUID_LIGHTS_COLOR      "7d840006-11eb-4c13-89f2-246b6e0b0005"
#define UUID_WIFI_CREDENTIALS  "7d840007-11eb-4c13-89f2-246b6e0b0006"
#define UUID_WIFI_STATUS       "7d840008-11eb-4c13-89f2-246b6e0b0007"
#define UUID_CHILD_LOCK        "7d840006-11eb-4c13-89f2-246b6e0b0008"
#define UUID_JSON_STATUS       "7d840009-11eb-4c13-89f2-246b6e0b0009"  // New JSON status characteristic

// Valid ranges for BLE characteristics
#define MIN_BRIGHTNESS 0
#define MAX_BRIGHTNESS 100

// JSON update interval (milliseconds)
#define JSON_UPDATE_INTERVAL 1000  // Update every 1 second

// Forward declarations
class BLEControl;

// BLE Server callback class to handle connections
class BLEServerCallback : public BLEServerCallbacks {
private:
    BLEControl* bleControl;

public:
    BLEServerCallback(BLEControl* control) : bleControl(control) {}
    
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;
};

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
    BLEAdvertising* pAdvertising;
    BLECharacteristic* pDoorStatus;
    BLECharacteristic* pDoorPosition;
    BLECharacteristic* pLEDStatus;
    BLECharacteristic* pLEDBrightness;
    BLECharacteristic* pLEDColor;
    BLECharacteristic* pWiFiCredentials;
    BLECharacteristic* pWiFiStatus;
    BLECharacteristic* pChildLock;
    BLECharacteristic* pJSONStatus;  // New JSON status characteristic
    
    // Connection state tracking
    bool isClientConnected;
    uint16_t connectedClientId;
    
    // Reference to external state flag to control door state
    bool* podOpenFlagRef;
    
    // Reference to WiFi controller
    WiFiControl* wifiControlRef;
    
    // Reference to child lock state
    bool* childLockRef;
    
    // Network credentials buffers
    String networkBuffer;
    String passwordBuffer;
    
    // JSON update timing
    unsigned long lastJSONUpdate;

public:
    // Constructor
    BLEControl(bool* podOpenFlag, WiFiControl* wifiControl, bool* childLock);
    
    // Main initialization
    void begin();
    
    // JSON status management
    void updateJSONStatus();
    void checkJSONUpdate();  // Call this from main loop
    
    // Connection management
    void startAdvertising();
    void stopAdvertising();
    bool getConnectionStatus() const { return isClientConnected; }
    
    // Internal setup methods
    void createCharacteristics(BLEService* pService);
    void setInitialValues();
    
    // Connection event handlers (called by BLEServerCallback)
    void handleClientConnect(uint16_t clientId);
    void handleClientDisconnect();
    
    // Characteristic write handlers
    void handleDoorStatusWrite(BLECharacteristic* characteristic);
    void handleDoorPositionWrite(BLECharacteristic* characteristic);
    void handleLEDStatusWrite(BLECharacteristic* characteristic);
    void handleLEDBrightnessWrite(BLECharacteristic* characteristic);
    void handleLEDColorWrite(BLECharacteristic* characteristic);
    void handleWiFiCredentialsWrite(BLECharacteristic* characteristic);
    void handleChildLockWrite(BLECharacteristic* characteristic);
    
    // Helper methods
    void onNetworkReceived(const std::string& value);
    void finalizeNetwork();
    
    // BLE characteristic update methods
    void updateDoorStatus(bool isOpen);
    void updateDoorPosition(uint8_t position);
    void updateLEDStatus(uint8_t ledState);
    void updateLEDBrightness(uint8_t brightness);
    void updateLEDColor(String color);
    void updateWiFiStatus(const String& status);
    void updateChildLock(bool childLockOn);
};

#endif // BLECONTROL_H