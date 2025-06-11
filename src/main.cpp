#include <Arduino.h>
#include "SPIFFS.h"
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// --- Filter MACs (in lowercase)
const char* allowedMacs[] = {
  "68:67:25:EE:BB:EC",
  "68:67:25:EE:BB:EE",
  "68:67:25:ee:bb:ec",
  "68:67:25:ee:bb:ee"
};
const size_t allowedMacsCount = sizeof(allowedMacs) / sizeof(allowedMacs[0]);
bool verbose = false;

bool isMacAllowed(const String& mac) {
  for (size_t i = 0; i < allowedMacsCount; ++i) {
    if (mac.equalsIgnoreCase(allowedMacs[i])) return true;
  }
  return false;
}

void serialPrint(String msg) {
  if (verbose) {
      Serial.println(msg);
  }
}

void scanAndLog() {
  serialPrint("Starting scan...");

  BLEScan* scanner = BLEDevice::getScan();
  scanner->clearResults();  // Clean up previous results
  scanner->setAdvertisedDeviceCallbacks(nullptr);  // no callbacks
  scanner->setActiveScan(true);  // important: requests scan responses
  scanner->setInterval(100);
  scanner->setWindow(99);  // make scan window close to interval
  //scanner->start(3, false);
  BLEScanResults results = scanner->start(3, false); // Scan 3s, blocking

  serialPrint("Scan over.");

  for (int i = 0; i < results.getCount(); ++i) {
    BLEAdvertisedDevice device = results.getDevice(i);
    //String mac = String(device.getAddress().toString());
    std::string stdMac = device.getAddress().toString();
    String mac = String(stdMac.c_str());  // Explicit conversion
    serialPrint(mac);

    if (isMacAllowed(mac)) {
      int rssi = device.getRSSI();
      unsigned long timestamp = millis() / 1000;

      char line[128];
      snprintf(line, sizeof(line), "%lu,%s,%d", timestamp, mac.c_str(), rssi);

      File f = SPIFFS.open("/ble_log.csv", FILE_APPEND);
      if (f) {
        f.println(line);
        f.close();
        Serial.println("Logged: ");
        Serial.println(line);
      } else {
        serialPrint("Failed to write to SPIFFS");
      }
    }
  }

  scanner->clearResults(); // Free memory
}

void checkSerialCommand() {
  static String input;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      input.trim();
      serialPrint(input);
      if (input == "dump") {
        Serial.println("Dumping...");
        Serial.println("----------");
        File f = SPIFFS.open("/ble_log.csv");
        if (f) {
          while (f.available()) {
            Serial.write(f.read());
          }
          f.close();
          Serial.println("----------");
          Serial.println("Dump ended");
        } else {
          serialPrint("No log file found.");
        }
      } else if (input == "clear") {
        SPIFFS.remove("/ble_log.csv");
        Serial.println("Log cleared.");
      } else if (input == "verbose") {
        verbose = not(verbose);
        serialPrint("Verbosity changed.");
      } else {
        serialPrint("Unknown command.");
      }
      input = "";
    } else {
      input += c;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed.");
    return;
  }

  BLEDevice::init(""); // Use empty device name for passive scan

  Serial.println("BLE beacon scanner running. Type 'dump' or 'clear' via serial.");
}

void loop() {
  serialPrint("Loop start");
  scanAndLog();
  checkSerialCommand();
  delay(1000);  // slight delay between scans
}
