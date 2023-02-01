#include <M5StickC.h>
#include "BLEDevice.h"

uint8_t seq; // remember number of boots in RTC Memory
float temp, humid, lux, bat; // sensor data
int button; // button data

#define ManufacturerId 0x03da  // manufacturer ID for STM550B / PTM215B
#define STM550B 0x6535  // e5 device address
#define PTM215B 0x6532  // e2 device address

BLEScan* pBLEScan;
int scanTime = 10; //In seconds

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice d) {
      if (d.haveManufacturerData()) {
        std::string data = d.getManufacturerData();
        int manu = data[1] << 8 | data[0];
        std::string address = d.getAddress().toString();
        int addr = address[0] << 8 | address[1];

        if (manu == ManufacturerId && addr == STM550B && seq != data[2]) {
          seq = data[2];
          temp = (float)(data[8] << 8 | data[7]) / 100.0;
          humid = (float)(data[10]) / 2.0;
          lux = (float)(data[13] << 8 | data[12]);
          bat = (float)(data[22]) / 2.0; 
          Serial.printf("STM550B: seq: %d, %s \n", seq, d.toString().c_str());
          Serial.printf("STM550B: t: %.1f, h: %.1f, l: %.1f, b: %.1f \n", temp, humid, lux, bat);
        }

        if (manu == ManufacturerId && addr == PTM215B && seq != data[2]) {
          seq = data[2];
          button = data[6];
          Serial.printf("PTM215B: seq: %d, %s \n", seq, d.toString().c_str());
          Serial.printf("PTM215B: button: %d \n", button);
        }
      }
    }
};

void setup() {
    M5.begin();
    M5.Axp.ScreenBreath(0);

    Serial.begin(115200);
    Serial.print("\r\nscan start.\r\n");

    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), true);  // true: duplicate packet
    pBLEScan->setActiveScan(false);  // default: false, active scan uses more power, but get results faster
    pBLEScan->setInterval(100);  // default: 100
    pBLEScan->setWindow(100);  // default: 100, less or equal setInterval value
}

void loop() {
    Serial.print("BLE scan start. \n");
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
    Serial.print("BLE scan end. \n");
    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
}
