#ifndef WIFI_CONTROL_H
#define WIFI_CONTROL_H

#include <WiFi.h>

class WiFiControl {
private:
    String ssid;
    String password;
    String previousSSID;
    String previousPassword;
    const unsigned long connectionTimeout = 30000; // Timeout in milliseconds (30 seconds)

public:
    WiFiControl() : ssid(""), password(""), previousSSID(""), previousPassword("") {}
    
    // Returns the current WiFi status as defined by the WiFi.h library
    int getWiFiStatus() {
        return WiFi.status();
    }
    
    // Returns a readable string representation of the WiFi status
    String getWiFiStatusString() {
        switch (WiFi.status()) {
            case WL_CONNECTED:
                return "Connected";
            case WL_IDLE_STATUS:
                return "Idle";
            case WL_NO_SSID_AVAIL:
                return "SSID not available";
            case WL_SCAN_COMPLETED:
                return "Scan completed";
            case WL_CONNECT_FAILED:
                return "Connection failed";
            case WL_CONNECTION_LOST:
                return "Connection lost";
            case WL_DISCONNECTED:
                return "Disconnected";
            default:
                return "Unknown status";
        }
    }

    // Begins the WiFi connection process without waiting for it to complete
    void beginConnection(const String& newSSID, const String& newPassword) {
        // Update stored credentials
        ssid = newSSID;
        password = newPassword;
        
        // Start connection attempt without waiting
        Serial.printf("Starting WiFi connection to: %s\n", ssid.c_str());
        WiFi.begin(ssid.c_str(), password.c_str());
    }
    
    // Attempts to connect to WiFi with the provided credentials
    // Returns true if connection was successful, false otherwise
    bool connectWiFi(const String& newSSID, const String& newPassword) {
        // Update stored credentials
        ssid = newSSID;
        password = newPassword;
        
        // Attempt to connect
        Serial.printf("Connecting to WiFi network: %s\n", ssid.c_str());
        WiFi.begin(ssid.c_str(), password.c_str());
        
        // Wait for connection with timeout
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - startTime > connectionTimeout) {
                Serial.println("Connection timed out!");
                return false;
            }
            delay(500);
            Serial.print(".");
        }
        
        Serial.println("\nConnected to WiFi!");
        Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
        return true;
    }
    
    // Updates WiFi credentials and attempts to connect
    // If connection fails, reverts to previous network if it was connected
    bool updateWiFiCredentials(const String& newSSID, const String& newPassword) {
        // Store current credentials as previous credentials
        previousSSID = ssid;
        previousPassword = password;
        
        // If currently connected, remember this
        bool wasConnected = (WiFi.status() == WL_CONNECTED);
        
        // Attempt to connect with new credentials
        if (connectWiFi(newSSID, newPassword)) {
            return true;
        }
        
        // If connection failed and was previously connected, try to reconnect to previous network
        if (wasConnected && !previousSSID.isEmpty() && !previousPassword.isEmpty()) {
            Serial.println("Failed to connect with new credentials. Reverting to previous network...");
            // Restore original credentials
            ssid = previousSSID;
            password = previousPassword;
            return connectWiFi(ssid, password);
        }
        
        return false;
    }
    
    // Disconnects from WiFi
    void disconnectWiFi() {
        if (WiFi.status() == WL_CONNECTED) {
            WiFi.disconnect();
            Serial.println("Disconnected from WiFi");
        } else {
            Serial.println("Not connected to WiFi");
        }
    }
    
    // Get the current SSID
    String getCurrentSSID() const {
        return ssid;
    }
    
    // Get the current password
    String getCurrentPassword() const {
        return password;
    }
    
    // Get the local IP address if connected
    IPAddress getLocalIP() {
        return WiFi.localIP();
    }
    
    // Get signal strength if connected
    int getSignalStrength() {
        return WiFi.RSSI();
    }
};

#endif // WIFI_CONTROL_H