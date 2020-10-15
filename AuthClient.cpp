/*
  AuthClient Library
   NetPIE Project
   http://netpie.io
*/

#include "AuthClient.h"
#include <Arduino.h>

AuthClient::AuthClient(Client& iclient) {
    client = &iclient;
}

AuthClient::~AuthClient() {

}

void AuthClient::init(char *authendpoint, char* appid, char* scope, unsigned long bts) {
    this->authendpoint = authendpoint;
    this->appid = appid;
    this->scope = scope;
    this->bootts = bts;
}

bool AuthClient::connect(bool issecuremode) {
    int port = issecuremode?GEARAUTHSECUREPORT:GEARAUTHPORT;
    this->securemode = issecuremode;
    if (client->connect(authendpoint,port)) {
        return true;
    }
    else {
        return false;
    }
}

void AuthClient::stop() {
    client->stop();
}

/**
 * [AuthClient::writeout write string to client
 * @param str     text
 * @param newline append new line character?
 * @param progmem true:copy from prog memory, false:copy from sram
 */
void AuthClient::writeout(char* str, bool newline, bool progmem) {
    unsigned char len;
    uint8_t ch, buff[MAXHEADERLINESIZE];

    #ifdef DEBUG_H
        Serial.print("WRITE---->");
        Serial.println(str);
    #endif

    if (!client) return;
    len = 0;
    if (str) {
        len = strlen(str);
        strcpy((char *)buff, str);
    }
    if (newline) {
        buff[len++] = '\r';
        buff[len++] = '\n';
    }
    client->write(buff,len);
}

void AuthClient::write(char* str) {
  writeout(str, false, false);
}

void AuthClient::writeln(char* str) {
  writeout(str, true, false);
}

bool AuthClient::readln(char *buffer, size_t buflen) {
    size_t pos = 0;
    while (true) {
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
                return true; 
            } 
        }
    }
    return false;
}

bool processTok(char* key, char* buff, char **p) {
    char *r;
    char len = strlen(key);

    if (strncmp(*p,key,len)==0) {
        *p += len;
        r = buff;
        while (**p!='\0' && **p!='&' && **p!='\r' && **p!='\n')
            *r++ = *(*p)++;

        *r = '\0';
        if (**p=='&') *p++;
        return true;
    }
    else return false;
}

char* AuthClient::append(char* buffer, char* mstr, char stopper) {
    /* strcat(buffer,mstr) causes a strange bug */
    strcat(buffer,mstr);
    if (stopper) {
        int len = strlen(buffer);
        buffer[len] = stopper;
        buffer[len+1] = '\0';
    }
    return buffer;
}

void AuthClient::addParam(char* buff, char* key, char* value, bool first) {
    if (!first) strcat(buff,"%26");
    strcat(buff,key);
    strcat(buff,"%3D");
    encode(strtail(buff),value);
}

void AuthClient::randomString(char *nonce, int size) {
    const char* noncealpha = "0123456789abcdefghijklmnopqrstuvwxyz";

    randomSeed(analogRead(0));
    for (int i=0;i<size;i++) {
        nonce[i] = *(noncealpha+random(0,35));
    }
    nonce[size] = '\0';
}

int AuthClient::getGearToken(char mode, char* token, char* tokensecret, char* endpoint, char *flag, char* gearkey, char* gearsecret, char* gearalias, char* scope, char* rtoken, char* rtokensecret) {
        #ifdef DEBUG_H
            Serial.println("Enter getGearToken()..");
        #endif

        char buff[MAXHEADERLINESIZE];
        char signbase[MAXSIGNATUREBASELENGTH];
        char nonce[NONCESIZE+1];
        char verifier[32+1];

         #define OAUTH_CALLBACK  "oauth_callback="
         #define OAUTH_CONSUMER_KEY  "oauth_consumer_key="
         #define OAUTH_NONCE  "oauth_nonce="
         #define OAUTH_SIGNATURE_METHOD_CONST  "oauth_signature_method=\"HMAC-SHA1\""
         #define OAUTH_TIMESTAMP  "oauth_timestamp="
         #define OAUTH_VERSION_CONST  "oauth_version=\"1.0\""
         #define OAUTH_SIGNATURE  "oauth_signature="
         #define OAUTH_TOKEN  "oauth_token="
         #define OAUTH_VERIFIER  "oauth_verifier="

        *token = '\0';
        *tokensecret = '\0';
        *endpoint = '\0';
        *flag = '\0';

        strcpy(signbase,this->securemode?"POST&https%3A%2F%2F":"POST&http%3A%2F%2F");
        sprintf(strtail(signbase),"%s%%3A%d",authendpoint,this->securemode?GEARAUTHSECUREPORT:GEARAUTHPORT);

        if (mode == _REQUESTTOKEN) {
            writeln("POST /api/rtoken HTTP/1.1");
            strcat(signbase,"%2Fapi%2Frtoken&");
        }
        else {
            writeln("POST /api/atoken HTTP/1.1");
            strcat(signbase,"%2Fapi%2Fatoken&");
        }
        sprintf(buff,"Host: %s:%d",authendpoint,this->securemode?GEARAUTHSECUREPORT:GEARAUTHPORT);
        writeln(buff);

        write("Authorization: OAuth ");

        //OAUTH_CALLBACK
        /* this header is too long -- have to break into smaller chunks to write */
        *buff = '\0';
        append(buff,OAUTH_CALLBACK,0);
        strcat(buff,"\"appid%3D");
        strcat(buff,appid);

        strcat(buff,"%26mgrev%3D");
        strcat(buff,MGREV);
        write(buff);        

        *buff = '\0';
        strcat(buff,"%26scope%3D");
        strcat(buff,scope);

        verifier[0] = '\0';
        // oauth verifier must longer tahn 2 characters
        if (strlen(gearalias)>2) {
            strcat(verifier,gearalias);
        }
        else {
            strcat(verifier,MGREV);
        }

        strcat(buff,"%26verifier%3D");
        strcat(buff,verifier);
        strcat(buff,"\",");
        write(buff);

        *buff = '\0';
        append(buff,OAUTH_CALLBACK,0);
        encode(strtail(signbase),buff);

        /*add key value*/
        buff[0] = '\0';
        addParam(buff,"appid",appid,1);
        addParam(buff,"mgrev",MGREV,0);
        addParam(buff,"scope",scope,0);
        addParam(buff,"verifier",verifier,0);
        encode(strtail(signbase),buff);

        //OAUTH_CONSUMER_KEY
        *buff = '\0';
        append(buff,OAUTH_CONSUMER_KEY,0);
        //sprintf(strtail(buff),"\"%s\"",gearkey);

        strcat(buff,"\"");
        strcat(buff,gearkey);
        strcat(buff,"\"");

        strcat(signbase,"%26"); //&
        encode(strtail(signbase),buff);

        strcat(buff,",");
        write(buff);

        //OAUTH_NONCE
/*
        randomSeed(analogRead(0));
        for (char i=0;i<NONCESIZE;i++) {
            nonce[i] = *(noncealpha+random(0,35));
        }
        nonce[NONCESIZE] = '\0';
*/

        randomString(nonce, NONCESIZE);

        *buff = '\0';
        append(buff,(char *)OAUTH_NONCE,0);
        sprintf(strtail(buff),"\"%s\"",nonce);
        strcat(signbase,"%26");  //&
        encode(strtail(signbase),buff);

        strcat(buff,",");
        write(buff);

        //OAUTH_SIGNATURE_METHOD
        *buff = '\0';
        append(buff,(char *)OAUTH_SIGNATURE_METHOD_CONST,0);
        strcat(signbase,"%26"); //&
        encode(strtail(signbase),buff);

        strcat(buff,",");
        write(buff);

        //OAUTH_TIMESTAMP
        *buff = '\0';
        append(buff,(char *)OAUTH_TIMESTAMP,0);

        //Bypass NTP
        sprintf(strtail(buff),"\"%lu\"",bootts+millis()/1000);

        strcat(signbase,"%26"); //&
        encode(strtail(signbase),buff);

        strcat(buff,",");
        write(buff);

        if (rtoken) {   
            //OAUTH_TOKEN
            *buff = '\0';
            append(buff,(char *)OAUTH_TOKEN,0);
            sprintf(strtail(buff),"\"%s\"",rtoken);
            strcat(signbase,"%26"); //&
            encode(strtail(signbase),buff);

            strcat(buff,",");
            write(buff);

            //OAUTH_VERIFIER
            *buff = '\0';
            append(buff,(char *)OAUTH_VERIFIER,0);
            sprintf(strtail(buff),"\"%s\"",verifier);
            strcat(signbase,"%26"); //&
            encode(strtail(signbase),buff);

            strcat(buff,",");
            write(buff);
        }

        //OAUTH_VERSION
        *buff = '\0';
        append(buff,(char *)OAUTH_VERSION_CONST,0);
        strcat(signbase,"%26"); //&
        encode(strtail(signbase),buff);

        strcat(buff,",");
        write(buff);

        // Calculate HMAC-SHA1(signbase) and include in the Authorization header
        //OAUTH_SIGNATURE
        char hkey[66];
        char signature[29];
       *buff = '\0';
        append(buff,(char *)OAUTH_SIGNATURE,'"');
        sprintf(hkey,"%s&%s",gearsecret,rtokensecret?rtokensecret:"");

        Sha1.initHmac((uint8_t*)hkey,strlen(hkey));
        Sha1.HmacBase64(signature, signbase);

        encode(strtail(buff),signature);
        strcat(buff,"\"");

        strcat(buff,"\r\n");
        write(buff);

        writeln("Accept: */*");
        writeln("Connection: close");
        writeln("User-Agent: E8A");
        writeln("Content-length: 0");
        writeln("Content-Type: application/x-www-form-urlencoded");
        writeln(NULL);

        #ifdef DEBUG_H
            Serial.println("Finish OAuth HTTP request..");
        #endif

        delay(2000);

        int httpstatus = 0;
        char pline = 0;
        while(true) {
            if (readln(buff, (size_t)MAXHEADERLINESIZE)) {

                    #ifdef DEBUG_H
                        Serial.println(buff);
                    #endif
                    if (httpstatus==0) {
                        if (strncmp(buff,"HTTP",4)==0) {
                            buff[12] = '\0';
                            httpstatus = atoi(buff+9);
                        }
                    }
                    else {
                           if (strlen(buff) < 1) {
                                readln(buff, (size_t)MAXHEADERLINESIZE);
                                char *p;
                                char *s, *t;
                                int last = 0;
                                p = buff;
                                while (*p != '\0') {
                                    s = p;
                                    while (*s!='=' && *s!='\0' && *s!='&') s++; // seek for =
                                    t= s;
                                    while (*t!='\0' && *t!='&') t++;            // seek for &
                                    *s = '\0';
                                    if (*t == '\0') last = 1;
                                    *t = '\0';
                                    if (strcmp(p,"oauth_token")==0) strcpy(token,s+1);
                                    else if (strcmp(p,"oauth_token_secret")==0) strcpy(tokensecret,s+1);
                                    else if (strcmp(p,"endpoint")==0) strcpy(endpoint,s+1);
                                    else if (strcmp(p,"flag")==0) strcpy(flag,s+1);
                                    delay(50);
                                    if (!last) p = t+1; else break;
                                }
                                return httpstatus;
                            }
                    }
            }
            else {
                return 0;
            }
        }
        return httpstatus;
}

char* AuthClient::encode(char *buffer, char ch) {
    // ignore doublequotes that include in oauth header value
    if(ch == '"') return buffer;

    if ('0' <= ch && ch <= '9' || 'a' <= ch && ch <= 'z' || 'A' <= ch && ch <= 'Z' || ch == '-' || ch == '.' || ch == '_' || ch == '~') {
        *buffer++ = ch;
    }
    else {
        int val = ((int) ch) & 0xff;
        snprintf(buffer, 4, "%%%02X", val);
        buffer += 3;
    }
    *buffer = '\0';
    return buffer;
}

char* AuthClient::encode(char *buffer, char *data) {
    char* p = buffer;
    while (char ch = *data++) p = encode(p, ch);
    return buffer;
}

char* AuthClient::strtail(char* buff) {
    char *p;
    p = buff + strlen(buff);
    return p;
}

void AuthClient::strcat(char* a, char* b) {
    char *p;
    p = a + strlen(a);
    strcpy(p,b);
}
