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