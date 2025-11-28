/* Heltec Automation LoRaWAN communication example
 *
 * Function:
 * 1. Upload node data to the server using the standard LoRaWAN protocol.
 *  
 * Description:
 * 1. Communicate using LoRaWAN protocol.
 * 
 * HelTec AutoMation, Chengdu, China
 * 成都惠利特自动化科技有限公司
 * www.heltec.org
 *
 * */

#include <Wire.h>               
#include "HT_SSD1306Wire.h"
#include "WiFi.h"
#include "LoRaWan_APP.h"
#include "secrets.h"
#include "BLEDevice.h"
#include <map>

// OLED display

static SSD1306Wire  display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr , freq , i2c group , resolution , rst
int counter = 0;

void writeData(int nb_wifi, int nb_ble) {
    char str[30];
    display.setTextAlignment(TEXT_ALIGN_LEFT);

    // display.setFont(ArialMT_Plain_10);
    // sprintf(str,"Scan n°%d", counter);
    // display.drawString(0, 10, str);

    display.setFont(ArialMT_Plain_10);
    sprintf(str,"WiFi APs found: %d", nb_wifi);
    display.drawString(0, 26, str);

    display.setFont(ArialMT_Plain_10);
    sprintf(str,"BLE devices found: %d", nb_ble);
    display.drawString(0, 42, str);
}

void VextON(void)
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) //Vext default OFF
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, HIGH);
}

// BLE scan

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
uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 15000;

/*OTAA or ABP*/
bool overTheAirActivation = true;

/*ADR enable*/
bool loraWanAdr = true;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = true;

/* Application port */
uint8_t appPort = 2;
/*!
* Number of trials to transmit the frame, if the LoRaMAC layer did not
* receive an acknowledgment. The MAC performs a datarate adaptation,
* according to the LoRaWAN Specification V1.0.2, chapter 18.4, according
* to the following table:
*
* Transmission nb | Data Rate
* ----------------|-----------
* 1 (first)       | DR
* 2               | DR
* 3               | max(DR-1,0)
* 4               | max(DR-1,0)
* 5               | max(DR-2,0)
* 6               | max(DR-2,0)
* 7               | max(DR-3,0)
* 8               | max(DR-3,0)
*
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 4;

byte payload[4];

/* Prepares the payload of the frame */
static void prepareTxFrame( uint8_t port )
{
  /*appData size is LORAWAN_APP_DATA_MAX_SIZE which is defined in "commissioning.h".
  *appDataSize max value is LORAWAN_APP_DATA_MAX_SIZE.
  *if enabled AT, don't modify LORAWAN_APP_DATA_MAX_SIZE, it may cause system hanging or failure.
  *if disabled AT, LORAWAN_APP_DATA_MAX_SIZE can be modified, the max value is reference to lorawan region and SF.
  *for example, if use REGION_CN470, 
  *the max value for different DR can be found in MaxPayloadOfDatarateCN470 refer to DataratesCN470 and BandwidthsCN470 in "RegionCN470.h".
  */
    appDataSize = sizeof(payload);
    memcpy(appData, payload, appDataSize);
}

//if true, next uplink will add MOTE_MAC_DEVICE_TIME_REQ 


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
  delay(100);

  VextON();
  delay(100);

  display.init();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  delay(100);

  memcpy(devEui, secret_devEui, 8);
  memcpy(appEui, secret_appEui, 16);
  memcpy(appKey, secret_appKey, 16);

  memcpy(nwkSKey, secret_nwkSKey, 16);
  memcpy(appSKey, secret_appSKey, 16);
  memcpy(&devAddr, &secret_devAddr, 4);

  Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);
}

void loop()
{
  switch( deviceState )
  {
    case DEVICE_STATE_INIT:
    {
#if(LORAWAN_DEVEUI_AUTO)
      LoRaWAN.generateDeveuiByChipID();
#endif
      LoRaWAN.init(loraWanClass,loraWanRegion);
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
      display.drawString(0, 26, "Scan WiFi...");
      display.display();

      Serial.println("Scan WiFi...");
      int n = WiFi.scanNetworks();
      Serial.printf("Number of WiFi AP(s) found: %d", n);
      Serial.println();

      delay(100);

      display.drawString(0, 42, "Scan BLE...");
      display.display();

      BLEScan *pBLEScan = BLEDevice::getScan();
      Serial.println("Scan BLE...");
      bleData.clear(); 
      pBLEScan->start(5, true);
      pBLEScan->stop();

      Serial.printf("Number of BLE device(s) found: %d", bleData.size());
      Serial.println();

      payload[0] = highByte(n*100);
      payload[1] = lowByte(n*100);
      payload[2] = highByte(bleData.size()*100);
      payload[3] = lowByte(bleData.size()*100);

      delay(100);

      display.clear();
      writeData(n, bleData.size());
      display.display();

      prepareTxFrame( appPort );
      LoRaWAN.send();
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }
    case DEVICE_STATE_CYCLE:
    {
      // Schedule next packet transmission
      txDutyCycleTime = appTxDutyCycle + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
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