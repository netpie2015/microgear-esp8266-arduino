/*
  MicroGear-Arduino-ESP8255 library
   NetPIE Project
   http://netpie.io
*/

#ifndef MICROGEAR_H
#define MICROGEAR_H

#include <stdio.h>
#include <ESP8266WiFi.h>
#include "PubSubClient.h"

#include "MQTTClient.h"
#include <EEPROM.h>
#include "SHA1.h"
#include "AuthClient.h"
//#include "debug.h"


#define GEARTIMEADDRESS "ga.netpie.io"
#define GEARTIMEPORT 8080

#define MINBACKOFFTIME             10
#define MAXBACKOFFTIME             10000
#define MAXENDPOINTLENGTH          200
#define MAXTOPICSIZE               128

#define KEYSIZE                    16
#define TOKENSIZE                  16
#define TOKENSECRETSIZE            32
#define USERNAMESIZE               65
#define PASSWORDSIZE               28
#define REVOKECODESIZE             28
#define FLAGSIZE                   4

#define EEPROM_STATE_NUL           65
#define EEPROM_STATE_REQ           66
#define EEPROM_STATE_ACC           67

#define EEPROM_STATEOFFSET         0
#define EEPROM_KEYOFFSET           1
#define EEPROM_TOKENOFFSET         17
#define EEPROM_TOKENSECRETOFFSET   33
#define EEPROM_REVOKECODEOFFSET    65
#define EEPROM_ENDPOINTSOFFSET     93

#define MICROGEAR_NOTCONNECT       0
#define MICROGEAR_CONNECTED        1
#define MICROGEAR_REJECTED         2
#define RETRY                      3

#define CLIENT_NOTCONNECT          0
#define CLIENT_CONNECTED           1

/* Event Type */
#define MESSAGE                    1
#define PRESENT                    2
#define ABSENT                     3
#define CONNECTED                  4
#define CALLBACK                   5


class MicroGear {
	private:
        char* appid;
		char* gearname;
		char* gearkey;
        char* gearsecret;
        char* gearalias;
        char* scope;
        char* token;
        char* tokensecret;
		char  mqtt_client_type;
		unsigned long bootts;
		int eepromoffset;
		bool eepromready;
        int backoff, retry;

        void* self;
        AuthClient* authclient;
		MQTTClient *mqttclient;
		Client *sockclient;

		bool getHTTPReply(Client*, char*, size_t);
		bool clientReadln(Client*, char*, size_t);

		void syncTime(Client*, unsigned long*);
		void initEndpoint(Client*, char*);
        void getToken(char*, char*, char*, char*, char*);

	public:
		int constate;
        char* endpoint;
		MicroGear(Client&);
		void setName(char*);
		void setAlias(char*);
		bool connect(char*);
		bool connected();
		void publish(char*, char*);
		void publish(char*, char*, bool);
		void subscribe(char*);
		void unsubscribe(char*);
		void chat(char*, char*);
		int state();
		void loop();
        void resetToken();
        void setToken(char*, char*, char*);
        int init(char*, char*);
        int init(char*, char*, char*);
        int init(char*, char*, char*, char*);
        void resetEndpoint();
		void strcat(char*, char*);
		void on(unsigned char,void (* callback)(char*, uint8_t*,unsigned int));
		void setEEPROMOffset(int);
		void readEEPROM(char*,int, int);
		void writeEEPROM(char*,int, int);
		//void msgCallback(char* topic, uint8_t* payload, unsigned int length);
};

#endif
