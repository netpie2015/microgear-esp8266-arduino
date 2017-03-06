# microgear-esp8266-arduino

microgear-esp8266-arduino คือ client library ที่ทำหน้าที่เป็นตัวกลางในการเชื่อมต่อ ESP8266 เข้ากับบริการของ netpie platform เพื่อการพัฒนา IOT application รายละเอียดเกี่ยวกับ netpie platform สามารถศึกษาได้จาก http://netpie.io

## ความเข้ากันได้
ทางทีมพัฒนาได้ทำการทดสบพบว่า library สามารถใช้ได้กับอุปกรณ์ต่อไปนี้ (อาจมีมากกว่านี้)
- ESP8266-01, 07, 12E, 12F
- NodeMCU v1, v2, V3
- Espresso Lite v2.0

## การติดตั้ง
* ดาวน์โหลด Arduino IDE 1.6.9 หรือใหม่กว่า จาก https://www.arduino.cc/en/Main/Software
*  หลังจากติดตั้งเสร็จ เปิด Preferences
*  ใส่ข้อความ `http://arduino.esp8266.com/stable/package_esp8266com_index.json` ลงในช่อง Additional Board Manager URLs
*  เปิด Boards Manager เมนู Tools ค้นหาคำว่า `esp8266` และคลิก install
*  ในเมนู Tools จะมีบอร์ด ESP8266 ชนิดต่างๆเพิ่มขึ้นมา เลือกให้ตรงกับชนิดของบอร์ดที่ใช้
*  ดาวน์โหลด Microgear library จาก https://github.com/netpieio/microgear-esp8266-arduino/archive/master.zip
*  Unzip ไปวางใน folder Libraries ของ Arduino IDE
*  รายละเอียดเพิ่มเติมเกี่ยวกับ ESP8266 Arduino IDE ศึกษาได้จาก https://github.com/esp8266/Arduino

**ข้อจำกัที่พบด**
* ฟีเจอร์ TLS ทำงานได้บน ESP8266 SDK 2.1.0 ไม่ทำงานบนเวอร์ชั่น 2.2.0 แต่ทำงานได้บนเวอร์ชั่น 2.3.0-rc1

**ตัวอย่างการเรียกใช้**
```c++
/*  NETPIE ESP8266 basic sample                            */
/*  More information visit : https://netpie.io             */

#include <ESP8266WiFi.h>
#include <MicroGear.h>

const char* ssid     = <WIFI_SSID>;
const char* password = <WIFI_KEY>;

#define APPID   <APPID>
#define KEY     <APPKEY>
#define SECRET  <APPSECRET>
#define ALIAS   "esp8266"

WiFiClient client;

int timer = 0;
MicroGear microgear(client);

/* If a new message arrives, do this */
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

/* When a microgear is connected, do this */
void onConnected(char *attribute, uint8_t* msg, unsigned int msglen) {
    Serial.println("Connected to NETPIE...");
    /* Set the alias of this microgear ALIAS */
    microgear.setName(ALIAS);
}


void setup() {
    /* Add Event listeners */
    /* Call onMsghandler() when new message arraives */
    microgear.on(MESSAGE,onMsghandler);

    /* Call onFoundgear() when new gear appear */
    microgear.on(PRESENT,onFoundgear);

    /* Call onLostgear() when some gear goes offline */
    microgear.on(ABSENT,onLostgear);

    /* Call onConnected() when NETPIE connection is established */
    microgear.on(CONNECTED,onConnected);

    Serial.begin(115200);
    Serial.println("Starting...");

    /* Initial WIFI, this is just a basic method to configure WIFI on ESP8266.                       */
    /* You may want to use other method that is more complicated, but provide better user experience */
    if (WiFi.begin(ssid, password)) {
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
    }

    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    /* Initial with KEY, SECRET and also set the ALIAS here */
    microgear.init(KEY,SECRET,ALIAS);

    /* connect to NETPIE to a specific APPID */
    microgear.connect(APPID);
}

void loop() {
    /* To check if the microgear is still connected */
    if (microgear.connected()) {
        Serial.println("connected");

        /* Call this method regularly otherwise the connection may be lost */
        microgear.loop();

        if (timer >= 1000) {
            Serial.println("Publish...");

            /* Chat with the microgear named ALIAS which is myself */
            microgear.chat(ALIAS,"Hello");
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
Initial library ด้วยคำสั่ง

**int MicroGear::init(char* *key*, char* *secret* [,char* *alias*])**

**arguments**
* *key* `string` - เป็น key สำหรับ gear ที่จะรัน ใช้ในการอ้างอิงตัวตนของ gear
* *secret* `string` - เป็น secret ของ key ซึ่งจะใช้ประกอบในกระบวนการยืนยันตัวตน
* *alias* `string` - เป็นการระบุชื่อของ device

```c++
microgear.init("sXfqDcXHzbFXiLk", "DNonzg2ivwS8ceksykGntrfQjxbL98", "myplant");
```

---

**void MicroGear:: setEEPROMOffset(int *offset*)**

ตั้งค่าตำแหน่งแรกของ EEPROM ที่จะให้ microgear เก็บบันทึก token คำสั่งนี้จะมีประโยชน์ในกรณีที่ผู้ใช้ ต้องการใช้ EEPROM ในการเก็บบันทึกข้อมูลอย่างขึ้นด้วย ข้อมูลจะได้ไม่บันทึกทับซ้อนกัน โดยปกติหากไม่ได้เรียกคำสั่งนี้ microgear library จะใช้ EEPROM เริ่มต้นที่ตำแหน่งที่ 0 ในการเก็บ token

![EEPROM-token](docs/images/EEPROM-token-diagram.png)

**arguments**
* *offset* - ค่า offset ของตำแหน่ง EEPROM ที่ microgaer ใช้บันทึกข้อมูล ค่า default เป็น 0

---

**void MicroGear::on(unsigned char event, void (* callback)(char*, uint8_t*,unsigned int))**

เพิ่มฟังก์ชั่นที่ตอบสนองต่อ event

**arguments**
* *event* - ชื่อ event (MESSAGE|CONNECTED|PRESENT|ABSENT)
* *callback* - ฟังก์ชั่น callback

---

**bool MicroGear::connect(char* appid)**

เชื่อต่อกับ NETPIE platform ถ้าเชื่อมต่อสำเร็จ จะมี event ชื่อ CONNECTED เกิดขึ้น ค่าที่ส่งคืนมาจากฟังก์ชั่น มีดังนี้
* *NETPIECLIENT_CONNECTED* - การเชื่อมต่อสำเร็จ
* *NETPIECLIENT_NOTCONNECTED* - การเชื่อมต่อล้มเหลว เช่นมีปัญหาเรื่องเครือข่าย
* *NETPIECLIENT_TOKENERROR* - ไม่ได้รับ access token อาจเป็นเพราะ appid, key หรือ secret ไม่ถูกต้อง

**arguments**
* *appid* - App ID.

---

**bool MicroGear::connected()**

ส่งค่าสถานะการเชื่อมต่อ เป็น true หากกำลังเชื่อมต่ออยู่

---

**void MicroGear::setAlias(char* alias)**

microgear สามารถตั้งนามแฝงของตัวเองได้ ซึ่งสามารถใช้เป็นชื่อให้คนอื่นเรียกในการใช้ฟังก์ชั่น chat() และชื่อที่ตั้งในโค้ด จะไปปรากฎบนหน้าจัดการ key บนเว็บ netpie.io อย่างอัตโนมัติ

**arguments**
* *alias* - ชื่อของ microgear นี้

---

**bool MicroGear::chat(char* target, char* message)**<br/>
**bool MicroGear::chat(char* target, int message)**<br/>
**bool MicroGear::chat(char* target, double message)**<br/>
**bool MicroGear::chat(char* target, double, int decimal)**<br/>
**bool MicroGear::chat(char* target, String message)**<br/>
		
**arguments**
* *target* - ชื่อของ microgear ที่ต้องการจะส่งข้อความไปถึง
* *decimal* - จำนวนตำแหน่งหลังจุดทศนิยม
* *message* - ข้อความ

---

**bool MicroGear::publish(char* topic, char* message [, bool retained])**<br/>
**bool MicroGear::publish(char* topic, double message [, bool retained])**<br/>
**bool MicroGear::publish(char* topic, double message, int decimal [, bool retained])**<br/>
**bool MicroGear::publish(char* topic, int message [, bool retained])**<br/>
**bool MicroGear::publish(char* topic, String message [, bool retained])**<br/>

ในกรณีที่ต้องการส่งข้อความแบบไม่เจาะจงผู้รับ สามารถใช้ฟังชั่น publish ไปยัง topic ที่กำหนดได้ ซึ่งจะมีแต่ microgear ที่ subscribe topoic นี้เท่านั้น ที่จะได้รับข้อความ

**arguments**
* *topic* - ชื่อของ topic ที่ต้องการจะส่งข้อความไปถึง
* *message* - ข้อความ
* *decimal* - จำนวนตำแหน่งหลังจุดทศนิยม
* *retained* - ให้ retain ข้อความไว้หรือไม่ default เป็น false (optional)

---

**void MicroGear::subscribe(char* topic)**

microgear อาจจะมีความสนใจใน topic ใดเป็นการเฉพาะ เราสามารถใช้ฟังก์ชั่น subscribe() ในการบอกรับ message ของ topic นั้นได้ และหาก topic นั้นเคยมีการ retain ข้อความไว้ microgear จะได้รับข้อความนั้นทุกครั้งที่ subscribe topic

**arguments**
* *topic* - ชื่อของ topic ที่ต้องการจะส่งข้อความไปถึง

---

**void MicroGear::unsubscribe(char* topic)**

ยกเลิกการ subscribe

**arguments**
* *topic* - ชื่อของ topic ที่ต้องการจะส่งข้อความไปถึง

---
**void microgear.writeFeed (char* feedid, char *datajson)**<br/>
**void microgear.writeFeed (char* feedid, char *datajson, char *apikey)**<br/>
**void microgear.writeFeed (char* feedid, String datajson)**<br/>
**void microgear.writeFeed (char* feedid, String datajson, char *apikey)**<br/>
เขียนข้อมูลลง feed storage

**arguments**
* *feedid* - ชื่อของ feed ที่ต้องการจะเขียนข้อมูล 
* *datajson* - ข้อมูลที่จะบันทึก ในรูปแบบ json 
* *apikey* - apikey สำหรับตรวจสอบสิทธิ์ หากไม่กำหนด จะใช้ default apikey ของ feed ที่ให้สิทธิ์ไว้กับ AppID

```js
microgear.writeFeed("homesensor","{temp:25.7,humid:62.8,light:8.5}");
```
---


**void MicroGear::resetToken()**

ส่งคำสั่ง revoke token ไปยัง netpie และลบ token ออกจาก cache ส่งผลให้ microgear ต้องขอ token ใหม่ในการเชื่อมต่อครั้งต่อไป

---

**void MicroGear::loop()**

method นี้ควรถูกเรียกใน arduino loop() เป็นระยะๆ เพื่อที่ microgear library จะได้ keep alive connection alive และจัดการกับ message ที่เข้ามา
