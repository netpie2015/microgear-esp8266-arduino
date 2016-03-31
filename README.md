# microgear-esp8266-arduino

microgear-esp8266-arduino  is a client library that is used to connect an ESP8266 chip to the NETPIE Platform's service for developing IoT applications. For more details on the NETPIE Platform, please visit https://netpie.io . 

## Compatibility
We have tested this library and found it compatible with (but not limited to) the following hardware 
- ESP8266-01, 07, 12E, 12F
- NodeMCU v1, v2, V3

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

#define APPID   <APPID>
#define KEY     <APPKEY>
#define SECRET  <APPSECRET>
#define ALIAS  "mygear"

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
  microgear.setAlias("mygear");
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
      microgear.init(KEY, SECRET, ALIAS);
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
* *alias* `string` - specifies the device alias.  

```c++
microGear.init("sXfqDcXHzbFXiLk",
               "DNonzg2ivwS8ceksykGntrfQjxbL98",
               "myplant");
```
