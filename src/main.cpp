/*
Sole Pod Control System
Hardware compatibility:
- Control Board v1.4
- LED Board v1.2
- Hall Sensor Board v1.0

Description:
This firmware controls the Sole Pod system, managing door motors,
tray motors, LED lighting, safety sensors, and BLE connectivity.
*/

#include <Arduino.h>
#include "MotorControl.h"
#include "Sensors.h"
#include "LEDControl.h"
#include "VoltageReader.h"
#include "BLEControl.h" 
#include "SystemSettings.h"
#include "WiFiControl.h"
#include "aws_service.h"
#include "nvs_flash.h"
#include "FS.h"
#include "SPIFFS.h"

#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// Configuration settings
#define DEBUG_MODE true       // Enable/disable debug messages
#define LOOP_DELAY_MS 10      // Main loop delay in milliseconds

// System state flags
bool childLockOn = false;     // Child lock status (true = locked)
bool podOpenFlag = false;     // Pod position flag (true = open, false = closed)

// SSID and password to store
const char* defaultSSID = "";
const char* defaultPassword = "";

//static bool bleActive = false;
static bool fleetProvisioningDone = false;
String thingName, deviceCert, deviceKey;
static bool systemInitialized = false;

// Create WiFi controller instance
WiFiControl wifiControl;

// Update BLEControl instantiation to include child lock reference
BLEControl bleControl(&podOpenFlag, &wifiControl, &childLockOn);

// Function prototypes
void setupSystem();
void runDoorControl();
void runLEDControl();
void runWiFiControl();  // New function for WiFi monitoring
void runChildLockControl(); // New function for child lock state monitoring
void printDebugInfo();

bool hasProvisionedCerts() {
    deviceCert = readStringFromNVS(NVS_NAMESPACE, "deviceCert");
    deviceKey = readStringFromNVS(NVS_NAMESPACE, "deviceKey");
    thingName  = readStringFromNVS(NVS_NAMESPACE, "thingName");

    return (!deviceCert.isEmpty() && !deviceKey.isEmpty() && !thingName.isEmpty());
}

bool AWSconnection() {
    thingName = readStringFromNVS(NVS_NAMESPACE, "thingName");
    deviceCert = readStringFromNVS(NVS_NAMESPACE, "deviceCert");
    deviceKey = readStringFromNVS(NVS_NAMESPACE, "deviceKey");
    root_ca = readFileToString("/root_ca.pem");

    if (deviceCert.length() == 0 || deviceKey.length() == 0 || root_ca.length() == 0) {
        Serial.println("\n Missing cert, key or root CA");
        return false;
    }

    Serial.println("\n Thing Name:");
    Serial.println(thingName);
    Serial.println("\n Device Cert:");
    Serial.println(deviceCert);
    Serial.println("\n Device Key:");
    Serial.println(deviceKey);
    Serial.println("\n Root CA:");
    Serial.println(root_ca);

    net.setCACert(root_ca.c_str());
    net.setCertificate(deviceCert.c_str());
    net.setPrivateKey(deviceKey.c_str());

    client.setServer(aws_endpoint, 8883);
    client.setBufferSize(4096);
    client.setCallback(mqttCallback);
    
    if (!client.connect(thingName.c_str())) {
        Serial.println("\n Failed to connect MQTT.");
        Serial.println(client.state());
        return false;
    }

    Serial.println("\n Connected MQTT with new cert.");
    client.subscribe("testat/topic");
    const char* jsonPayload = "{\"message\":\"Hello from device\"}";

    String shadowDeltaTopic = String("$aws/things/") + thingName + "/shadow/update/delta";
    client.subscribe(shadowDeltaTopic.c_str());
    Serial.printf("Subscribed to Shadow Delta topic: %s\n", shadowDeltaTopic.c_str());

    bool pubSuccess = client.publish("testat/topic", jsonPayload);
    if (pubSuccess) {
        Serial.println("\n Published JSON message to testat/topic");
    } else {
        Serial.println("\n Failed to publish JSON message");
    }

    if (!systemInitialized) {
        setupSystem();
        systemInitialized = true;
    }

    publishReportedState("sneaker_pot_12345678", led_state, led_brightness, led_color, podOpenFlag, childLockOn);

    return true;
}


void ResetChildLock() {
    if (WiFi.status() != WL_CONNECTED && fleetProvisioningDone) {
        if (childLockOn) { 
            Serial.println("WiFi lost connection. Disabling child lock.");
            childLockOn = false;
            saveChildLockState(childLockOn);  
        }
    }
}

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    bleControl.begin();
    //bleActive = true;
    // Wait for serial connection when in debug mode
    if (DEBUG_MODE) {
        delay(1000);  // Give time for serial monitor to connect
        Serial.println("Sole Pod System Starting...");
    }
    
    if (!SPIFFS.begin(true)) {
        Serial.println("\n SPIFFS Mount Failed");
    
    }
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        Serial.println("\n NVS init failed. Erasing and reinitializing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    Serial.println("\n NVS initialized.");
    
    // Start WiFi connection process without waiting
    if (DEBUG_MODE) {
        Serial.println("Starting WiFi connection in background...");
    }
    
    // Use the non-blocking connection method
    if (wifiControl.loadCredentialsFromNVS()) {
            Serial.println("Connecting to saved WiFi credentials...");
            wifiControl.beginConnection(wifiControl.getCurrentSSID(), wifiControl.getCurrentPassword());
        } else {
            // Nếu chưa có, dùng default (hoặc chờ BLE provisioning)
            wifiControl.beginConnection(defaultSSID, defaultPassword);
        }   
    
    if (DEBUG_MODE) {
        Serial.println("System initialization complete!");
        Serial.print("Motor stall detection threshold set to: ");
        Serial.println(STALL_VOLTAGE_THRESHOLD);
    }
}

void loop() {

    if (wifiControl.getWiFiStatus() == WL_CONNECTED && !fleetProvisioningDone) {
        if (hasProvisionedCerts()) {
            Serial.println("\n Certificate is already in NVS. Connecting to AWS...");
            if (AWSconnection()) {
                Serial.println("\n Connected to AWS successfully!");
                fleetProvisioningDone = true;
            } else {
                Serial.println("\n Failed to connect to AWS. Will retry later.");
            }        
        } else {
            Serial.println("\n No certificate found. Performing Fleet Provisioning...");
            if (runFleetProvisioning()) {
                Serial.println("\n Fleet provisioning done!");
                delay(1000);
                ESP.restart(); // Khởi động lại để dùng cert mới
            } else {
                Serial.println("\n Fleet provisioning failed!");
            }
        }
        //fleetProvisioningDone = true;
    }
    
    ResetChildLock();
    
    client.loop();
    // Run safety checks
    //runSafetyChecks();
    
    if (systemInitialized) {
        // Handle door control functionality
        runDoorControl();
        
        // Handle LED control functionality
        runLEDControl();
    }
    
    // Handle WiFi connection monitoring
    runWiFiControl();
    
    // Handle child lock state monitoring
    //runChildLockControl();
    
    // Check and update JSON status characteristic (NEW)
    //bleControl.checkJSONUpdate();
    
    // Print debug information if enabled
    if (DEBUG_MODE) {
        printDebugInfo();
    }
    
    // Small delay to stabilize loop timing
    delay(LOOP_DELAY_MS);
}

void runWiFiControl() {
    static unsigned long lastWiFiCheckTime = 0;
    static String lastWiFiStatus = "";
    static bool initialConnectionAttempt = true;
    static unsigned long connectionStartTime = 0;
    static int reconnectAttempts = 0;          
    const int MAX_RECONNECT_ATTEMPTS = 3;
    const unsigned long WIFI_CHECK_INTERVAL = 5000; // Check more frequently during initial connection
    const unsigned long CONNECTION_TIMEOUT = 10000; // 10 second timeout
    
    unsigned long currentTime = millis();
    
    // Handle initial connection attempt
    if (initialConnectionAttempt) {
        if (connectionStartTime == 0) {
            connectionStartTime = currentTime;
        }
        
        // Check if connected
        if (wifiControl.getWiFiStatus() == WL_CONNECTED) {
            initialConnectionAttempt = false;
            reconnectAttempts = 0;
            if (DEBUG_MODE) {
                Serial.println("\n WiFi connected successfully in background!");
                Serial.printf("Network: %s\n", wifiControl.getCurrentSSID().c_str());
                Serial.printf("IP Address: %s\n", wifiControl.getLocalIP().toString().c_str());
            }
        }
        // Check for timeout
        else if (currentTime - connectionStartTime > CONNECTION_TIMEOUT) {
            initialConnectionAttempt = false;
            if (DEBUG_MODE) {
                Serial.println("\n Initial WiFi connection attempt timed out.");
            }
        }
    }
    
    // Regular WiFi status check (less frequent after initial setup)
    if (currentTime - lastWiFiCheckTime >= WIFI_CHECK_INTERVAL) {
        String currentStatus;

        if (wifiControl.getWiFiStatus() == WL_CONNECTED) {
            currentStatus = "CONNECTED:" + wifiControl.getCurrentSSID() + ":" +
                            wifiControl.getLocalIP().toString();
            reconnectAttempts = 0; 
            
        } else {
            currentStatus = "\n DISCONNECTED";
            
            // Nếu không phải initial attempt và có credentials
            if (!initialConnectionAttempt && wifiControl.getCurrentSSID().length() > 0) {
                if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
                    if (DEBUG_MODE) {
                        Serial.printf("WiFi connection lost! Attempting reconnect %d/%d...\n",
                                      reconnectAttempts + 1, MAX_RECONNECT_ATTEMPTS);
                    }
                    wifiControl.beginConnection(wifiControl.getCurrentSSID(), wifiControl.getCurrentPassword());
                    reconnectAttempts++;
                } else {
                    // Quá số lần thử → xóa WiFi credentials trong NVS
                    if (DEBUG_MODE) {
                        Serial.println("Max reconnect attempts reached. Clearing saved WiFi credentials.");
                    }
                    wifiControl.clearCredentialsFromNVS();  // Hàm xóa NVS (cần thêm vào WiFiControl)
                    //wifiControl.beginConnection("", "");     // Ngắt kết nối, chờ BLE provisioning mới
                    reconnectAttempts = 0;
                }
            }
        }
        
        // Chỉ cập nhật BLE nếu trạng thái thay đổi
        if (lastWiFiStatus != currentStatus) {
            bleControl.updateWiFiStatus(currentStatus);
            lastWiFiStatus = currentStatus;
        }
        
        lastWiFiCheckTime = currentTime;
    }
}

// void runChildLockControl() {
//     static bool prevChildLockState = false; // Track previous child lock state
//     static bool firstRun = true; // Flag to force first update
    
//     // Update BLE child lock status if the state has changed or first run
//     if (prevChildLockState != childLockOn || firstRun) {
//         // Update BLE characteristic
//         bleControl.updateChildLock(childLockOn);
        
//         // Save child lock state to persistent storage
//         saveChildLockState(childLockOn);
        
//         // Update previous state for next iteration
//         prevChildLockState = childLockOn;
//         firstRun = false;
//     }
// }

void setupSystem() {
    // Initialize sensors
    initSwitches();
    
    // Initialize motor control
    initMotors();
    
    // Initialize voltage monitoring
    initVoltageReader();
    
    // Initialize settings module
    initSettings();
    
    // Load saved settings including child lock state
    String savedLedColor;
    uint8_t savedLedBrightness;
    uint8_t savedDoorPosition;
    uint8_t savedLedState;
    bool savedDoorStatus;
    bool savedChildLock;
    
    if (loadAllSettings(savedLedColor, savedLedBrightness, savedDoorPosition, 
                       savedLedState, savedDoorStatus, savedChildLock)) {
        // Apply loaded settings to the global variables
        ledColorHex = savedLedColor;
        ledBrightness = savedLedBrightness;
        doorPosition = savedDoorPosition;
        // No need to set lightState here - we'll call setLEDState after initialization
        podOpenFlag = savedDoorStatus; // Set the door status based on saved value
        childLockOn = savedChildLock; // Set the child lock status based on saved value
    }
    
    // Initialize LED control
    initLEDs();
    
    // Explicitly set the LED state based on saved setting (defaults to OFF)
    setLEDState(savedLedState);
    
    // Initialize BLE control
    // if (wifiControl.getWiFiStatus() != WL_CONNECTED) {
    //         bleControl.begin();
    //         bleActive = true;
    //         Serial.println("\n BLE started!");
    // } else {
    //         bleActive = false;
    //         Serial.println("\n WiFi already connected - BLE disabled.");
    // }
}

// Handle door related functionality
void runDoorControl() {
    static uint8_t prevState = POD_STATE_UNDEFINED;
    static bool prevOpenFlag = false;
    static uint8_t prevDoorPosition = 0; 
    
    // Process door button input
    handleDoorButton(podOpenFlag, childLockOn);
    manageMotors(podOpenFlag);
    
    // Read current door state
    uint8_t currentState = readState();
    uint8_t currentDoorPosition = getDoorPosition();
    
    // Update BLE door status if:
    // 1. The physical door state has changed OR
    // 2. The target state (podOpenFlag) has changed
    if (prevState != currentState || prevOpenFlag != podOpenFlag) {
        // Determine if door is considered "open" for BLE status
        // Door is "open" when:
        // - It's physically open (POD_STATE_DOOR_OPEN or further)
        // - OR it's in transition and the target is "open" (podOpenFlag = true)
        bool doorIsOpenForBLE = false;
        
        if (currentState == POD_STATE_DOOR_OPEN || 
            currentState == POD_STATE_TRAY_MIDWAY || 
            currentState == POD_STATE_OPEN) {
            // Door is physically open or further in open state
            doorIsOpenForBLE = true;
        } 
        else if (currentState == POD_STATE_DOOR_MIDWAY) {
            // Door is in transition - use the target state
            doorIsOpenForBLE = podOpenFlag;
        }
        
        // Update BLE status
        bleControl.updateDoorStatus(doorIsOpenForBLE);
        
        // Save door status if the flag has changed
        if (prevOpenFlag != podOpenFlag) {
            saveDoorStatus(podOpenFlag);
        }
        
        // Update previous values for next iteration
        prevState = currentState;
        prevOpenFlag = podOpenFlag;
    }

    // Update BLE door position if it has changed
    if (prevDoorPosition != currentDoorPosition) {
        bleControl.updateDoorPosition(currentDoorPosition);
        prevDoorPosition = currentDoorPosition;
    }
}

void runLEDControl() {
    static uint8_t prevLEDState = 255; // Initialize with an invalid state to force first update
    static uint8_t prevLEDBrightness = 101; // Track previous brightness
    static String prevLEDColor = ""; // Track previous color
    
    // Process LED button input
    handleLEDButton(childLockOn);
    
    // Read current LED state, brightness, and color
    uint8_t currentLEDState = getLEDState();
    uint8_t currentLEDBrightness = getLEDBrightness();
    String currentLEDColor = getLEDColor();
    
    // Update BLE LED status if the LED state has changed
    if (prevLEDState != currentLEDState) {
        // Update BLE
        bleControl.updateLEDStatus(currentLEDState);
        
        // Update previous state for next iteration
        prevLEDState = currentLEDState;
    }
    
    // Update BLE LED brightness if the brightness has changed
    if (prevLEDBrightness != currentLEDBrightness) {
        // Brightness is already in 0-100 range, no scaling needed
        bleControl.updateLEDBrightness(currentLEDBrightness);
        
        // Update previous brightness for next iteration
        prevLEDBrightness = currentLEDBrightness;
    }
    
    // Update BLE LED color if the color has changed
    if (prevLEDColor != currentLEDColor) {
        // Update BLE
        bleControl.updateLEDColor(currentLEDColor);
        
        // Update previous color for next iteration
        prevLEDColor = currentLEDColor;
    }
}

// Print debug information to serial console
void printDebugInfo() {
    static unsigned long lastDebugTime = 0;
    static bool lastBLEConnectionStatus = false;
    bool currentBLEStatus = bleControl.getConnectionStatus(); // Get BLE status
    
    // Only update debug info every 500ms to avoid flooding serial
    if (millis() - lastDebugTime > 5000) {
        uint8_t currentState = readState();
        float voltage = readAverageVoltage();
        
        if (wifiControl.getWiFiStatus() == WL_CONNECTED) {
            Serial.println("--- System Status ---");
            
            Serial.print("Pod State: ");
            Serial.print(getStateDescription(currentState));
            Serial.print(" (");
            Serial.print(currentState);
            Serial.println(")");
            Serial.print("LED State: ");
            Serial.print(getLEDState() == LED_STATE_ON ? "ON" : "OFF");
            Serial.print("LED Brightness: ");
            Serial.println(getLEDBrightness());
            Serial.println(")");
            Serial.print("LED Color: ");
            Serial.println(getLEDColor());
            Serial.print("Target: ");
            Serial.println(podOpenFlag ? "OPENING/OPEN" : "CLOSING/CLOSED");
            Serial.print("Child Lock: ");
            Serial.println(childLockOn ? "ENABLED" : "DISABLED");
            Serial.print("Door Position: ");
            Serial.println(getDoorPosition());
            Serial.print("LED State: ");
            Serial.println(getLEDState() == LED_STATE_ON ? "ON" : "OFF");
            Serial.print("LED Brightness: ");
            Serial.println(getLEDBrightness());
            
            // Print individual sensor states
            Serial.println("Sensors:");
            Serial.print("  Door Closed: ");
            Serial.println(isDoorClosed() ? "ACTIVE" : "INACTIVE");
            Serial.print("  Door Open: ");
            Serial.println(isDoorOpen() ? "ACTIVE" : "INACTIVE");
            Serial.print("  Tray Closed: ");
            Serial.println(isTrayClose() ? "ACTIVE" : "INACTIVE");
            Serial.print("  Tray Open: ");
            Serial.println(isTrayOpen() ? "ACTIVE" : "INACTIVE");
        }

        // Add WiFi status to debug info
        Serial.print("WiFi Status: ");
        Serial.println(wifiControl.getWiFiStatusString());
        if (wifiControl.getWiFiStatus() == WL_CONNECTED) {
            Serial.print("Network: ");
            Serial.println(wifiControl.getCurrentSSID());
            Serial.print("IP Address: ");
            Serial.println(wifiControl.getLocalIP().toString());
            Serial.print("Signal Strength: ");
            Serial.print(wifiControl.getSignalStrength());
            Serial.println(" dBm");
        }

        // Detect BLE connection changes
        if (lastBLEConnectionStatus != currentBLEStatus) {
            if (currentBLEStatus) {
                Serial.println("*** BLE CLIENT CONNECTED ***");
            } else {
                Serial.println("*** BLE CLIENT DISCONNECTED - ADVERTISING RESUMED ***");
            }
            lastBLEConnectionStatus = currentBLEStatus;
        }
        
        Serial.println("-------------------");
        
        lastDebugTime = millis();
    }
}