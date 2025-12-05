#include <ArduinoBLE.h>
#include <HT_SSD1306Wire.h>

// OLED display
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);  // addr , freq , i2c group , resolution , rst

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void)  //Vext default OFF
{
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

void setup() {
  Serial.begin(115200);

  // display
  VextON();
  delay(100);

  display.init();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  delay(100);

  while (!Serial);

  Serial.println("Starting BLE Beacon...");

  if (!BLE.begin()) {
    Serial.println("BLE start failed!");
    while (1);
  }

  // Set device name
  BLE.setDeviceName("ESP32-Mister-T");
  BLE.setLocalName("ESP32-Mister-T");

  // Set advertising interval (optional)
  // Value is in units of 0.625 ms → 160 = 100 ms
  BLE.setAdvertisingInterval(160);

  // Configure advertising packet
  BLE.advertise();

  Serial.println("Beacon is now advertising!");

  display.drawString(0, 26, "Adversiting !");
  display.display();
}

void loop() {

}
