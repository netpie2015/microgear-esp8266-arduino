#ifndef DEBUG_H
#define DEBUG_H
#endif

#define DEBUG_PRINT(str1)		    \
	Serial.println(str1);

#define DEBUG_LOG(str1)             \
	Serial.print("DEBUG : ");		\
	Serial.println(str1);

#define DEBUG_LOG2(str1,str2)       \
	Serial.print("DEBUG : ");		\
	Serial.print(str1);				\
	Serial.println(str2);

#define DEBUG_LOG3(str1,str2,str3)  \
	Serial.print("DEBUG : ");		\
	Serial.print(str1);				\
	Serial.print(str2);				\
	Serial.println(str3);

#define DEBUG_LOG4(str1,str2,str3,str4)  \
	Serial.print("DEBUG : ");			 \
	Serial.print(str1);					 \
	Serial.print(str2);					 \
	Serial.print(str3);					 \
	Serial.println(str4);
