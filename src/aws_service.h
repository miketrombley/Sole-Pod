#ifndef AWS_SERVICE_H
#define AWS_SERVICE_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#define NVS_NAMESPACE "certs"

extern String root_ca;
extern WiFiClientSecure net;
extern PubSubClient client;
extern const char *aws_endpoint;
extern const char *provisioning_template;

extern bool childLockOn;     // Child lock status (true = locked)

static int8_t led_state = 0;         // 0 = off, 1 = on
static int8_t led_brightness = 100; 
static char led_color[9] = "0000FF";

bool runFleetProvisioning();

String readFileToString(const char *path);
String readStringFromNVS(const char* ns, const char* key);
void publishReportedState(const String &thingName, bool ledState, int ledBrightness, const String &ledColor, bool podOpen, bool childLock);

bool generateCSR(String &csr, String &privateKey);

void mqttCallback(char *topic, byte *payload, unsigned int length);

#endif 
