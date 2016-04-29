// RealTimeClock.h

#ifndef _REALTIMECLOCK_h
#define _REALTIMECLOCK_h

#include <ESP8266WiFi.h>
#include <NTPClient.h>
//#include <ArduinoJson.h>

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


class RealTimeClock
{
public:
	RealTimeClock();
	~RealTimeClock();
	unsigned long GetTime();
	bool Update();

private:
	const unsigned long UPDATE_TIME_INTERVAL = 3600000; // 60 minutes
	unsigned long gLastUpdatedTime = 0;
	unsigned long gLastTime = 0;
	
	const char* BUFFER_TIME_CLIENT[sizeof(NTPClient)];
	const char* TIME_SERVER = "time.nist.gov";
	NTPClient* gTimeClient;

	void SetupNtp();
	bool UpdateTime(bool pForceUpdate);
	bool ShouldUpdateTime();
	bool ShouldUpdateTime(int pFactor);
};

#endif

