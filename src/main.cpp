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
#include <FastLED.h>
#include "MotorControl.h"
#include "Sensors.h"
#include "LEDControl.h"
#include "VoltageReader.h"
#include "BLEControl.h" 
#include "SystemSettings.h"
#include "WiFiControl.h"

// Configuration settings
#define DEBUG_MODE true       // Enable/disable debug messages
#define LOOP_DELAY_MS 10      // Main loop delay in milliseconds

// System state flags
bool childLockOn = false;     // Child lock status (true = locked)
bool podOpenFlag = false;     // Pod position flag (true = open, false = closed)

// SSID and password to store
const char* defaultSSID = "";
const char* defaultPassword = "";

// Create WiFi controller instance
WiFiControl wifiControl;

// Update BLEControl instantiation
BLEControl bleControl(&podOpenFlag, &wifiControl);

// Function prototypes
void setupSystem();
void runDoorControl();
void runLEDControl();
void runWiFiControl();  // New function for WiFi monitoring
void printDebugInfo();

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    // Wait for serial connection when in debug mode
    if (DEBUG_MODE) {
        delay(1000);  // Give time for serial monitor to connect
        Serial.println("Sole Pod System Starting...");
    }
    
    // Initialize all subsystems
    setupSystem();
    
    // Initialize WiFi connection with stored credentials
    if (DEBUG_MODE) {
        Serial.println("Initializing WiFi connection...");
    }
    
    if (wifiControl.connectWiFi(defaultSSID, defaultPassword)) {
        if (DEBUG_MODE) {
            Serial.println("Successfully connected to WiFi!");
            Serial.printf("Network: %s\n", wifiControl.getCurrentSSID().c_str());
            Serial.printf("IP Address: %s\n", wifiControl.getLocalIP().toString().c_str());
            Serial.printf("Signal Strength: %d dBm\n", wifiControl.getSignalStrength());
        }
    } else if (DEBUG_MODE) {
        Serial.println("Failed to connect to WiFi. Check credentials or network availability.");
    }
    
    if (DEBUG_MODE) {
        Serial.println("System initialization complete!");
        Serial.print("Motor stall detection threshold set to: ");
        Serial.println(STALL_VOLTAGE_THRESHOLD);
    }
}

void loop() {
    // Run safety checks
    //runSafetyChecks();
    
    // Handle door control functionality
    runDoorControl();
    
    // Handle LED control functionality
    runLEDControl();
    
    // Handle WiFi connection monitoring
    runWiFiControl();
    
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
    const unsigned long WIFI_CHECK_INTERVAL = 30000; // Check every 30 seconds
    
    // Only check periodically to avoid constant checking
    if (millis() - lastWiFiCheckTime >= WIFI_CHECK_INTERVAL) {
        String currentStatus;
        
        if (wifiControl.getWiFiStatus() == WL_CONNECTED) {
            currentStatus = "CONNECTED:" + wifiControl.getCurrentSSID() + ":" + 
                           wifiControl.getLocalIP().toString();
        } else {
            currentStatus = "DISCONNECTED";
            
            if (wifiControl.getCurrentSSID().length() > 0) {
                // Try to reconnect if we have credentials
                if (DEBUG_MODE) {
                    Serial.println("WiFi connection lost! Attempting to reconnect...");
                }
                wifiControl.connectWiFi(wifiControl.getCurrentSSID(), wifiControl.getCurrentPassword());
            }
        }
        
        // Only update BLE characteristic if status changed
        if (lastWiFiStatus != currentStatus) {
            bleControl.updateWiFiStatus(currentStatus);
            lastWiFiStatus = currentStatus;
        }
        
        lastWiFiCheckTime = millis();
    }
}

void setupSystem() {
    // Initialize sensors
    initSwitches();
    
    // Initialize motor control
    initMotors();
    
    // Initialize voltage monitoring
    initVoltageReader();
    
    // Initialize settings module
    initSettings();
    
    // Load saved settings
    String savedLedColor;
    uint8_t savedLedBrightness;
    uint8_t savedDoorPosition;
    uint8_t savedLedState;
    bool savedDoorStatus;
    
    if (loadAllSettings(savedLedColor, savedLedBrightness, savedDoorPosition, 
                       savedLedState, savedDoorStatus)) {
        // Apply loaded settings to the global variables
        ledColorHex = savedLedColor;
        ledBrightness = savedLedBrightness;
        doorPosition = savedDoorPosition;
        // No need to set lightState here - we'll call setLEDState after initialization
        podOpenFlag = savedDoorStatus; // Set the door status based on saved value
    }
    
    // Initialize LED control
    initLEDs();
    
    // Explicitly set the LED state based on saved setting (defaults to OFF)
    setLEDState(savedLedState);
    
    // Initialize BLE control
    bleControl.begin();
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
    
    // Only update debug info every 500ms to avoid flooding serial
    if (millis() - lastDebugTime > 1000) {
        uint8_t currentState = readState();
        float voltage = readAverageVoltage();
        
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
        Serial.println(childLockOn ? "ON" : "OFF");
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
        
        Serial.println("-------------------");
        
        lastDebugTime = millis();
    }
}