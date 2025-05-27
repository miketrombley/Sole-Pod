/*
 * AWS MQTT Handler
 * Handles MQTT communications with AWS IoT Core
 */

 #ifndef AWS_MQTT_HANDLER_H
 #define AWS_MQTT_HANDLER_H
 
 #include <Arduino.h>
 #include <WiFiClientSecure.h>
 #include <PubSubClient.h>
 #include <ArduinoJson.h>
 #include "aws_config.h"
 
 class AwsMqttHandler {
   public:
     AwsMqttHandler();
     
     // Initialize the MQTT client
     void begin();
     
     // Connect to AWS IoT
     bool connect();
     
     // Reconnect if connection is lost
     bool reconnect();
     
     // Check if connected to AWS IoT
     bool isConnected();
     
     // Process MQTT messages
     void loop();
     
     // Publish a message to AWS IoT
     bool publish(const char* topic, const char* payload);
     
     // Publish device status to AWS IoT (all parameters)
     bool publishDeviceStatus(bool podStatus, bool ledStatus, String ledColor, int ledBrightness);
     
     // Publish individual status values to AWS IoT
     bool publishPodStatus(bool podStatus);
     bool publishLedStatus(bool ledStatus);
     bool publishLedColor(String ledColor);
     bool publishLedBrightness(int ledBrightness);
     
     // Set callback for incoming messages
     void setCallback(void (*callback)(char*, byte*, unsigned int));
     
   private:
     WiFiClientSecure wifiClient;
     PubSubClient mqttClient;
     unsigned long lastReconnectAttempt = 0;
     const long reconnectInterval = 5000; // 5 seconds
 };
 
 #endif // AWS_MQTT_HANDLER_H