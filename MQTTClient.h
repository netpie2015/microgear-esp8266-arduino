/**
 *  MQTTClient - abstract class to implement MQTT client class
 */

#ifndef MQTTClient_h
#define MQTTClient_h
#include "Client.h"

class MQTTClient {
	public:   
	virtual boolean connect(const char* id);
	virtual boolean connect(const char* id, const char* user, const char* pass);
	virtual boolean connect(const char* id, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage);
	virtual boolean connect(const char* id, const char* user, const char* pass, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage);
	virtual void disconnect();
	virtual boolean publish(const char* topic, const char* payload);
	virtual boolean publish(const char* topic, const char* payload, boolean retained);
	virtual boolean publish(const char* topic, const uint8_t * payload, unsigned int plength);
	virtual boolean publish(const char* topic, const uint8_t * payload, unsigned int plength, boolean retained);
	virtual boolean publish_P(const char* topic, const uint8_t * payload, unsigned int plength, boolean retained);
	virtual boolean subscribe(const char* topic);
	virtual boolean subscribe(const char* topic, uint8_t qos);
	virtual boolean unsubscribe(const char* topic);
	virtual boolean loop();
	virtual boolean connected();
    virtual int state();
  };
#endif
