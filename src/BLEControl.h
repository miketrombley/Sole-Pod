#ifndef BLECONTROL_H
#define BLECONTROL_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLECharacteristic.h>
#include <BLEService.h>
#include <BLEAdvertising.h>
#include "WiFiControl.h" // Add this include

// Define Service and Characteristic UUIDs
#define UUID_SERVICE           "7d840001-11eb-4c13-89f2-246b6e0b0000"
#define UUID_DOOR_STATUS       "7d840002-11eb-4c13-89f2-246b6e0b0001"
#define UUID_DOOR_POSITION     "7d840003-11eb-4c13-89f2-246b6e0b0002"
#define UUID_LIGHTS            "7d840004-11eb-4c13-89f2-246b6e0b0003"
#define UUID_LIGHTS_BRIGHTNESS "7d840005-11eb-4c13-89f2-246b6e0b0004"
#define UUID_LIGHTS_COLOR      "7d840006-11eb-4c13-89f2-246b6e0b0005"
#define UUID_WIFI_CREDENTIALS  "7d840007-11eb-4c13-89f2-246b6e0b0006"
#define UUID_WIFI_STATUS       "7d840008-11eb-4c13-89f2-246b6e0b0007"

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

class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer);
    void onDisconnect(BLEServer* pServer);
};

class BLEControl {
private:
    BLEServer* pServer;
    BLECharacteristic* pDoorStatus;
    BLECharacteristic* pDoorPosition;
    BLECharacteristic* pLEDStatus;
    BLECharacteristic* pLEDBrightness;
    BLECharacteristic* pLEDColor;
    BLECharacteristic* pWiFiCredentials;
    BLECharacteristic* pWiFiStatus;
    
    // Reference to external state flag to control door state
    bool* podOpenFlagRef;
    
    // Reference to WiFi controller
    WiFiControl* wifiControlRef;
    
    // Network credentials buffers
    String networkBuffer;
    String passwordBuffer;

public:
    // Updated constructor to include WiFiControl reference
    BLEControl(bool* podOpenFlag, WiFiControl* wifiControl);
    void begin();
    
    // Handler for door status characteristic changes
    void handleDoorStatusWrite(BLECharacteristic* characteristic);

    // Handler for door position characteristic changes
    void handleDoorPositionWrite(BLECharacteristic* characteristic);
    
    // Handler for LED status characteristic changes
    void handleLEDStatusWrite(BLECharacteristic* characteristic);
    
    // Handler for LED brightness characteristic changes
    void handleLEDBrightnessWrite(BLECharacteristic* characteristic);
    
    // Handler for LED color characteristic changes
    void handleLEDColorWrite(BLECharacteristic* characteristic);
    
    // Handler for WiFi credentials characteristic changes
    void handleWiFiCredentialsWrite(BLECharacteristic* characteristic);
    
    // Helper method to process received network info
    void onNetworkReceived(const std::string& value);
    
    // Method to attempt WiFi connection with stored credentials
    void finalizeNetwork();
    
    // Updates the BLE characteristic based on the current door state
    void updateDoorStatus(bool isOpen);

    // Updates the BLE characteristic based on the current door position
    void updateDoorPosition(uint8_t position);
    
    // Updates the BLE characteristic based on the current LED state
    void updateLEDStatus(uint8_t ledState);
    
    // Updates the BLE characteristic based on the current LED brightness
    void updateLEDBrightness(uint8_t brightness);
    
    // Updates the BLE characteristic based on the current LED color
    void updateLEDColor(String color);
    
    // Updates the BLE characteristic with current WiFi status
    void updateWiFiStatus(const String& status);

    // Updates the BLE characteristic with current WiFi signal strength
    void onConnect(BLEServer* pServer);
    void onDisconnect(BLEServer* pServer);
    void restartAdvertising();
};

#endif // BLECONTROL_H