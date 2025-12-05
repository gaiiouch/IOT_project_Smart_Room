#include <ArduinoBLE.h>
#include <LoRaWan_APP.h>
#include <secrets.h>
#include <HT_SSD1306Wire.h>
#include <WiFi.h>

// OLED display
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);  // addr , freq , i2c group , resolution , rst

void writeData(int nb_wifi, int nb_ble, int rssi) {
  char str[30];
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  sprintf(str, "WiFi APs found: %d", nb_wifi);
  display.drawString(0, 10, str);

  display.setFont(ArialMT_Plain_10);
  sprintf(str, "BLE devices found: %d", nb_ble);
  display.drawString(0, 26, str);

  if (rssi != 150) {
    display.setFont(ArialMT_Plain_10);
    sprintf(str, "Target device: -%d dBm", rssi);
    display.drawString(0, 42, str);
  } else {
    display.setFont(ArialMT_Plain_10);
    sprintf(str, "Target device not found.");
    display.drawString(0, 42, str);
  }
}

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void)  //Vext default OFF
{
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}


// TTN connection

/* OTAA para*/
uint8_t devEui[8];
uint8_t appEui[16];
uint8_t appKey[16];

/* ABP para*/
uint8_t nwkSKey[16];
uint8_t appSKey[16];
uint32_t devAddr;

/*LoraWan channelsmask, default channels 0-7*/
uint16_t userChannelsMask[6] = { 0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t loraWanClass = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 20000;

/*OTAA or ABP*/
bool overTheAirActivation = true;

/*ADR enable*/
bool loraWanAdr = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = true;

/* Application port */
uint8_t appPort = 2;

uint8_t confirmedNbTrials = 4;

byte payload[6];

/* Prepares the payload of the frame */
static void prepareTxFrame(uint8_t port) {
  appDataSize = sizeof(payload);
  memcpy(appData, payload, appDataSize);
}

// BLE
#define MAX_DEVICES 150

String devices[MAX_DEVICES];
int deviceCount = 0;

bool addUniqueDevice(const String& addr) {
  // Checks if MAC address is already registred
  for (int i = 0; i < deviceCount; i++) {
    if (devices[i] == addr) {
      return false;  // already registred
    }
  }
  // Add device to list
  if (deviceCount < MAX_DEVICES) {
    devices[deviceCount++] = addr;
    return true;
  }
  return false;
}


void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  // display
  VextON();
  delay(100);

  display.init();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  delay(100);

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // BLE
  while (!Serial)
    ;

  Serial.println("BLE initialisation…");
  Serial.println("BLE started.");

  if (!BLE.begin()) {
    Serial.println("Error when starting BLE.");
    while (1)
      ;
  }

  // LoraWan
  memcpy(devEui, secret_devEui, 8);
  memcpy(appEui, secret_appEui, 16);
  memcpy(appKey, secret_appKey, 16);

  memcpy(nwkSKey, secret_nwkSKey, 16);
  memcpy(appSKey, secret_appSKey, 16);
  memcpy(&devAddr, &secret_devAddr, 4);

  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  display.drawString(0, 10, "Setup done.");
  display.display();

  // WiFi
  display.drawString(0, 26, "Scan WiFi...");
  display.display();

  Serial.println("Scan WiFi...");
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    Serial.printf("Detected AP | SSID: %s | RSSI: %d dBm | MAC: %s",
                  WiFi.SSID(i).c_str(),
                  WiFi.RSSI(i),
                  WiFi.BSSIDstr(i).c_str());
    Serial.println();
  }
  Serial.printf("Number of WiFi AP(s) found: %d", n);
  Serial.println();
  Serial.println();

  delay(100);

  //BLE for all devices
  display.drawString(0, 42, "Scan BLE...");
  display.display();

  for (int i = 0; i < MAX_DEVICES; i++) {
    devices[i] = "";
  }
  deviceCount = 0;

  BLE.scan();

  unsigned long start1 = millis();
  while (millis() - start1 < 5000) {
    BLEDevice peripheral = BLE.available();
    if (peripheral) {
      String mac = peripheral.address();
      addUniqueDevice(mac);
      Serial.printf("Detected device | Name: %s | RSSI: %d dBm | MAC address: ", 
                    peripheral.deviceName(), 
                    peripheral.rssi());
      Serial.println(peripheral.address());
    }
  }

  BLE.stopScan();

  Serial.printf("Number of BLE device(s) found during 5 seconds scan: %d", deviceCount);
  Serial.println();
  Serial.println();


  // BLE for one specific device
  int target_rssi = -150;
  unsigned long start2 = millis();
  BLE.scanForName("ESP32-Mister-T");
  while (millis() - start2 < 1000) {
    BLEDevice peripheral = BLE.available();
    if (peripheral) {
      Serial.printf("Target device detected | Name: %s | RSSI: %d dBm | MAC address: ", 
                    peripheral.deviceName(), 
                    peripheral.rssi());
      Serial.println(peripheral.address());
      target_rssi = peripheral.rssi();
    }
  }

  BLE.stopScan();

  Serial.println();
  Serial.println();


  // data to send to TTN
  payload[0] = highByte(n * 100);
  payload[1] = lowByte(n * 100);
  payload[2] = highByte(deviceCount * 100);
  payload[3] = lowByte(deviceCount * 100);
  payload[4] = highByte(abs(target_rssi) * 100);
  payload[5] = lowByte(abs(target_rssi) * 100);

  delay(100);

  // display measurements results
  display.clear();
  writeData(n, deviceCount, abs(target_rssi));
  display.display();

  delay(3000);
  display.clear();
}


void loop() {

  switch (deviceState) {
    case DEVICE_STATE_INIT:
      {
#if (LORAWAN_DEVEUI_AUTO)
        LoRaWAN.generateDeveuiByChipID();
#endif
        LoRaWAN.init(loraWanClass, loraWanRegion);
        //both set join DR and DR when ADR off
        LoRaWAN.setDefaultDR(3);
        break;
      }
    case DEVICE_STATE_JOIN:
      {
        display.drawString(0, 10, "Joining...");
        display.display();

        LoRaWAN.join();
        break;
      }
    case DEVICE_STATE_SEND:
      {
        display.drawString(0, 26, "Sending...");
        display.display();

        prepareTxFrame(appPort);
        LoRaWAN.send();
        deviceState = DEVICE_STATE_CYCLE;
        break;
      }
    case DEVICE_STATE_CYCLE:
      {
        // Schedule next packet transmission
        txDutyCycleTime = appTxDutyCycle + randr(-APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND);
        LoRaWAN.cycle(txDutyCycleTime);
        deviceState = DEVICE_STATE_SLEEP;
        break;
      }
    case DEVICE_STATE_SLEEP:
      {
        LoRaWAN.sleep(loraWanClass);
        break;
      }
    default:
      {
        deviceState = DEVICE_STATE_INIT;
        break;
      }
  }
}
