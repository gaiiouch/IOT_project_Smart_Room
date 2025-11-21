#include "WiFi.h"
#include "BLEDevice.h"
#include <map>

struct BLEInfo {
  int count = 0;        // Nombre de paquets reçus
  int rssiSum = 0;      // Somme des RSSI
};

std::map<String, BLEInfo> bleData;  // Stocke les infos par adresse MAC

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    String mac = advertisedDevice.getAddress().toString().c_str();
    int rssi = advertisedDevice.getRSSI();

    // Mise à jour ou création d’une entrée
    bleData[mac].count++;
    bleData[mac].rssiSum += rssi;
  }
};

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();  
  delay(100);

  BLEDevice::init("");
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);  // meilleur scan
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(90);
}

void loop() {
  Serial.println("Scan WiFi...");
  int n = WiFi.scanNetworks();

  if (n == 0) {
    Serial.println("Aucun réseau trouvé.");
  } else {
    Serial.printf("%d réseaux trouvés :\n", n);
    for (int i = 0; i < n; ++i) {
      Serial.printf("SSID: %s | RSSI: %d dBm | MAC: %s | Ch: %d\n",
        WiFi.SSID(i).c_str(),
        WiFi.RSSI(i),
        WiFi.BSSIDstr(i).c_str(),
        WiFi.channel(i)
      );
    }
  }

  Serial.println();
  delay(5000);  // Scan toutes les 5 secondes

  BLEScan *pBLEScan = BLEDevice::getScan();
  Serial.println("\n--- Début du scan BLE (5s) ---");
  bleData.clear();  // Efface les données des scans précédents

  pBLEScan->start(10, true);  // Scan pendant 5 secondes
  pBLEScan->stop();

  Serial.println("--- Fin du scan BLE ---\n");

    // Résumé final
  if (bleData.empty()) {
    Serial.println("Aucun appareil BLE trouvé.");
  } else {
    Serial.printf("Appareils BLE trouvés : %d\n\n", bleData.size());

    for (auto const& entry : bleData) {
      String mac = entry.first;
      const BLEInfo& info = entry.second;

      float rssiMean = (float)info.rssiSum / info.count;

      Serial.printf("MAC: %s | Paquets: %d | RSSI moyen: %.1f dBm\n",
        mac.c_str(),
        info.count,
        rssiMean
      );
    }
  }

  delay(3000);  // Pause entre deux scans
}
