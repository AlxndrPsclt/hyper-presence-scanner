#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

const char* ssid = "HUAWEI_B818_F5F9";        // Your WiFi SSID
const char* password = "L50MB7E438J"; // Your WiFi Password
//const char* ssid = "AlexHotspot";        // Your WiFi SSID
//const char* password = "aqut6350"; // Your WiFi Password
const char* serverUrl = "http://192.168.8.130:5000/message"; // Your server URL
const char* scannerId = "ESP32C3_01";

BLEScan* pBLEScan;

void setup() {
    Serial.begin(115200);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Initialize BLE
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true); // Active scan to get more detailed data
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
}

void sendDataToServer(String macAddress, int rssi) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverUrl);
        http.addHeader("Content-Type", "application/json");

        String jsonData = "{\"mac_address\": \"" + macAddress + "\", \"rssi\": " + String(rssi) + ", \"scanner_id\": \""+ String(scannerId)+ "\" }";

        int httpResponseCode = http.POST(jsonData);
        if (httpResponseCode > 0) {
            Serial.println("Data sent successfully: " + jsonData);
        } else {
            Serial.println("Error sending data: " + String(httpResponseCode));
        }
        http.end();
    } else {
        Serial.println("WiFi not connected");
    }
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        String macAddress = advertisedDevice.getAddress().toString().c_str();
        int rssi = advertisedDevice.getRSSI();
        Serial.print("iBeacon found: ");
        Serial.print(macAddress);
        Serial.print(" RSSI: ");
        Serial.println(rssi);

        // Send the data to the server
        if (macAddress=="58:cf:79:f1:b3:ea" || macAddress=="68:67:25:ee:16:32" || macAddress=="68:67:25:ee:bb:ee" || macAddress=="db:eb:04:b9:b8:bb") {
            sendDataToServer(macAddress, rssi);
        }
    }
};

void loop() {
    // Scan for BLE devices
    pBLEScan->start(5, false);  // Scan for 5 seconds
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), true);
    pBLEScan->stop();
    delay(5000);  // Delay between scans
}
