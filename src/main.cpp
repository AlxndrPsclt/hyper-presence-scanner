#include <Arduino.h>
#include "SPIFFS.h"
#include <NimBLEDevice.h>

// --- Filter MACs (in lowercase)
const char* allowedMacs[] = {
  "68:67:25:EE:BB:EC",
  "68:67:25:EE:BB:EE",
  "68:67:25:ee:bb:ec",
  "68:67:25:ee:bb:ee"
};
const size_t allowedMacsCount = sizeof(allowedMacs) / sizeof(allowedMacs[0]);

bool isMacAllowed(const String& mac) {
  for (size_t i = 0; i < allowedMacsCount; ++i) {
    if (mac.equalsIgnoreCase(allowedMacs[i])) return true;
  }
  return false;
}

void scanAndLog() {
  Serial.println("Starting scan...");
  NimBLEScan* scanner = NimBLEDevice::getScan();
  Serial.println("Scanner ready...");

  scanner->clearResults();  // Clean up previous results
  scanner->start(3, false); // Scan for 3 seconds (blocking)

  Serial.println("Scan over.");
  NimBLEScanResults results = scanner->getResults();

  for (int i = 0; i < results.getCount(); ++i) {
    Serial.println("Found result");
    const NimBLEAdvertisedDevice* device = results.getDevice(i);  // Get pointer
    String mac = device->getAddress().toString().c_str();
    Serial.println(mac);

    if (isMacAllowed(mac)) {
      int rssi = device->getRSSI();
      unsigned long timestamp = millis() / 1000;

      char line[128];
      snprintf(line, sizeof(line), "%lu %s %d\n", timestamp, mac.c_str(), rssi);

      File f = SPIFFS.open("/ble_log.txt", FILE_APPEND);
      if (f) {
        f.print(line);
        f.close();
        Serial.print("Logged: ");
        Serial.print(line);
      } else {
        Serial.println("Failed to write to SPIFFS");
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
      Serial.println(input);
      if (input == "dump") {
        Serial.println("Dumping...");
        Serial.println("----------");
        File f = SPIFFS.open("/ble_log.txt");
        if (f) {
          while (f.available()) {
            Serial.write(f.read());
          }
          f.close();
          Serial.println("----------");
          Serial.println("Dump ended");
        } else {
          Serial.println("No log file found.");
        }
      } else if (input == "clear") {
        SPIFFS.remove("/ble_log.txt");
        Serial.println("Log cleared.");
      } else {
        Serial.println("Unknown command.");
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

  NimBLEDevice::init("");
  NimBLEScan* scanner = NimBLEDevice::getScan();
  scanner->setActiveScan(true);
  scanner->setInterval(45);
  scanner->setWindow(15);

  Serial.println("BLE beacon scanner running. Type 'dump' or 'clear' via serial.");
}

void loop() {
  scanAndLog();
  checkSerialCommand();
  delay(10000);  // slight delay between scans
}
