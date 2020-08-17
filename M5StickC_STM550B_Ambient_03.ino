#include <M5StickC.h>
#include "BLEDevice.h"
#include "Ambient.h"

uint8_t seq; // remember number of boots in RTC Memory
float temp, humid, lux, bat; // sensor data
int button; // button data
bool found = false;

#define ManufacturerId 0x03da  // manufacturer ID for STM550B / PTM215B
#define STM550B 0x6535  // e2 device address
#define PTM215B 0x6532  // e5 device address

WiFiClient client;
const char* ssid = ""; // Wi-Fi SSID here
const char* password = ""; // Wi-Fi Password here

Ambient ambient;
unsigned int channelId = ""; // Ambient チャネルID
const char* writeKey = ""; // Ambient ライトキー

BLEScan* pBLEScan;
int scanTime = 3; //In seconds

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
          found = true;
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
    // pBLEScan->setWindow(100);  // default: 100, less or equal setInterval value

    ambient.begin(channelId, writeKey, &client); // チャネルIDとライトキーを指定してAmbientの初期化
}

void loop() {
    Serial.print("BLE scan start. \n");
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false); // true: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/BLEScan.cpp#L208
    Serial.print("BLE scan end. \n");
    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory

    if (found) {
      int lpcnt=0 ;
      int lpcnt2=0 ;

      WiFi.begin(ssid, password);  //  Wi-Fi APに接続
      while (WiFi.status() != WL_CONNECTED) {  //  Wi-Fi AP接続待ち
        delay(100);
        lpcnt += 1 ;                           //
        if (lpcnt > 6) {                       // 6回目(3秒) で切断/再接続
          WiFi.disconnect(true,true);          //
          WiFi.begin(ssid, password);    //
          lpcnt = 0 ;                          //
          lpcnt2 += 1 ;                        // 再接続の回数をカウント
        }                                      //
        if (lpcnt2 > 3) {                      // 3回 接続できなければ、
          ESP.restart() ;                      // ソフトウェアリセット
        }                  
        Serial.print(".");
      }
      Serial.print("\nWiFi connected. \nIP address: ");
      Serial.println(WiFi.localIP());

      // 温度、湿度、照度、電源容量の値をAmbientに送信する
      ambient.set(1, temp);
      ambient.set(2, humid);
      ambient.set(3, lux);
      ambient.set(4, bat);
      ambient.send();
      Serial.print("ambient: data sent. \n");

      WiFi.disconnect(true,true);
      Serial.print("WiFi disconnected. \n");

      found = false;
    }
}
