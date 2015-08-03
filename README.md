# microgear-esp8266-arduino

microgear-esp8266-arduino คือ client library ที่ทำหน้าที่เป็นตัวกลางในการเชื่อมต่อ ESP8266 เข้ากับบริการของ netpie platform เพื่อการพัฒนา IOT application รายละเอียดเกี่ยวกับ netpie platform สามารถศึกษาได้จาก http://netpie.io

## การติดตั้ง

```
git clone https://github.com/netpieio/microgear-esp8266-arduino.git
```

ตัวอย่างการเรียกใช้
```c++
#include <AuthClient.h>
#include <MicroGear.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>

const char* ssid     = <WIFI_SSID>;
const char* password = <WIFI_KEY>;

#define APPID       <APPID>
#define GEARKEY     <APPKEY>
#define GEARSECRET  <APPSECRET>
#define SCOPE       ""

WiFiClient client;
AuthClient *authclient;

void msghandler(char *topic, uint8_t* msg, unsigned int msglen) {
  Serial.print("Incoming message --> ");
  Serial.print(topic);
  for (int i=0; i<msglen; i++)
    Serial.print((char)msg[i]);
  Serial.println();  
}

void foundgear(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.print("Found new member --> ");
  for (int i=0; i<msglen; i++)
    Serial.print((char)msg[i]);
  Serial.println();  
}

void lostgear(char *attribute, uint8_t* msg, unsigned int msglen) {
  Serial.print("Lost member --> ");
  for (int i=0; i<msglen; i++)
    Serial.print((char)msg[i]);
  Serial.println();  
}

int timer = 0;
MicroGear microgear(client);

void setup() {
    Serial.begin(115200);

    /* Event listener */
    microgear.on(MESSAGE,msghandler);
    microgear.on(PRESENT,foundgear);
    microgear.on(ABSENT,lostgear);

    Serial.println("Starting...");

    if (WiFi.begin(ssid, password)) {

      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }

      Serial.println("WiFi connected");  
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());

	  //uncomment the line below if you want to reset token -->
      //microgear.resetToken();
      microgear.init(GEARKEY,GEARSECRET,SCOPE);
      microgear.connect(APPID);
      microgear.setName("mygear");
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
**microgear.init (*gearkey*, *gearsecret*, *scope*)**

**arguments**
* *gearkey* `string` - เป็น key สำหรับ gear ที่จะรัน ใช้ในการอ้างอิงตัวตนของ gear
* *gearsecret* `string` - เป็น secret ของ key ซึ่งจะใช้ประกอบในกระบวนการยืนยันตัวตน
* *scope* `string` - เป็นการระบุขอบเขตของสิทธิ์ที่ต้องการ

**scope**
เป็นการต่อกันของ string ในรูปแบบต่อไปนี้ คั่นด้วยเครื่องหมาย comma
  * [r][w]:&lt;/topic/path&gt; - r และ w คือสิทธิ์ในการ publish ละ subscribe topic ดังที่ระบุ เช่น rw:/outdoor/temp
  *  name:&lt;gearname&gt; - คือสิทธิ์ในการตั้งชื่อตัวเองว่า &lt;gearname&gt;
  *  chat:&lt;gearname&gt; - คือสิทธ์ในการ chat กับ &lt;gearname&gt;
ในขั้นตอนของการสร้าง key บนเว็บ netpie.io นักพัฒนาสามารถกำหนดสิทธิ์ขั้นพื้นฐานให้แต่ละ key ได้อยู่แล้ว หากการ create microgear อยู่ภายใต้ขอบเขตของสิทธิ์ที่มี token จะถูกจ่ายอัตโนมัติ และ microgear จะสามารถเชื่อมต่อ netpie platform ได้ทันที แต่หาก scope ที่ร้องขอนั้นมากเกินกว่าสิทธิ์ที่กำหนดไว้ นักพัฒนาจะได้รับ notification ให้พิจารณาอนุมัติ microgear ที่เข้ามาขอเชื่อมต่อ ข้อควรระวัง หาก microgear มีการกระทำการเกินกว่าสิทธิ์ที่ได้รับไป เช่น พยายามจะ publish ไปยัง topic ที่ตัวเองไม่มีสิทธิ์ netpie จะตัดการเชื่อมต่อของ microgear โดยอัตโนมัติ ในกรณีที่ใช้ APPKEY เป็น gearkey เราสามารถละเว้น attribute นี้ได้ เพราะ APPKEY จะได้สิทธิ์ทุกอย่างในฐานะของเจ้าของ app โดย default อยู่แล้ว 

```c++
microGear.init("sXfqDcXHzbFXiLk",
               "DNonzg2ivwS8ceksykGntrfQjxbL98",
               "r:/outdoor/temp,w:/outdoor/valve,name:logger,chat:plant");
```
