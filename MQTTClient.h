/**
 *  MQTTClient - abstract class to implement MQTT client class
 */

#ifndef MQTTClient_h
#define MQTTClient_h
#include "Client.h"

class MQTTClient {
	public:   
	virtual int connect(char *);
	virtual int connect(char *, char *, char *);
	virtual int connect(char *, char *, uint8_t, uint8_t, char *);
	virtual int connect(char *, char *, char *, char *, uint8_t, uint8_t, char*);
	virtual void disconnect();
	virtual boolean publish(char *, char *);
	virtual boolean publish(char *, uint8_t *, unsigned int);
	virtual boolean publish(char *, uint8_t *, unsigned int, boolean);
	virtual boolean publish_P(char *, uint8_t *, unsigned int, boolean);
	virtual boolean subscribe(char *);
	virtual boolean subscribe(char *, uint8_t qos);
	virtual boolean unsubscribe(char *);
	virtual boolean loop();
	virtual boolean connected();
  };
#endif
