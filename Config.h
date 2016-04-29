// Config.h

#ifndef _CONFIG_h
#define _CONFIG_h

#include <EEPROM.h>

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class Config
{
public:
	char* Ssid;
	char* Pwd;
	char* MqttServer;
	char* MqttTopic;
	char* MqttName;

	bool Save(int pAddress);
	bool Load(int pAddress);

protected:
	
private:
	const int EEPROM_SIZE = 512;
	const byte EEPROM_CHECK_SUM = 124; // Used to check if config is stored. Change if structure changes

	int SaveString(int pAddress, char* pString);
	int ReadString(int pAddress, char* pString[]);
};

#endif

