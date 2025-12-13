# IOT_project_Smart_Room

Participants : Anne-Gaëlle MAUGER and Louise MARC

## What is this project about

The goal of this project is to be able to know the occupancy of a room thanks to IoT technology. 

More precisely, we use BLE and WiFi scanning to detect beacons and deduce a number of devices in the area. 
This is the lorawan_scanner code part. The data are sent from our esp32 boards to LoRaWan gateways through TTN.

Then, the part beacon_advertiser allows an esp32 board to emit regularly a BLE beacon. The scanner can detect him specifically and gives its rssi value when detected. That can help to know the distance between the scanner and the emitter. 

## Libraries used

### Arduino

ArduinoBLE
Adafruit SSD1306 and associated libraries
Heltec EPS32 Dev-Boards

### Python 

Use matplotlib to display data. 