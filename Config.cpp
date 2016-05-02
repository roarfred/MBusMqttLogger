// 
// 
// 

#include "Config.h"


bool Config::Save(int pAddress)
{
	int vAddress = pAddress;

	EEPROM.begin(EEPROM_SIZE);
	EEPROM.put(vAddress, EEPROM_CHECK_SUM);
	vAddress++;

	vAddress += SaveString(vAddress, Ssid);
	vAddress += SaveString(vAddress, Pwd);
	vAddress += SaveString(vAddress, MqttServer);
	vAddress += SaveString(vAddress, MqttTopic);
	vAddress += SaveString(vAddress, MqttName);

	bool vRet = EEPROM.commit();
	EEPROM.end();
	
	return vRet;
}

int Config::SaveString(int pAddress, char* pString)
{
	int vAddress = 0;
	int vLength = strlen(pString) + 1;

	EEPROM.put(pAddress + vAddress, vLength);
	vAddress++;

	for (int i = 0; i<vLength; i++)
	{
		EEPROM.put(pAddress + vAddress, pString[i]);
		vAddress++;
	}

	return vAddress;
}

bool Config::Load(int pAddress)
{
	int vAddress = pAddress;
	bool vSuccess = false;

	EEPROM.begin(EEPROM_SIZE);
	if (EEPROM.read(vAddress) == EEPROM_CHECK_SUM)
	{
		vAddress++;
		Serial1.println("Found correct checksum of EEPROM");

		vAddress += ReadString(vAddress, &Ssid);
		vAddress += ReadString(vAddress, &Pwd);
		vAddress += ReadString(vAddress, &MqttServer);
		vAddress += ReadString(vAddress, &MqttTopic);
		vAddress += ReadString(vAddress, &MqttName);

		Serial1.print("Loaded stored configuration: [");
		Serial1.print(" SSID: ");
		Serial1.print(Ssid);
		Serial1.print(", Pwd: ");
		Serial1.print(Pwd);
		Serial1.print(", MQTT server: ");
		Serial1.print(MqttServer);
		Serial1.print(", Topic: ");
		Serial1.print(MqttTopic);
		Serial1.print(", Name: ");
		Serial1.print(MqttName);
		Serial1.print(" ]");

		vSuccess = true;
	}
	EEPROM.end();

	return vSuccess;
}


int Config::ReadString(int pAddress, char* pString[])
{
	int vAddress = 0;

	byte vLength = EEPROM.read(pAddress + vAddress);
	vAddress++;

	*pString = new char[vLength];
	for (int i = 0; i<vLength; i++)
	{
		(*pString)[i] = EEPROM.read(pAddress + vAddress++);
	}
	
	return vAddress;
}