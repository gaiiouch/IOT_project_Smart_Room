/*
   BLE Scanner filtrant un iBeacon spécifique + affiche le RSSI
   + continue à afficher le nombre total de devices détectés
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <ArduinoBLE.h>

int scanTime = 5;  //In seconds
BLEScan *pBLEScan;

static const char* TARGET_BEACON_UUID = "A134D0B2-1DA2-1BA7-C94C-E8E00C9F7A2D";

bool beaconFound = false;
int lastRSSI = 0;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {

    // Vérifier s'il y a des données Manufacturer (iBeacon)
    if (advertisedDevice.haveManufacturerData()) {
      std::string manufacturerData = advertisedDevice.getManufacturerData();

      if (manufacturerData.length() >= 22) {

        // Extraire UUID iBeacon
        char uuidStr[37];
        sprintf(uuidStr,
                "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                (uint8_t)manufacturerData[4],  (uint8_t)manufacturerData[5],
                (uint8_t)manufacturerData[6],  (uint8_t)manufacturerData[7],
                (uint8_t)manufacturerData[8],  (uint8_t)manufacturerData[9],
                (uint8_t)manufacturerData[10], (uint8_t)manufacturerData[11],
                (uint8_t)manufacturerData[12], (uint8_t)manufacturerData[13],
                (uint8_t)manufacturerData[14], (uint8_t)manufacturerData[15],
                (uint8_t)manufacturerData[16], (uint8_t)manufacturerData[17],
                (uint8_t)manufacturerData[18], (uint8_t)manufacturerData[19]
               );

        // Comparer avec l’UUID attendu
        if (strcasecmp(uuidStr, TARGET_BEACON_UUID) == 0) {
          beaconFound = true;
          lastRSSI = advertisedDevice.getRSSI();
        }
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Scanning...");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}

void loop() {

  beaconFound = false;  // reset avant chaque scan

  BLEScanResults *foundDevices = pBLEScan->start(scanTime, false);

  // Affiche le nombre total de devices trouvés
  int count = foundDevices->getCount();
  Serial.print("Devices found: ");
  Serial.println(count);

  // Affiche si le beacon est détecté
  if (beaconFound) {
    Serial.println("---------------");
    Serial.println(" Beacon détecté !");
    Serial.print(" UUID : ");
    Serial.println(TARGET_BEACON_UUID);
    Serial.print(" RSSI : ");
    Serial.println(lastRSSI);
    Serial.println("---------------");
  } else {
    Serial.println("Beacon NON détecté.");
  }

  Serial.println("Scan done!\n");

  pBLEScan->clearResults();
  delay(2000);
}
