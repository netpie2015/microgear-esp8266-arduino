#include "MicroGear.h"

unsigned char topicprefixlen;
MicroGear* mg = NULL;

void (* cb_message)(char*, uint8_t*,unsigned int);
void (* cb_present)(char*, uint8_t*,unsigned int);
void (* cb_absent)(char*, uint8_t*,unsigned int);
void (* cb_connected)(char*, uint8_t*,unsigned int);
void (* cb_error)(char*, uint8_t*,unsigned int);
void (* cb_info)(char*, uint8_t*,unsigned int);

void msgCallback(char* topic, uint8_t* payload, unsigned int length) {
    /* remove /appid/ */
    char* rtopic =  topic+topicprefixlen+1;

    /* if a control message */
    if (*rtopic == '&') {
        if (strcmp(rtopic,"&present") == 0) {
            if (cb_present) {
                cb_present("present",payload,length);
            }
        }
        else if (strcmp(rtopic,"&absent") == 0) {
            if (cb_absent) {
                cb_absent("absent",payload,length);
            }
        }
        else if (strcmp(rtopic,"&resetendpoint") == 0) {
            #ifdef DEBUG_H
                Serial.println("to reset endpoint");
            #endif
            if (mg) mg->resetEndpoint();
        }
    }
    else if (*topic == '@') {
        if (strcmp(topic,"@error")==0) {
            if (cb_error)  {
                cb_error("error",payload,length);
            }
        }
        else if (strcmp(topic,"@info")==0) {
            if (cb_info)  {
                cb_info("info",payload,length);
            }
        }
    }
    else if (cb_message) {
        cb_message(topic,payload,length);  
    }
}

bool MicroGear::clientReadln(Client* client, char *buffer, size_t buflen) {
    size_t pos = 0;

    while (true) {
        uint8_t byte = client->read();
        if (byte == '\n') {
            // EOF found.
            if (pos < buflen) {
                if (pos > 0 && buffer[pos - 1] == '\r')
                pos--;
                buffer[pos] = '\0';
            }
            else {
                buffer[buflen - 1] = '\0';
            }
            return true;
        }

        if (byte != 255) {
            if (pos < buflen) buffer[pos++] = byte;
        }
        else{
            buffer[pos++] = '\0';
            return false; 
        } 
    }
}

int MicroGear::getHTTPReply(Client *client, char *buff, size_t buffsize) {
    int httpstatus = 0;
    char pline = 0;
    buff[0] = '\0';

    while (true) {
        clientReadln(client, buff, buffsize);
        if (httpstatus==0) {
            if (strncmp(buff,"HTTP",4)==0) {
                buff[12] = '\0';
                httpstatus = atoi(buff+9);
            }
        }
        else {
            if (pline > 0) {
                return httpstatus;
            }
            if (strlen(buff)<1) pline++;
        }
    }
}

void MicroGear::resetEndpoint() {
    writeEEPROM("",EEPROM_ENDPOINTSOFFSET,1);
}

void MicroGear::initEndpoint(Client *client, char* endpoint) {
    if (*endpoint == '\0') {

        #ifdef DEBUG_H
            Serial.println("resync endpoint..");
        #endif

        char pstr[100];
        int port = this->securemode?GEARAUTHSECUREPORT:GEARAUTHPORT;

        if(client->connect(gearauth,port)){
			sprintf(pstr,"GET /api/endpoint/%s HTTP/1.1\r\n\r\n",this->gearkey);
			client->write((const uint8_t *)pstr,strlen(pstr));

			delay(1000);
			getHTTPReply(client,pstr,200);

			if (strlen(pstr)>6) {
				strcpy(endpoint,pstr+6);
				writeEEPROM(endpoint,EEPROM_ENDPOINTSOFFSET,MAXENDPOINTLENGTH);
			}
			client->stop();
		}
    }
}

void MicroGear::syncTime(Client *client, unsigned long *bts) {
    char tstr[200];
    char nonce[10];
    char hash[32], hashkey[60];
    int port = (this->securemode)?GEARAUTHSECUREPORT:GEARAUTHPORT;

    *bts = 0;
    if(client->connect(gearauth,port)){

		if (this->securemode) {
			WiFiClientSecure *clientsecure = (WiFiClientSecure *)(client);

			// verify a certificate fingerprint against a fingerprint saved in eeprom
			readEEPROM(tstr, EEPROM_CERTFINGERPRINT, FINGERPRINTSIZE);
			#ifdef DEBUG_H
				Serial.print("fingerprint loaded from eeprom : ");
				Serial.println(tstr);
			#endif
			if (clientsecure->verify(tstr, gearauth)) {
				#ifdef DEBUG_H
					Serial.println("fingerprint matched");
				#endif
			}
			else {
				#ifdef DEBUG_H
					Serial.println("fingerprint mismatched, going to update");
				#endif
				AuthClient::randomString(nonce,8);
				sprintf(tstr,"GET /api/fingerprint/%s/%s HTTP/1.1\r\n\r\n",this->gearkey,nonce);
				clientsecure->write((const uint8_t *)tstr,strlen(tstr));
				delay(800);
				getHTTPReply(clientsecure,tstr,200);
				tstr[FINGERPRINTSIZE-1] = '\0';        // split fingerprint and signature
				sprintf(hashkey,"%s&%s&%s",this->gearkey,this->gearsecret,nonce);
				Sha1.initHmac((uint8_t*)hashkey,strlen(hashkey));
				Sha1.HmacBase64(hash, tstr);
				for (int i=0;i<HMACSIZE;i++)
					if (hash[i]=='/') hash[i] = '_';

				if(strcmp(hash,tstr+FINGERPRINTSIZE)==0) {
					#ifdef DEBUG_H
						Serial.println("new fingerprint updated");
					#endif
					writeEEPROM(tstr, EEPROM_CERTFINGERPRINT, FINGERPRINTSIZE);
				}
				else {
					#ifdef DEBUG_H
						Serial.println("fingerprint verification failed, abort");
					#endif
					clientsecure->stop();
					delay(5000);
					return;
				}
			}
		}

		strcpy(tstr,"GET /api/time HTTP/1.1\r\n\r\n");
		client->write((const uint8_t *)tstr,strlen(tstr));

		delay(1000);
		getHTTPReply(client,tstr,200);
		*bts = atol(tstr) - millis()/1000;
		client->stop();
	}
}

MicroGear::MicroGear(Client& netclient ) {
    sockclient = &netclient;
    constate = MQTTCLIENT_NOTCONNECTED;
    authclient = NULL;
	mqttclient = NULL;

    this->securemode = DEFAULTSECUREMODE;

    this->token = NULL;
    this->tokensecret = NULL;
    this->backoff = 10;
    this->retry = RETRY;

    strcpy(this->gearauth,GEARAUTHHOST);
    this->gearauth[MAXGEARAUTHSIZE] = '\0';

    this->eepromoffset = 0;
    cb_message = NULL;
    cb_connected = NULL;
    cb_absent = NULL;
    cb_present = NULL;
    cb_error = NULL;
    cb_info = NULL;

    eepromready = false;

    mg = (MicroGear *)this;
}

void MicroGear::on(unsigned char event, void (* callback)(char*, uint8_t*,unsigned int)) {
    switch (event) {
        case MESSAGE : 
                if (callback) cb_message = callback;
                break;
        case PRESENT : 
                if (callback) cb_present = callback;
                if (connected())
                    subscribe("/&present");
                break;
        case ABSENT : 
                if (callback) cb_absent = callback;
                if (connected())
                    subscribe("/&absent");
                break;
        case CONNECTED : 
                if (callback) cb_connected = callback;
                break;
        case ERROR : 
                if (callback) cb_error = callback;
                break;
        case INFO : 
                if (callback) cb_info = callback;
                break;
    }
}

void MicroGear::setEEPROMOffset(int val) {
    this->eepromoffset = val;
}

void MicroGear::readEEPROM(char* buff,int offset,int len) {
    int i;
    for (i=0;i<len;i++) {
        buff[i] = (char)EEPROM.read(this->eepromoffset+offset+i);
    }
    buff[len] = '\0';
}

void MicroGear::writeEEPROM(char* buff,int offset,int len) {
    int i;
    for (i=0;i<len;i++) {
        EEPROM.write(this->eepromoffset+offset+i,buff[i]);
    }
    EEPROM.commit();
}

void MicroGear::resetToken() {
    char state[2];
    if (!eepromready) {
        EEPROM.begin(this->eepromoffset+256);
        eepromready = true;
    }
   if (eepromready) {
            readEEPROM(state,EEPROM_STATEOFFSET,1);
            if (state[0] == EEPROM_STATE_ACC) {
                char pstr[200];
                char token[TOKENSIZE+1];
                char revokecode[REVOKECODESIZE+1];
                int port = this->securemode?GEARAUTHSECUREPORT:GEARAUTHPORT;
                
                if(sockclient->connect(gearauth,port)){
					readEEPROM(token,EEPROM_TOKENOFFSET,TOKENSIZE);
					readEEPROM(revokecode,EEPROM_REVOKECODEOFFSET,REVOKECODESIZE);
					sprintf(pstr,"GET /api/revoke/%s/%s HTTP/1.1\r\n\r\n",token,revokecode);
					sockclient->write((const uint8_t *)pstr,strlen(pstr));

					delay(1000);
					getHTTPReply(sockclient,pstr,200);

					if (strcmp(pstr,"FAILED")!=0) {    
						*state = EEPROM_STATE_NUL;
						writeEEPROM(state,EEPROM_STATEOFFSET,1);
					}
					sockclient->stop();
				}
            }
            else { 
                *state = EEPROM_STATE_NUL;
                writeEEPROM(state,EEPROM_STATEOFFSET,1);
            }
    } 
}


bool MicroGear::getToken(char *gkey, char *galias, char* token, char* tokensecret, char *endpoint) {
    char state[2], tstate[2];
    int authstatus = 0;

    char ekey[KEYSIZE+1];
    char rtoken[TOKENSIZE+1];
    char rtokensecret[TOKENSECRETSIZE+1];
    char flag[FLAGSIZE+1];
    char buff[2*TOKENSECRETSIZE+2];
    char revokecode[REVOKECODESIZE+1];

    token[0] = '\0';
    tokensecret[0] = '\0';

    readEEPROM(ekey,EEPROM_KEYOFFSET,KEYSIZE);
    if (strncmp(gkey,ekey,KEYSIZE)!=0) resetToken();

    readEEPROM(state,EEPROM_STATEOFFSET,1);
    #ifdef DEBUG_H
        Serial.print("Token rom state == ");
        Serial.print(state);
        Serial.println(";");
    #endif
    delay(500);
    // if token is null  --> get a requesttoken
    if (*state != EEPROM_STATE_REQ && *state != EEPROM_STATE_ACC) {

        #ifdef DEBUG_H
            Serial.println("need a req token.");
        #endif

        do {
            delay(2000);
            if (authclient->connect(this->securemode)) {

                #ifdef DEBUG_H
                    Serial.println("authclient is connected");
                #endif

                authstatus = authclient->getGearToken(_REQUESTTOKEN,rtoken,rtokensecret,endpoint,flag,gearkey,gearsecret,galias,scope,NULL,NULL);
                delay(1000);
                #ifdef DEBUG_H
                    Serial.println(authstatus); Serial.println(rtoken); Serial.println(rtokensecret); Serial.println(endpoint);
                #endif
                authclient->stop();

                if (authstatus == 200) {
                    *state = EEPROM_STATE_REQ;
                    writeEEPROM(state,EEPROM_STATEOFFSET,1);
                    writeEEPROM(gearkey,EEPROM_KEYOFFSET,KEYSIZE);
                    writeEEPROM(rtoken,EEPROM_TOKENOFFSET,TOKENSIZE);
                    writeEEPROM(rtokensecret,EEPROM_TOKENSECRETOFFSET,TOKENSECRETSIZE);
                    #ifdef DEBUG_H
                        Serial.println("@ Write Request Token");
                        Serial.println(state);
                        Serial.println(rtoken);
                        Serial.println(rtokensecret);
                    #endif
                }
            }
            else {
                #ifdef DEBUG_H
                    Serial.println("authclient is disconnected");
                #endif
                //delay(2000);
                return false;
            }    
        } while (!authstatus);
    }

    //if token is a requesttoken --> get an accesstoken
    if (*state == EEPROM_STATE_REQ) {

        readEEPROM(rtoken,EEPROM_TOKENOFFSET,TOKENSIZE);
        readEEPROM(rtokensecret,EEPROM_TOKENSECRETOFFSET,TOKENSECRETSIZE);
        
        do {
            delay(1000);
            authstatus = 0;
            while(authstatus==0) {
                if (authclient->connect(this->securemode)) {
                    #ifdef DEBUG_H
                        Serial.println("authclient is connected, get access token");
                    #endif
                    authstatus = authclient->getGearToken(_ACCESSTOKEN,token,tokensecret,endpoint,flag,gearkey,gearsecret,galias,"",rtoken,rtokensecret);
                    delay(1000);
                    authclient->stop();
                    delay(1000);
                }
                else {
                    #ifdef DEBUG_H
                        Serial.println("authclient is disconnected");
                    #endif
	                authclient->stop();
                    delay(1000);
					break;
                }
            }
            #ifdef DEBUG_H
                Serial.println(authstatus);
                Serial.print("flag = ");
                Serial.println(flag[0]);
            #endif

            // still write an EEPROM ic case of flag S 
            if (authstatus == 200) {
                *state = EEPROM_STATE_ACC;

                // if return with session flag mark eeprom token as null
                if (flag[0] != 'S') *tstate = EEPROM_STATE_ACC;
                else *tstate = EEPROM_STATE_NUL;

                writeEEPROM(tstate,EEPROM_STATEOFFSET,1);
                writeEEPROM(gearkey,EEPROM_KEYOFFSET,KEYSIZE);
                writeEEPROM(token,EEPROM_TOKENOFFSET,TOKENSIZE);
                writeEEPROM(tokensecret,EEPROM_TOKENSECRETOFFSET,TOKENSECRETSIZE);

                //Calculate revokecode
                sprintf(buff,"%s&%s",tokensecret,gearsecret);
                Sha1.initHmac((uint8_t*)buff,strlen(buff));
                Sha1.HmacBase64(revokecode, token);
            
                for (int i=0;i<REVOKECODESIZE;i++)
                    if (revokecode[i]=='/') revokecode[i] = '_';
                writeEEPROM(revokecode,EEPROM_REVOKECODEOFFSET,REVOKECODESIZE);

                char *p = endpoint+12;
                while (*p != '\0') {
                    if (*p == '%') {
                        *p = *(p+1)>='A'?16*(*(p+1)-'A'+10):16*(*(p+1)-'0');
                        *p += *(p+2)>='A'?(*(p+2)-'A'+10):(*(p+2)-'0');
                        strcpy(p+1,p+3);
                    }
                    p++;
                }
                writeEEPROM(endpoint+12,EEPROM_ENDPOINTSOFFSET,MAXENDPOINTLENGTH);
            }
            else {
                if (authstatus != 401) {
                    #ifdef DEBUG_H
                        Serial.println("Return code is not 401");
                        Serial.println(authstatus);
                    #endif

                    /* request token error e.g. revoked */
                    *state = EEPROM_STATE_NUL;
                    writeEEPROM(state,EEPROM_STATEOFFSET,1);
                    break;
                }
            }
        }while (*state == EEPROM_STATE_REQ);
        // reset accesstoken retry counter
        retry = RETRY;
        #ifdef DEBUG_H
            Serial.println(authstatus); Serial.println(token); Serial.println(tokensecret); Serial.println(endpoint);
        #endif
    }

    //if token is a requesttoken --> get an accesstoken
    if (*state == EEPROM_STATE_ACC) {
        readEEPROM(token,EEPROM_TOKENOFFSET,TOKENSIZE);
        readEEPROM(tokensecret,EEPROM_TOKENSECRETOFFSET,TOKENSECRETSIZE);
        readEEPROM(endpoint,EEPROM_ENDPOINTSOFFSET,MAXENDPOINTLENGTH);
    }
    
    authclient->stop();

    if (*state != EEPROM_STATE_ACC) {
        #ifdef DEBUG_H
            Serial.println("Fail to get a token.");
        #endif
        //delay(2000);
        return false;
    }
    return true;
}

int MicroGear::connectBroker(char* appid) {
    char username[USERNAMESIZE+1];
    char password[PASSWORDSIZE+1];
    char buff[2*TOKENSECRETSIZE+2];
    char token[TOKENSIZE+1];
    char tokensecret[TOKENSECRETSIZE+1];
    char endpoint[MAXENDPOINTLENGTH+1];
    int gbport;
    bool tokenOK;

    syncTime(sockclient, &bootts);

    #ifdef DEBUG_H
        Serial.print("Time stamp : ");
        Serial.println(bootts);
    #endif

    this->appid = appid;
    topicprefixlen = strlen(appid)+1;

    if (mqttclient) {
        // recently disconnected with code 4
        if (mqttclient->state() == 4) {
            resetToken();
        }
        delete(mqttclient);
    }

    if (authclient) delete(authclient);
    authclient = new AuthClient(*sockclient);
    authclient->init(gearauth,appid,scope,bootts);
    
    tokenOK = getToken(this->gearkey,this->gearalias,token,tokensecret,endpoint);
    delete(authclient);
    authclient = NULL;
    
    if (tokenOK && *token && *tokensecret) {

        /* if endpoint is empty, request a new one */
        initEndpoint(sockclient, endpoint);

        /* generate one-time user/password */
        sprintf(username,"%s%%%s%%%lu",token,gearkey,bootts+millis()/1000);
        sprintf(buff,"%s&%s",tokensecret,gearsecret);
        Sha1.initHmac((uint8_t*)buff,strlen(buff));
        Sha1.HmacBase64(password, username);

        #ifdef DEBUG_H
            Serial.println("Going to connect to MQTT broker");
            Serial.println(token);
            Serial.println(username);
            Serial.println(password);
            Serial.println(endpoint);
        #endif

        char *p = endpoint;
        while (*p!='\0') {
            if (*p == ':') {
                *p = '\0';
                p++;
                break;
            }
            p++;
        }

        gbport = this->securemode?GBSECUREPORT:GBPORT;
        mqttclient = new PubSubClient(endpoint, gbport, msgCallback, *sockclient);
        #ifdef DEBUG_H
            Serial.print("Connecting to : ");
            Serial.print(endpoint);
            Serial.print(":");
            Serial.println(gbport);
        #endif
        delay(500);
        
        constate = this->mqttclient->connect(token,username+TOKENSIZE+1,password);
        switch (constate) {
            case MQTTCLIENT_CONNECTED :
                    backoff = MINBACKOFFTIME;
                    if (cb_present)
                        subscribe("/&present");
                    if (cb_absent)
                        subscribe("/&absent");

                    sprintf(buff,"/&id/%s/#",token);
                    subscribe(buff);

                    if (cb_connected)
                        cb_connected(NULL,NULL,0);

                    return NETPIECLIENT_CONNECTED;

            case MQTTCLIENT_NOTCONNECTED :
                    if (backoff < MAXBACKOFFTIME) backoff = 2*backoff;
                    delay(backoff);
                    return NETPIECLIENT_NOTCONNECTED;
        }
    }
    else return NETPIECLIENT_TOKENERROR;
}

void MicroGear::useTLS(bool usetls) {
    this->securemode = usetls;
}

int MicroGear::connect(char* appid) {
    return connectBroker(appid);
}

bool MicroGear::connected() {
    //if (constate == CLIENT_NOTCONNECT) return CLIENT_NOTCONNECT;
    //else return this->mqttclient->connected();
    return this->sockclient->connected();
}

void MicroGear::subscribe(char* topic) {
    char top[MAXTOPICSIZE] = "/";

    strcat(top,appid);
    strcat(top,topic);
    mqttclient->subscribe(top);
}

void MicroGear::unsubscribe(char* topic) {
    char top[MAXTOPICSIZE] = "/";

    strcat(top,appid);
    strcat(top,topic);
    mqttclient->unsubscribe(top);
}

bool MicroGear::publish(char* topic, char* message, bool retained) {
    char top[MAXTOPICSIZE] = "/";

    strcat(top,appid);
    strcat(top,topic);
    return mqttclient->publish(top, message, retained);
}

bool MicroGear::publish(char* topic, char* message) {
    return publish(topic, message, false);
}

bool MicroGear::publish(char* topic, double message, int n) {
    return publish(topic, message, n, false);
}

bool MicroGear::publish(char* topic, double message, int n, bool retained) {
    char mstr[16];
    dtostrf(message,0,n,mstr);
    return publish(topic, mstr, retained);
}

bool MicroGear::publish(char* topic, double message) {
    return publish(topic, message, 8, false);
}

bool MicroGear::publish(char* topic, double message, bool retained) {
    return publish(topic, message, 8, retained);
}

bool MicroGear::publish(char* topic, int message) {
    return publish(topic, message, 0, false);
}

bool MicroGear::publish(char* topic, int message, bool retained) {
    return publish(topic, message, 0, retained);
}

bool MicroGear::publish(char* topic, String message) {
    return publish(topic, message, false);
}

bool MicroGear::publish(char* topic, String message, bool retained) {
    char buff[MAXBUFFSIZE];
    message.toCharArray(buff,MAXBUFFSIZE-1);
    return publish(topic, buff, retained);
}

bool MicroGear::publish(char* topic, String message, String apikey) {
    char buff[MAXBUFFSIZE];
    message.toCharArray(buff,MAXBUFFSIZE-1);
    
	char top[MAXTOPICSIZE] = "";
	strcat(top,topic);
	if(apikey!=""){
		strcat(top,"/");
		char buffapikey[MAXBUFFSIZE];
		apikey.toCharArray(buffapikey,MAXBUFFSIZE);
		strcat(top,buffapikey);
	}
    return publish(top, buff);
}

bool MicroGear::publish(char* topic, String message, char* apikey) {
    char buff[MAXBUFFSIZE];
    message.toCharArray(buff,MAXBUFFSIZE-1);
    
	char top[MAXTOPICSIZE] = "";
	strcat(top,topic);
	if(apikey!=""){
		strcat(top,"/");
		strcat(top,apikey);
	}
    return publish(top, buff);
}

bool MicroGear::writeFeed(char* feedname, char *data, char* apikey) {
    char buff[MAXBUFFSIZE] = "/@writefeed/";
    
	strcat(buff,feedname);
	if(apikey!=NULL && strlen(apikey)>0){
		strcat(buff,"/");
		strcat(buff,apikey);
	}
    return publish(buff, data);
}

bool MicroGear::writeFeed(char* feedname, char *data) {
    return writeFeed(feedname, data, NULL);
}

bool MicroGear::writeFeed(char* feedname, String data, char* apikey) {
    char buff[MAXBUFFSIZE];
    data.toCharArray(buff,MAXBUFFSIZE-1);
    
    return writeFeed(feedname, buff, apikey);
}

bool MicroGear::writeFeed(char* feedname, String data) {
    return writeFeed(feedname, data, NULL);
}

/*
  setName() is deprecated 
*/
void MicroGear::setName(char* gearname) {
    char top[MAXTOPICSIZE];
    if (this->gearname) {
        strcpy(top,"/gearname/");
        strcat(top,this->gearname);
        unsubscribe(top);
    }
    strcpy(top,"/gearname/");
    strcat(top,gearname);
    this->gearname = gearname;

    subscribe(top); 
}

void MicroGear::setAlias(char* gearalias) {
    char top[MAXTOPICSIZE];
    strcpy(top,"/@setalias/");
    strcat(top,gearalias);
    this->gearalias = gearalias;
    publish(top,"");
}

bool MicroGear::chat(char* targetgear, char* message) {
    bool result;
    char top[MAXTOPICSIZE];

    sprintf(top,"/%s/gearname/%s",appid,targetgear);
    result = mqttclient->publish(top, message);
    mqttclient->loop();
    return result;
}

bool MicroGear::chat(char* topic, double message, int n) {
    bool result;
    char mstr[16];

    dtostrf(message,0,n,mstr);
    result = chat(topic, mstr);
    mqttclient->loop();
    return result;
}

bool MicroGear::chat(char* topic, double message) {
    return chat(topic, message, 8);
}

bool MicroGear::chat(char* topic, int message) {
    return chat(topic, message, 0);
}

bool MicroGear::chat(char* topic, String message) {
    char buff[MAXBUFFSIZE];
    message.toCharArray(buff,MAXBUFFSIZE);
    return chat(topic, buff);
}

int MicroGear::init(char* gearkey,char* gearsecret) {
    init(gearkey,gearsecret,"","");
}

int MicroGear::init(char* gearkey,char* gearsecret,char* gearalias) {
    init(gearkey,gearsecret,gearalias,"");
}

int MicroGear::init(char* gearkey,char* gearsecret,char* gearalias, char* scope) {
    this->gearkey = gearkey;
    this->gearsecret = gearsecret;
    this->gearalias = gearalias;
    this->scope = scope;

    if (!eepromready) {
        EEPROM.begin(this->eepromoffset+256);
        eepromready = true;
    }
}

void MicroGear::loop() {
    this->mqttclient->loop();
}

void MicroGear::setToken(char *key, char* token,char* tokensecret) {
    char state[2];
    this->token = token;
    this->tokensecret = tokensecret;
    *state = EEPROM_STATE_ACC;
    writeEEPROM(state,EEPROM_STATEOFFSET,1);
    writeEEPROM(key,EEPROM_KEYOFFSET,KEYSIZE);
    writeEEPROM(token,EEPROM_TOKENOFFSET,TOKENSIZE);
    writeEEPROM(tokensecret,EEPROM_TOKENSECRETOFFSET,TOKENSECRETSIZE);
}

void MicroGear::strcat(char* a, char* b) {
    char *p;
    p = a + strlen(a);
    strcpy(p,b);
}

int MicroGear::state() {
    if (!mqttclient) return -9;
    else return this->mqttclient->state();
}

int MicroGear::setConfig(char* key, char* value) {
    if (strcmp(key,"GEARAUTH")==0) {
        strncpy(gearauth,value,MAXGEARAUTHSIZE);
        return 1;
    }
    else {
        return 0;
    }
}
