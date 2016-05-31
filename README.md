# microgear-esp8266-arduino

microgear-esp8266-arduino  is a client library that is used to connect an ESP8266 chip to the NETPIE Platform's service for developing IoT applications. For more details on the NETPIE Platform, please visit https://netpie.io . 

## Compatibility
We have tested this library and found it compatible with (but not limited to) the following hardware 
- ESP8266-01, 07, 12E, 12F
- NodeMCU v1, v2, V3
- Espresso Lite v2.0

## Installation
*  Download Arduino IDE 1.6.9 or later from https://www.arduino.cc/en/Main/Software
*  After installation, open Preferences
*  Enter `http://arduino.esp8266.com/stable/package_esp8266com_index.json`in the field Additional Board Manager URLs
*  Open Boards Manager menu Tools. Search for  `esp8266` and click install
*  In the menu Tools, there will be several  ESP8266 boards. Choose the one you use.
*  Download Microgear library from https://github.com/netpieio/microgear-esp8266-arduino/archive/master.zip
*  Unzip and put files in the folder folder Libraries of Arduino IDE
*  For more information about ESP8266 Arduino IDE, see  https://github.com/esp8266/Arduino

Usage Example
```c++
#include <ESP8266WiFi.h>
#include <MicroGear.h>

const char* ssid     = <WIFI_SSID>;
const char* password = <WIFI_KEY>;

#define APPID       <APPID>
#define KEY     <APPKEY>
#define SECRET  <APPSECRET>
#define ALIAS   "esp8266"

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
    microgear.setName(ALIAS);
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
    }

    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

	//uncomment the line below if you want to reset token -->
    //microgear.resetToken();
    microgear.init(KEY,SECRET,ALIAS);
    microgear.connect(APPID);
}

void loop() {
    if (microgear.connected()) {
        Serial.println("connected");
        microgear.loop();
        if (timer >= 1000) {
            Serial.println("Publish...");
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
## Library Usage
---
To initial a microgear use one of these methods : 

**int MicroGear::init(char* *key*, char* *secret* [,char* *alias*])**

**arguments**
* *key* - is used as a microgear identity.
* *secret* - comes in a pair with gearkey. The secret is used for authentication and integrity. 
* *alias* - specifies the device alias (optional).  

```c++
microgear.init("sXfqDcXHzbFXiLk", "DNonzg2ivwS8ceksykGntrfQjxbL98", "myplant");
```

---

**void MicroGear::on(unsigned char event, void (* callback)(char*, uint8_t*,unsigned int))**

Add a callback listener to the event.

**arguments**
* *event* - a name of the event (MESSAGE|CONNECTED|PRESENT|ABSENT).
* *callback* - a callback function .

---

**bool MicroGear::connect(char* appid)**

Connect to NETPIE. If succeed, a CONNECTED event will be fired. 

**arguments**
* *appidt* - an App ID.

---

**bool MicroGear::connected(char* appid)**

Check the connection status, return true if it is connected.

**arguments**
* *appidt* - an App ID.

---

**void MicroGear::setAlias(char* alias)**

microgear can set its own alias, which to be used for others make a function call chat(). The alias will appear on the key management portal of netpie.io .

**arguments**
* *alias* - an alias.

---

**void MicroGear::chat(char* target, char* message)**

**arguments**
* *target* - the alias of the microgear(s) that a message will be sent to.
* *message* - message to be sent.

---

**void MicroGear::publish(char* topic, char* message [, bool retained])**

In the case that the microgear want to send a message to an unspecified receiver, the developer can use the function publish to the desired topic, which all the microgears that subscribe such topic will receive a message.

**arguments**
* *topic* - name of topic to be send a message to.
* *message* - message to be sent.
* *retained* - retain a message or not (the default is false)

---

**void MicroGear::subscribe(char* topic)**

microgear may be interested in some topic. The developer can use the function subscribe() to subscribe a message belong to such topic. If the topic used to retain a message, the microgear will receive a message everytime it subscribes that topic.

**arguments**
* *topic* - name of topic to be send a message to.

---

**void MicroGear::unsubscribe(char* topic) *

cancel subscription

**arguments**
* *topic* - name of topic to be send a message to.
 
---

**void MicroGear::resetToken()**

To send a revoke token control message to NETPIE and delete the token from cache. As a result, the microgear will need to request a new token for the next connection.
