#include "BLEControl.h"
#include "MotorControl.h"
#include "LEDControl.h"

// BLE Server Callbacks Implementation
void BLEServerCallback::onConnect(BLEServer* pServer) {
    if (bleControl) {
        uint16_t clientId = pServer->getConnId();
        bleControl->handleClientConnect(clientId);
    }
}

void BLEServerCallback::onDisconnect(BLEServer* pServer) {
    if (bleControl) {
        bleControl->handleClientDisconnect();
    }
}

// BLE Characteristic Callbacks Implementation
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
    else if (uuid == UUID_LIGHTS_COLOR) {
        bleControl->handleLEDColorWrite(characteristic);
    }
    else if (uuid == UUID_WIFI_CREDENTIALS) {
        bleControl->handleWiFiCredentialsWrite(characteristic);
    }
    else if (uuid == UUID_CHILD_LOCK) {
        bleControl->handleChildLockWrite(characteristic);
    }
}

void BLECharacteristicCallback::onRead(BLECharacteristic* characteristic) {
    std::string uuid = characteristic->getUUID().toString();
    std::string value = characteristic->getValue();
    
    // Print the value read from the characteristic
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
    else if (uuid == UUID_LIGHTS_COLOR) {
        Serial.print("BLE Client read LED color: ");
        Serial.println(value.c_str());
    }
    else if (uuid == UUID_CHILD_LOCK) {
        Serial.print("BLE Client read child lock status: ");
        Serial.println(value.c_str());
    }
    else if (uuid == UUID_JSON_STATUS) {
        Serial.print("BLE Client read JSON status: ");
        Serial.println(value.c_str());
    }
}

// BLEControl Constructor
BLEControl::BLEControl(bool* podOpenFlag, WiFiControl* wifiControl, bool* childLock) 
    : pServer(nullptr), pAdvertising(nullptr), pDoorStatus(nullptr), pDoorPosition(nullptr), 
      pLEDStatus(nullptr), pLEDBrightness(nullptr), pLEDColor(nullptr), pWiFiCredentials(nullptr), 
      pWiFiStatus(nullptr), pChildLock(nullptr), pJSONStatus(nullptr), isClientConnected(false), connectedClientId(0),
      podOpenFlagRef(podOpenFlag), wifiControlRef(wifiControl), childLockRef(childLock), 
      networkBuffer(""), passwordBuffer(""), lastJSONUpdate(0) {
}

void BLEControl::begin() {
    Serial.println("Initializing BLE...");
    
    // Initialize BLE
    BLEDevice::init("Sole Pod");
    pServer = BLEDevice::createServer();
    
    // Set server callbacks for connection management
    pServer->setCallbacks(new BLEServerCallback(this));

    // Create service with more characteristics support
    static BLEUUID serviceUUID("7d840001-11eb-4c13-89f2-246b6e0b0000");
    BLEService* pService = pServer->createService(serviceUUID, 35, 0); // Increased for JSON characteristic

    // Create all characteristics
    createCharacteristics(pService);
    
    // Set initial values based on current states
    setInitialValues();

    // Start the service
    pService->start();

    // Get advertising instance and configure it
    pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(UUID_SERVICE);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // Functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    
    // Start advertising
    startAdvertising();

    Serial.println("BLE Control initialized and advertising started");
}

void BLEControl::createCharacteristics(BLEService* pService) {
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
    
    // Create LED Color Characteristic
    pLEDColor = pService->createCharacteristic(
        UUID_LIGHTS_COLOR,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ
    );
    pLEDColor->setCallbacks(new BLECharacteristicCallback(this, UUID_LIGHTS_COLOR));
    
    // Create WiFi Credentials Characteristic (write-only)
    pWiFiCredentials = pService->createCharacteristic(
        UUID_WIFI_CREDENTIALS, 
        BLECharacteristic::PROPERTY_WRITE
    );
    pWiFiCredentials->setCallbacks(new BLECharacteristicCallback(this, UUID_WIFI_CREDENTIALS));
    
    // Create WiFi Status Characteristic (read-only)
    pWiFiStatus = pService->createCharacteristic(
        UUID_WIFI_STATUS, 
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    
    // Create Child Lock Characteristic
    pChildLock = pService->createCharacteristic(
        UUID_CHILD_LOCK,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ
    );
    pChildLock->setCallbacks(new BLECharacteristicCallback(this, UUID_CHILD_LOCK));
    
    // Create JSON Status Characteristic (read-only with notify)
    pJSONStatus = pService->createCharacteristic(
        UUID_JSON_STATUS,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pJSONStatus->setCallbacks(new BLECharacteristicCallback(this, UUID_JSON_STATUS));
}

void BLEControl::setInitialValues() {
    // Set initial values based on current states
    updateDoorStatus(*podOpenFlagRef);
    updateDoorPosition(getDoorPosition());
    updateLEDStatus(getLEDState());
    
    // Convert internal brightness (0-255) to BLE brightness (0-100)
    uint8_t scaledBrightness = map(getLEDBrightness(), 0, 255, 0, 100);
    updateLEDBrightness(scaledBrightness);
    
    // Set initial color
    updateLEDColor(getLEDColor());
    
    // Set initial WiFi status
    updateWiFiStatus(wifiControlRef->getWiFiStatusString());

    // Set initial child lock status
    updateChildLock(*childLockRef);
    
    // Set initial JSON status
    updateJSONStatus();
}

// JSON Status Management
void BLEControl::updateJSONStatus() {
    if (!pJSONStatus) return;
    
    // Create JSON document
    StaticJsonDocument<512> jsonDoc;
    
    // Get current values from individual characteristics
    String doorStatusValue = String(pDoorStatus->getValue().c_str());
    String doorPositionValue = String(pDoorPosition->getValue().c_str());
    String ledStatusValue = String(pLEDStatus->getValue().c_str());
    String ledBrightnessValue = String(pLEDBrightness->getValue().c_str());
    String ledColorValue = String(pLEDColor->getValue().c_str());
    String wifiStatusValue = String(pWiFiStatus->getValue().c_str());
    String childLockValue = String(pChildLock->getValue().c_str());
    
    // Populate JSON document
    jsonDoc["door_status"] = doorStatusValue.toInt();
    jsonDoc["door_position"] = doorPositionValue.toInt();
    jsonDoc["led_status"] = ledStatusValue.toInt();
    jsonDoc["led_brightness"] = ledBrightnessValue.toInt();
    jsonDoc["led_color"] = ledColorValue;
    jsonDoc["wifi_status"] = wifiStatusValue;
    jsonDoc["child_lock"] = childLockValue.toInt();
    jsonDoc["timestamp"] = millis();  // Add timestamp for freshness
    
    // Serialize JSON to string
    char jsonBuffer[512];
    size_t jsonLength = serializeJson(jsonDoc, jsonBuffer);
    
    // Update the characteristic
    pJSONStatus->setValue(jsonBuffer);
    
    // Notify connected clients if any
    if (isClientConnected) {
        pJSONStatus->notify();
    }
    
    Serial.print("JSON Status updated: ");
    Serial.println(jsonBuffer);
}

void BLEControl::checkJSONUpdate() {
    unsigned long currentTime = millis();
    
    // Update JSON status at regular intervals
    if (currentTime - lastJSONUpdate >= JSON_UPDATE_INTERVAL) {
        updateJSONStatus();
        lastJSONUpdate = currentTime;
    }
}

void BLEControl::startAdvertising() {
    if (pAdvertising) {
        pAdvertising->start();
        Serial.println("BLE advertising started");
    }
}

void BLEControl::stopAdvertising() {
    if (pAdvertising) {
        pAdvertising->stop();
        Serial.println("BLE advertising stopped");
    }
}

void BLEControl::handleClientConnect(uint16_t clientId) {
    if (isClientConnected) {
        // Already have a client connected - disconnect the new one
        Serial.printf("BLE: New client attempted to connect, but client %d is already connected. Disconnecting new client.\n", connectedClientId);
        pServer->disconnect(clientId);
        return;
    }
    
    // Accept the new connection
    isClientConnected = true;
    connectedClientId = clientId;
    Serial.printf("BLE Client connected (ID: %d)\n", clientId);
    
    // Send initial JSON status to new client
    updateJSONStatus();
    
    // Optional: Stop advertising to save resources (since we only want one connection)
    // Uncomment the next line if you want to stop advertising when connected
    // stopAdvertising();
}

void BLEControl::handleClientDisconnect() {
    Serial.printf("BLE Client disconnected (ID: %d)\n", connectedClientId);
    
    // Reset connection state
    isClientConnected = false;
    connectedClientId = 0;
    
    // Restart advertising to allow new connections
    Serial.println("Restarting BLE advertising...");
    startAdvertising();
}

// All the existing characteristic handler methods remain the same
void BLEControl::handleDoorStatusWrite(BLECharacteristic* characteristic) {
    if (characteristic == pDoorStatus) {
        std::string value = characteristic->getValue();
        String doorStatus = String(value.c_str());
        
        if (doorStatus == "1") {
            Serial.println("BLE Command: Open Pod");
            *podOpenFlagRef = true;
        } 
        else if (doorStatus == "0") {
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
        String ledStatus = String(value.c_str());
        
        if (ledStatus == "1") {
            Serial.println("BLE Command: Turn LED ON");
            setLEDState(LED_STATE_ON);
        } 
        else if (ledStatus == "0") {
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
        
        int brightness = 0;
        try {
            brightness = std::stoi(value);
            
            if (brightness >= 0 && brightness <= 100) {
                Serial.print("BLE Command: Set LED Brightness to ");
                Serial.print(brightness);
                Serial.println("%");
                
                if (brightness == 0) {
                    // Brightness 0 means turn LED off
                    setLEDState(LED_STATE_OFF);
                    setLEDBrightness(0);  // Store the brightness value
                } else {
                    // Brightness > 0 means turn LED on with that brightness
                    setLEDState(LED_STATE_ON);
                    setLEDBrightness(brightness);
                }
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

void BLEControl::handleDoorPositionWrite(BLECharacteristic* characteristic) {
    if (characteristic == pDoorPosition) {
        std::string value = characteristic->getValue();
        
        int position = 0;
        try {
            position = std::stoi(value);
            
            if (position == 50 || position == 100) {
                Serial.print("BLE Command: Set Door Position to ");
                Serial.println(position);
                
                setDoorPosition(position);
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

void BLEControl::handleLEDColorWrite(BLECharacteristic* characteristic) {
    if (characteristic == pLEDColor) {
        std::string value = characteristic->getValue();
        String ledColor = String(value.c_str());
        
        Serial.print("BLE Command: Set LED Color to ");
        Serial.println(ledColor);
        
        setLEDColor(ledColor);
    }
}

void BLEControl::handleWiFiCredentialsWrite(BLECharacteristic* characteristic) {
    if (characteristic == pWiFiCredentials) {
        std::string value = characteristic->getValue();
        onNetworkReceived(value);
    }
}

void BLEControl::handleChildLockWrite(BLECharacteristic* characteristic) {
    if (characteristic == pChildLock) {
        std::string value = characteristic->getValue();
        String childLockStatus = String(value.c_str());
        
        if (childLockStatus == "1") {
            Serial.println("BLE Command: Enable Child Lock");
            *childLockRef = true;
        } 
        else if (childLockStatus == "0") {
            Serial.println("BLE Command: Disable Child Lock");
            *childLockRef = false;
        }
        else {
            Serial.println("Invalid Child Lock value received! Only 0 or 1 allowed.");
        }
    }
}

void BLEControl::onNetworkReceived(const std::string& value) {
    String data = String(value.c_str());
    int networkIndex = data.indexOf("ENDNETWORK");
    int passwordIndex = data.indexOf("ENDPASSWORD");
    if (networkIndex != -1 && passwordIndex != -1) {
        String ssid = data.substring(0, networkIndex);
        String password = data.substring(networkIndex + 10, passwordIndex);
        
        networkBuffer = ssid;
        passwordBuffer = password;
        Serial.printf("Received Network SSID: %s\n", ssid.c_str());
        Serial.printf("Received Password: %s\n", password.c_str());
        
        finalizeNetwork();
    } else {
        Serial.println("Invalid format, missing ENDNETWORK or ENDPASSWORD.");
    }
}

void BLEControl::finalizeNetwork() {
    if (networkBuffer.length() > 0 && passwordBuffer.length() > 0) {
        Serial.println("Attempting to connect to WiFi with new credentials...");
        
        bool connected = wifiControlRef->updateWiFiCredentials(networkBuffer, passwordBuffer);
        
        String status;
        if (connected) {
            status = "CONNECTED:" + networkBuffer + ":" + wifiControlRef->getLocalIP().toString();
            Serial.println("WiFi connection successful!");
        } else {
            status = "FAILED:" + networkBuffer;
            Serial.println("WiFi connection failed!");
        }
        
        updateWiFiStatus(status);
        
        networkBuffer = "";
        passwordBuffer = "";
    }
}

// All the update methods remain the same
void BLEControl::updateDoorStatus(bool isOpen) {
    if (pDoorStatus) {
        String status = isOpen ? "1" : "0";
        pDoorStatus->setValue(status.c_str());
        Serial.print("BLE Door Status updated: ");
        Serial.println(status);
    }
}

void BLEControl::updateLEDStatus(uint8_t ledState) {
    if (pLEDStatus) {
        String status = (ledState == LED_STATE_ON) ? "1" : "0";
        pLEDStatus->setValue(status.c_str());
        Serial.print("BLE LED Status updated: ");
        Serial.println(status);
    }
}

void BLEControl::updateLEDBrightness(uint8_t brightness) {
    if (brightness > MAX_BRIGHTNESS) {
        brightness = MAX_BRIGHTNESS;
    }
    
    if (pLEDBrightness) {
        String value = String(brightness);
        pLEDBrightness->setValue(value.c_str());
        Serial.print("BLE LED Brightness updated: ");
        Serial.print(brightness);
        Serial.println(" (0-100 scale)");
    }
}

void BLEControl::updateDoorPosition(uint8_t position) {
    if (position != 50 && position != 100) {
        position = 100;
    }
    
    if (pDoorPosition) {
        String value = String(position);
        pDoorPosition->setValue(value.c_str());
        Serial.print("BLE Door Position updated: ");
        Serial.println(position);
    }
}

void BLEControl::updateLEDColor(String color) {
    if (pLEDColor) {
        pLEDColor->setValue(color.c_str());
        Serial.print("BLE LED Color updated: ");
        Serial.println(color);
    }
}

void BLEControl::updateWiFiStatus(const String& status) {
    if (pWiFiStatus) {
        pWiFiStatus->setValue(status.c_str());
        Serial.print("BLE WiFi Status updated: ");
        Serial.println(status);
    }
}

void BLEControl::updateChildLock(bool childLockOn) {
    if (pChildLock) {
        String status = childLockOn ? "1" : "0";
        pChildLock->setValue(status.c_str());
        Serial.print("BLE Child Lock updated: ");
        Serial.println(childLockOn ? "ENABLED" : "DISABLED");
    }
}