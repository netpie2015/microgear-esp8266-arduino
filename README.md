# microgear-esp8266-arduino

microgear-esp8266-arduino  is a client library that is used to connect an ESP8266 chip to the NETPIE Platform's service for developing IoT applications. For more details on the NETPIE Platform, please visit https://netpie.io . 

## Compatibility
We have tested this library and found it compatible with (but not limited to) the following hardware 
- ESP8266-01
- ESP8266-12E
- NodeMCU v1 ??? v2

## Installation
* Download Arduino IDE 1.6.5 from https://www.arduino.cc/en/Main/Software
*  After installation, open Preferences
*  Enter `http://arduino.esp8266.com/stable/package_esp8266com_index.json`in the field Additional Board Manager URLs
*  Open Boards Manager menu Tools. Search for  `esp8266` and click install
*  In the menu Tools, there will be several  ESP8266 boards. Choose the one you use.
*  Download Microgear library from https://github.com/netpieio/microgear-esp8266-arduino/archive/master.zip
*  Unzip and put files in the folder folder Libraries of Arduino IDE
*  For more information about ESP8266 Arduino IDE, see  https://github.com/esp8266/Arduino

Usage Example
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
#define GEARKEY     <APPKEY>
#define GEARSECRET  <APPSECRET>
#define SCOPE       ""

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

	  //uncomment the line below if you want to reset token -->
      //microgear.resetToken();
      microgear.init(GEARKEY,GEARSECRET,SCOPE);
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
## Library Usage
---
**microgear.init (*gearkey*, *gearsecret*, *scope*)**

**arguments**
* *gearkey* `string` - is used as a microgear identity.
* *gearsecret* `string` comes in a pair with gearkey. The secret is used for authentication and integrity. 
* *scope* `string` - specifies the right.  

**scope** is an optional field. This can be specified when the microgear needs additional rights beyond default scope. If the scope is specified, it may need an approval from the Application ID's owner for each request. The scope format is the concatenation of strings in the following forms, separated with commas:

  * [r][w]:&lt;/topic/path&gt; - r and w is the right to publish and subscribe topic as specified such as rw:/outdoor/temp
  *  name:&lt;gearname&gt; - is the right to name the &lt;gearname&gt;
  *  chat:&lt;gearname&gt; - is the right to chat with &lt;gearname&gt;
In the key generation process on the web netpie.io, the developer can specify basic rights to each key. If the creation of microgear is within right scope, a token will be automatically issued, and the microgear can be connected to NETPIE immediately. However, if the requested scope is beyond the specified right, the developer will recieve a notification to approve a microgear's connection. Note that if the microgear has operations beyond its right (e.g., pulishing to the topic that it does not has the right to do so), NETPIE will automatically disconnect the microgear. In case that APPKEY is used as a gearkey, the developer can ignore this attribute since by default the APPKEY will gain all rights as the ownwer of the app.

```c++
microGear.init("sXfqDcXHzbFXiLk",
               "DNonzg2ivwS8ceksykGntrfQjxbL98",
               "r:/outdoor/temp,w:/outdoor/valve,name:logger,chat:plant");
```