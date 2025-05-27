/*
 * AWS MQTT Handler
 * Implementation file for AWS MQTT communications
 */

 #include "AwsMqttHandler.h"

// Callback function for receiving MQTT messages (defined in main.cpp)
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  // Create a buffer for the payload
  char message[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  
  Serial.println(message);
  
  // Parse the JSON payload
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  // Check for parsing errors
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }
  
  // Check if this is a shadow update
  if (doc.containsKey("state") && doc["state"].containsKey("desired")) {
    // Extract the desired state
    JsonObject desired = doc["state"]["desired"];
    
    // Update device status based on received shadow with the new token names
    if (desired.containsKey("is_open")) {
      Serial.println("is_open:" + String(desired["is_open"].as<bool>()));      
    }
    
    if (desired.containsKey("nightlight")) {
      Serial.println("nightlight:" + String(desired["nightlight"].as<bool>()));     
    }
    
    if (desired.containsKey("color")) {
      Serial.println("color:" + String(desired["color"].as<String>()));      
    }
    
    if (desired.containsKey("nightlight_brightness")) {
      Serial.println("nightlight_brightness:" + String(desired["nightlight_brightness"].as<int>())); 
    }
  }
}
 
 // Constructor
 AwsMqttHandler::AwsMqttHandler() : mqttClient(wifiClient) {
 }
 
 // Initialize the MQTT client
 void AwsMqttHandler::begin() {
   // Configure WiFiClientSecure to use the AWS certificates
   wifiClient.setCACert(AWS_CERT_CA);
   wifiClient.setCertificate(AWS_CERT_CRT);
   wifiClient.setPrivateKey(AWS_CERT_PRIVATE);
   
   // Configure MQTT client
   mqttClient.setServer(AWS_IOT_ENDPOINT, 8883);
   
   // Set default callback
   mqttClient.setCallback(mqttCallback);
   
   Serial.println("AWS MQTT Handler initialized");
 }
 
 // Connect to AWS IoT
 bool AwsMqttHandler::connect() {
   Serial.print("Connecting to AWS IoT Core...");
   
   // Create a random client ID
   String clientId = "ESP32-";
   clientId += String(random(0xffff), HEX);
   
   // Connect to the MQTT broker on AWS
   if (mqttClient.connect(clientId.c_str())) {
     Serial.println("Connected to AWS IoT!");
     
     // Subscribe to the desired topic
     if (mqttClient.subscribe(AWS_IOT_SUBSCRIBE_TOPIC)) {
       Serial.print("Subscribed to: ");
       Serial.println(AWS_IOT_SUBSCRIBE_TOPIC);
     } else {
       Serial.println("Failed to subscribe to topic");
       return false;
     }
     
     return true;
   } else {
     Serial.print("Failed to connect to AWS IoT, rc=");
     Serial.println(mqttClient.state());
     return false;
   }
 }
 
 // Reconnect if connection is lost
 bool AwsMqttHandler::reconnect() {
   // Check if it's time to attempt reconnecting
   unsigned long currentMillis = millis();
   
   if (currentMillis - lastReconnectAttempt > reconnectInterval) {
     lastReconnectAttempt = currentMillis;
     
     // Attempt to reconnect
     if (connect()) {
       lastReconnectAttempt = 0;
       return true;
     } else {
       Serial.println("Reconnect failed, will try again...");
       return false;
     }
   }
   
   return false;
 }
 
 // Check if connected to AWS IoT
 bool AwsMqttHandler::isConnected() {
   return mqttClient.connected();
 }
 
 // Process MQTT messages
 void AwsMqttHandler::loop() {
   // Check if connected and reconnect if needed
   if (!isConnected()) {
     reconnect();
   }
   
   // Allow the MQTT client to process incoming messages
   if (isConnected()) {
     mqttClient.loop();
   }
 }
 
 // Publish a message to AWS IoT
 bool AwsMqttHandler::publish(const char* topic, const char* payload) {
   bool success = mqttClient.publish(topic, payload);
   
   if (success) {
     Serial.print("Published to ");
     Serial.print(topic);
     Serial.print(": ");
     Serial.println(payload);
   } else {
     Serial.println("Failed to publish message");
   }
   
   return success;
 }
 
 // Publish device status to AWS IoT (all parameters)
 bool AwsMqttHandler::publishDeviceStatus(bool podStatus, bool ledStatus, String ledColor, int ledBrightness) {
   // Create a JSON document for the device shadow
   StaticJsonDocument<256> jsonDoc;
   JsonObject state = jsonDoc.createNestedObject("state");
   JsonObject reported = state.createNestedObject("reported");
   
   // Add device status to the JSON document with the specified token names
   reported["is_open"] = podStatus;
   reported["nightlight"] = ledStatus;
   reported["color"] = ledColor;
   reported["nightlight_brightness"] = ledBrightness;
   
   // Serialize the JSON document to a string
   char jsonBuffer[256];
   serializeJson(jsonDoc, jsonBuffer);
   
   // Publish the message
   return publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
 }
 
 // Publish pod status to AWS IoT
 bool AwsMqttHandler::publishPodStatus(bool podStatus) {
   // Create a JSON document for the device shadow
   StaticJsonDocument<128> jsonDoc;
   JsonObject state = jsonDoc.createNestedObject("state");
   JsonObject reported = state.createNestedObject("reported");
   
   // Add only pod status to the JSON document
   reported["is_open"] = podStatus;
   
   // Serialize the JSON document to a string
   char jsonBuffer[128];
   serializeJson(jsonDoc, jsonBuffer);
   
   // Publish the message
   return publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
 }
 
 // Publish LED status to AWS IoT
 bool AwsMqttHandler::publishLedStatus(bool ledStatus) {
   // Create a JSON document for the device shadow
   StaticJsonDocument<128> jsonDoc;
   JsonObject state = jsonDoc.createNestedObject("state");
   JsonObject reported = state.createNestedObject("reported");
   
   // Add only LED status to the JSON document
   reported["nightlight"] = ledStatus;
   
   // Serialize the JSON document to a string
   char jsonBuffer[128];
   serializeJson(jsonDoc, jsonBuffer);
   
   // Publish the message
   return publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
 }
 
 // Publish LED color to AWS IoT
 bool AwsMqttHandler::publishLedColor(String ledColor) {
   // Create a JSON document for the device shadow
   StaticJsonDocument<128> jsonDoc;
   JsonObject state = jsonDoc.createNestedObject("state");
   JsonObject reported = state.createNestedObject("reported");
   
   // Add only LED color to the JSON document
   reported["color"] = ledColor;
   
   // Serialize the JSON document to a string
   char jsonBuffer[128];
   serializeJson(jsonDoc, jsonBuffer);
   
   // Publish the message
   return publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
 }
 
 // Publish LED brightness to AWS IoT
 bool AwsMqttHandler::publishLedBrightness(int ledBrightness) {
   // Create a JSON document for the device shadow
   StaticJsonDocument<128> jsonDoc;
   JsonObject state = jsonDoc.createNestedObject("state");
   JsonObject reported = state.createNestedObject("reported");
   
   // Add only LED brightness to the JSON document
   reported["nightlight_brightness"] = ledBrightness;
   
   // Serialize the JSON document to a string
   char jsonBuffer[128];
   serializeJson(jsonDoc, jsonBuffer);
   
   // Publish the message
   return publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
 }
 
 // Set callback for incoming messages
 void AwsMqttHandler::setCallback(void (*callback)(char*, byte*, unsigned int)) {
   mqttClient.setCallback(callback);
 }