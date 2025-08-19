#include "aws_service.h"
#include <WiFiClientSecure.h>
#include <SPIFFS.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "LEDControl.h"
#include "MotorControl.h"

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/x509_csr.h"
#include "mbedtls/pk.h"

String root_ca, claim_cert, claim_key;
const char *aws_endpoint = "a1ki7phy60saz7-ats.iot.ap-southeast-1.amazonaws.com";
const char *provisioning_template = "Test_SneakerPot";
WiFiClientSecure net;
PubSubClient client(net);
nvs_handle_t handle;

static String csr_out, privateKey_out;
static String certificatePem_out, ownershipToken_out, thingName_out, deviceKey_out;
bool provisioningSuccess = false;



bool writeStringToNVS(const char* ns, const char* key, const String& value) {
    if (nvs_open(ns, NVS_READWRITE, &handle) != ESP_OK) return false;
    esp_err_t err = nvs_set_str(handle, key, value.c_str());
    
    if (err != ESP_OK) {
        Serial.printf("Failed to write %s: %s\n", key, esp_err_to_name(err));
    }
    
    nvs_commit(handle);
    nvs_close(handle);
    
    return (err == ESP_OK);
}

String readStringFromNVS(const char* ns, const char* key) {
    if (nvs_open(ns, NVS_READONLY, &handle) != ESP_OK) return "";
    size_t required_size;
    if (nvs_get_str(handle, key, NULL, &required_size) != ESP_OK) {
        nvs_close(handle);
        return "";
    }
    char* buf = (char*)malloc(required_size);

    if (buf == NULL) {
        Serial.printf("Fail to create buf\n");

        nvs_close(handle);
        return "";
    }
    

    if (nvs_get_str(handle, key, buf, &required_size) != ESP_OK) {
        free(buf);
        nvs_close(handle);
        return "";
    }
    String result = String(buf);
    free(buf);
    nvs_close(handle);
    return result;
}

String readFileToString(const char* path) {
    File file = SPIFFS.open(path, "r");
    if (!file) return "";
    String content = file.readString();
    file.close();
    content.trim();
    return content;
}


bool generateCSR(String &csr, String &privateKey) {
    mbedtls_pk_context key;
    mbedtls_x509write_csr req;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    const char *pers = "csr";

    char *csr_buf = (char *)malloc(2048);
    char *key_buf = (char *)malloc(2048);
    if (!csr_buf || !key_buf) {
        Serial.println("\n Failed to allocate CSR or key buffer");
        return false;
    }

    mbedtls_pk_init(&key);
    mbedtls_x509write_csr_init(&req);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                               (const unsigned char *)pers, strlen(pers)) != 0) goto cleanup;
    if (mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA)) != 0) goto cleanup;
    if (mbedtls_rsa_gen_key(mbedtls_pk_rsa(key), mbedtls_ctr_drbg_random, &ctr_drbg, 2048, 65537) != 0) goto cleanup;

    mbedtls_x509write_csr_set_md_alg(&req, MBEDTLS_MD_SHA256);
    mbedtls_x509write_csr_set_key(&req, &key);
    mbedtls_x509write_csr_set_subject_name(&req, "CN=ESP32_Device");

    if (mbedtls_x509write_csr_pem(&req, (unsigned char *)csr_buf, 2048, mbedtls_ctr_drbg_random, &ctr_drbg) < 0) goto cleanup;
    csr = String(csr_buf);

    if (mbedtls_pk_write_key_pem(&key, (unsigned char *)key_buf, 2048) < 0) goto cleanup;
    privateKey = String(key_buf);

    free(csr_buf);
    free(key_buf);
    mbedtls_pk_free(&key);
    mbedtls_x509write_csr_free(&req);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return true;

cleanup:
    free(csr_buf);
    free(key_buf);
    mbedtls_pk_free(&key);
    mbedtls_x509write_csr_free(&req);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return false;
}

void publishReportedState(const String &thingName, bool ledState, int ledBrightness, const String &ledColor, bool podOpen, bool childLockOn) {
    DynamicJsonDocument doc(512);

    JsonObject state = doc.createNestedObject("state");
    JsonObject reported = state.createNestedObject("reported");

    reported["led"] = ledState ? 1 : 0;
    reported["led_brightness"] = ledBrightness;
    reported["led_color"] = ledColor;
    reported["podOpen"] = podOpen;
    reported["childLockOn"] = childLockOn;
    reported["doorPosition"] = doorPosition;

    String jsonStr;
    serializeJson(doc, jsonStr);

    String topic = String("$aws/things/") + thingName + "/shadow/update";

    Serial.printf("Publishing to topic: %s\nPayload: %s\n", topic.c_str(), jsonStr.c_str());

    if (client.publish(topic.c_str(), jsonStr.c_str())) {
        Serial.println("\n Reported state published successfully");
    } else {
        Serial.println("\n Failed to publish reported state");
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    String message((char *)payload, length);  
    Serial.printf("\n Incoming message on topic: %s\n", topic);
    Serial.printf("Payload: %s\n", message.c_str());
    Serial.printf("Payload length: %u bytes\n", length);

    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, payload, length);
    if (err) {
        Serial.println("\n Failed to parse JSON");
        return;
    }

    String topicStr = String(topic);

    if (topicStr == "testat/topic") {
        String msg = doc["message"].as<String>();
        Serial.printf("\n Test message: %s\n", msg.c_str());
    }

    if (topicStr.endsWith("/shadow/update/delta")) {
        if (doc.containsKey("state")) {
            JsonObject stateObj = doc["state"];

            // "state": {
            //     "desired": {
            //     "welcome": "aws-iot",
            //     "led": 1,
            //     "led_brightness": 10,
            //     "led_color": "0000FF",
            //     "podOpen": false,
            //      "childLockOn": false
            //     },
            //     "reported": {
            //     "welcome": "aws-iot",
            //     "led": 0,
            //     "led_brightness": 100,
            //     "led_color": "0000FF",
            //     "podOpen": true,
            //      "childLockOn": true
            //     }
            // }
            // }

            // On/off LED
            if (stateObj.containsKey("led")) {
                int ledState = stateObj["led"].as<int>();
                Serial.printf("Delta update - LED state: %d\n", ledState);
                led_state = ledState;
                setLEDState(ledState == 1 ? LED_STATE_ON : LED_STATE_OFF);
            }

            // LED Brightness
            if (stateObj.containsKey("led_brightness")) {
                int brightness = stateObj["led_brightness"].as<int>();
                Serial.printf("Delta update - LED brightness: %d\n", brightness);
                if (brightness >= 0 && brightness <= 100) {
                    led_brightness = brightness;
                    setLEDBrightness(led_brightness);
                } else {
                    Serial.printf("Brightness value out of range (0-100): %d\n", brightness);
                }
            }

            // LED Color (HEX string)
            if (stateObj.containsKey("led_color")) {
                String colorHex = stateObj["led_color"].as<String>();
                Serial.printf("Delta update - LED color: %s\n", colorHex.c_str());

                // Gọi hàm setLEDColor đã có để validate, chuyển đổi và cập nhật màu LED
                setLEDColor(colorHex);
            }
            // Open/Close Pod
            if (stateObj.containsKey("podOpen")) {
                    bool podOpenVal = stateObj["podOpen"].as<bool>();
                    Serial.printf("Delta update - podOpen: %s\n", podOpenVal ? "true" : "false");

                    podOpenFlag = podOpenVal;
                    manageMotors(podOpenFlag);
            }
            // Smart serve
            if (stateObj.containsKey("doorPosition")) {
                int doorPos = stateObj["doorPosition"].as<int>();
                Serial.printf("Delta update - doorPosition: %d\n", doorPos);

                if (doorPos == 50 || doorPos == 100) {
                    doorPosition = doorPos;
                    manageMotors(podOpenFlag);   
                    saveDoorPosition(doorPosition);
                } else {
                    Serial.printf("Ignored invalid doorPosition value: %d\n", doorPos);
                }
            }
            // On/Off Child Lock
            if (stateObj.containsKey("childLockOn")) {
                bool childLockVal = stateObj["childLockOn"].as<bool>();
                Serial.printf("Delta update - childLockOn: %s\n", childLockVal ? "true" : "false");
                childLockOn = childLockVal;
                saveChildLockState(childLockOn);
            }
            // Gửi event cập nhật trạng thái nếu cần

            //send_data_update_event();
        }

    }

    // Xử lý khi nhận certificate từ AWS
    if (topicStr == "$aws/certificates/create/json/accepted") {
        deviceKey_out = doc["privateKey"].as<String>();
        certificatePem_out = doc["certificatePem"].as<String>();
        ownershipToken_out = doc["certificateOwnershipToken"].as<String>();
        certificatePem_out.replace("\r", "");

        String payload = "{\"certificateOwnershipToken\":\"" + ownershipToken_out +
                         "\",\"parameters\":{\"SerialNumber\":\"12345678\"}}";
        String regTopic = String("$aws/provisioning-templates/Test_SneakerPot/provision/json");
        client.publish(regTopic.c_str(), payload.c_str());
        Serial.println("\n Published ownershipToken to registerThing");

    } else if (topicStr == "$aws/provisioning-templates/Test_SneakerPot/provision/json/accepted") {
        thingName_out = doc["thingName"].as<String>();
        thingName_out.trim();
        Serial.printf("\n Provisioned thing name: %s\n", thingName_out.c_str());

    } else if (topicStr == "$aws/provisioning-templates/Test_SneakerPot/provision/json/rejected") {
        Serial.println("\n RegisterThing rejected:");
        serializeJsonPretty(doc, Serial);

    } else if (topicStr == "$aws/certificates/create/json/rejected") {
        Serial.println("\n Certificate create rejected:");
        serializeJsonPretty(doc, Serial);

    } 
}

bool waitForProvisioningResponse(unsigned long timeoutMs = 30000) {
    unsigned long start = millis();
    while ((thingName_out == "" || certificatePem_out == "") && millis() - start < timeoutMs) {
        if (!client.loop()) {
            Serial.println("\n MQTT loop failed or disconnected");
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return !(thingName_out == "" || certificatePem_out == "");
}

bool runFleetProvisioning() {
    if (!SPIFFS.begin(true)) {
        Serial.println("\n SPIFFS init failed.");
        return false;
    }

    root_ca = readFileToString("/root_ca.pem");
    claim_cert = readFileToString("/claim_cert.pem");
    claim_key = readFileToString("/claim_private_key.pem");
    if (claim_cert == "" || claim_key == "" || root_ca == "") {
        Serial.println("\n Missing or invalid claim cert/key/root CA");
        return false;
    }
    if (!generateCSR(csr_out, privateKey_out)) {
        Serial.println("\n CSR generation failed.");
        return false;
    }

    Serial.println("\n Claim Cert:");
    Serial.println(claim_cert);
    Serial.println("\n Claim Key:");
    Serial.println(claim_key);

    net.setCACert(root_ca.c_str());
    net.setCertificate(claim_cert.c_str());
    net.setPrivateKey(claim_key.c_str());

    client.setServer(aws_endpoint, 8883);
    client.setBufferSize(4096);
    client.setCallback(mqttCallback);

    if (!client.connect("esp32-claim")) {
        Serial.println("\n MQTT connect failed.");
        Serial.println(client.state());
        return false;
    }
    Serial.println("\n MQTT connected.");

    client.subscribe("$aws/certificates/create/json/accepted");
    client.subscribe("$aws/certificates/create/json/rejected");
    client.subscribe("test/topic");

    String acceptedTopic = "$aws/provisioning-templates/Test_SneakerPot/provision/json/accepted";
    String rejectedTopic = "$aws/provisioning-templates/Test_SneakerPot/provision/json/rejected";
    client.subscribe(acceptedTopic.c_str());
    client.subscribe(rejectedTopic.c_str());

    String escapedCsr = csr_out;
    escapedCsr.replace("\n", "\\n");
    escapedCsr.replace("\r", ""); 

    String payload = "{\"certificateSigningRequest\":\"" + escapedCsr + "\"}";
    client.publish("$aws/certificates/create/json", payload.c_str());

    Serial.println("\n CSR Publish Payload:");
    Serial.println(payload);


    if (!waitForProvisioningResponse(30000)) {
        Serial.println("\n Provisioning response timeout.");
        return false;
    }

    if (!writeStringToNVS(NVS_NAMESPACE, "deviceCert", certificatePem_out)) return false;
    if (!writeStringToNVS(NVS_NAMESPACE, "deviceKey", deviceKey_out)) return false;
    if (!writeStringToNVS(NVS_NAMESPACE, "thingName", thingName_out)) return false;

    return true;
}
