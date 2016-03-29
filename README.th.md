# microgear-esp8266-arduino

microgear-esp8266-arduino คือ client library ที่ทำหน้าที่เป็นตัวกลางในการเชื่อมต่อ ESP8266 เข้ากับบริการของ netpie platform เพื่อการพัฒนา IOT application รายละเอียดเกี่ยวกับ netpie platform สามารถศึกษาได้จาก http://netpie.io

## ความเข้ากันได้
ทางทีมพัฒนาได้ทำการทดสบพบว่า library สามารถใช้ได้กับอุปกรณ์ต่อไปนี้ (อาจมีมากกว่านี้)
- ESP8266-01
- ESP8266-12E
- NodeMCU v1 และ v2

## การติดตั้ง
* ดาวน์โหลด Arduino IDE 1.6.5 จาก https://www.arduino.cc/en/Main/Software
*  หลังจากติดตั้งเสร็จ เปิด Preferences
*  ใส่ข้อความ `http://arduino.esp8266.com/stable/package_esp8266com_index.json` ลงในช่อง Additional Board Manager URLs
*  เปิด Boards Manager เมนู Tools ค้นหาคำว่า `esp8266` และคลิก install
*  ในเมนู Tools จะมีบอร์ด ESP8266 ชนิดต่างๆเพิ่มขึ้นมา เลือกให้ตรงกับชนิดของบอร์ดที่ใช้
*  ดาวน์โหลด Microgear library จาก https://github.com/netpieio/microgear-esp8266-arduino/archive/master.zip
*  Unzip ไปวางใน folder Libraries ของ Arduino IDE
*  รายละเอียดเพิ่มเติมเกี่ยวกับ ESP8266 Arduino IDE ศึกษาได้จาก https://github.com/esp8266/Arduino

ตัวอย่างการเรียกใช้
```c++
#include <AuthClient.h>
#include <MicroGear.h>
#include <MQTTClient.h>
#include <SHA1.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <MicroGear.h>

const char* ssid     = <WIFI_SSID>;
const char* password = <WIFI_KEY>;

#define APPID       <APPID>
#define KEY     <APPKEY>
#define SECRET  <APPSECRET>
#define ALIAS       "myplant"

WiFiClient client;
AuthClient *authclient;

int timer = 0;
MicroGear microgear(client);

void onMsghandler(char *topic, uint8_t* msg, unsigned int msglen) {
  Serial.print("Incoming message --> ");
  msg[msglen] = '\0';
  Serial.println((char *)msg);
}

void onFoundgear(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.print("Found new member --> ");
  for (int i=0; i<msglen; i++)
    Serial.print((char)msg[i]);
  Serial.println();  
}

void onLostgear(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.print("Lost member --> ");
  for (int i=0; i<msglen; i++)
    Serial.print((char)msg[i]);
  Serial.println();
}

void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.println("Connected to NETPIE...");
  microgear.setName("mygear");
}


void setup() {
    /* Event listener */
    microgear.on(MESSAGE,onMsghandler);
    microgear.on(PRESENT,onFoundgear);
    microgear.on(ABSENT,onLostgear);
    microgear.on(CONNECTED,onConnected);

    Serial.begin(115200);
    Serial.println("Starting...");

    if (WiFi.begin(ssid, password)) {

      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }

      Serial.println("WiFi connected");  
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());

      microgear.init(KEY,SECRET,ALIAS);
      microgear.connect(APPID);
    }
}

void loop() {
  if (microgear.connected()) {
    Serial.println("connected");
    microgear.loop();
    if (timer >= 1000) {
        Serial.println("Publish...");
        microgear.chat("mygear","Hello");
        timer = 0;
    } 
    else timer += 100;
  }
  else {
      Serial.println("connection lost, reconnect...");
      if (timer >= 5000) {
      microgear.connect(APPID);
      timer = 0;
    }
    else timer += 100;
  }
  delay(100);
}
```
## การใช้งาน library
---
**microgear.init (*gearkey*, *gearsecret*, *alias*)**

**arguments**
* *key* `string` - เป็น key สำหรับ gear ที่จะรัน ใช้ในการอ้างอิงตัวตนของ gear
* *secret* `string` - เป็น secret ของ key ซึ่งจะใช้ประกอบในกระบวนการยืนยันตัวตน
* *alias* `string` - เป็นการระบุชื่อของ device

```c++
microGear.init("sXfqDcXHzbFXiLk",
               "DNonzg2ivwS8ceksykGntrfQjxbL98",
               "myplant");
```
