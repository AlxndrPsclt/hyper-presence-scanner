#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <PubSubClient.h>

const char* ssid = "HUAWEI_B818_F5F9";        // Your WiFi SSID
const char* password = "L50MB7E438J"; // Your WiFi Password
//const char* ssid = "AlexHotspot";        // Your WiFi SSID
//const char* password = "aqut6350"; // Your WiFi Password
const char* serverUrl = "http://192.168.8.130:5000/message"; // Your server URL
const char* scannerId = "ESP32C3_01";

// MQTT Broker
const char *mqtt_broker = "192.168.8.146";
const char *topic = "emqx/esp32";
//const char *mqtt_username = "emqx";
//const char *mqtt_password = "public";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

BLEScan* pBLEScan;

void setup() {
    Serial.begin(115200);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...\n");
    }
    Serial.println("Connected to WiFi\n");

    client.setServer(mqtt_broker, mqtt_port);
        while (!client.connected()) {
        String client_id = "esp32-scanner-";
        client_id += String(WiFi.macAddress());
        Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
        //if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
        if (client.connect(client_id.c_str())) {
            Serial.println("Public EMQX MQTT broker connected\n");
        } else {
            Serial.print("failed with state ");
            Serial.print(client.state());
            Serial.print("\n");
            delay(2000);
        }
    }

    // Initialize BLE
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true); // Active scan to get more detailed data
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
}

void sendDataToServer(const char *macAddress, int rssi) {
    if (WiFi.status() == WL_CONNECTED) {
        //const char *mac = macAddress;

        //String jsonData = "{\"mac_address\": \"" + macAddress + "\", \"rssi\": " + String(rssi) + ", \"scanner_id\": \""+ String(scannerId)+ "\" }";
        //client.publish(topic, jsonData);
        client.publish("mac_address", macAddress);
    } else {
        Serial.println("WiFi not connected");
    }
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        const char *macAddress = advertisedDevice.getAddress().toString().c_str();
        int rssi = advertisedDevice.getRSSI();
        Serial.print("iBeacon found: ");
        Serial.print(macAddress);
        Serial.print(" RSSI: ");
        Serial.println(rssi);
        sendDataToServer(macAddress, rssi);

        // Send the data to the server
        //if (macAddress=="58:cf:79:f1:b3:ea" || macAddress=="68:67:25:ee:16:32" || macAddress=="68:67:25:ee:bb:ee" || macAddress=="db:eb:04:b9:b8:bb") {
        //}
    }
};

void loop() {
    // Scan for BLE devices
    pBLEScan->start(5, false);  // Scan for 5 seconds
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), true);
    pBLEScan->stop();
    delay(5000);  // Delay between scans
}
