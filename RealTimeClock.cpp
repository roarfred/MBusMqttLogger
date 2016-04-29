// 
// 
// 

#include "RealTimeClock.h"

RealTimeClock::RealTimeClock()
{
	gTimeClient = new (BUFFER_TIME_CLIENT) NTPClient(TIME_SERVER, 0, 0);
	SetupNtp();
}
RealTimeClock::~RealTimeClock() 
{
	//delete(gTimeClient);
}
unsigned long RealTimeClock::GetTime()
{
	UpdateTime(false);

	// I first thought we had to implement rollover code, but the
	// beauty of the unsigned variables just so happens to smile 
	// (the result will be the remainder plus the value anyway)
	return gLastTime + (millis() - gLastUpdatedTime) / 1000;
}

bool RealTimeClock::Update()
{
	return UpdateTime(false);
}
void RealTimeClock::SetupNtp()
{
	Serial.print(F("Connecting to NTP server"));
	while (!UpdateTime(true)) {
		Serial.print(".");
	}
	Serial.println("");
}
bool RealTimeClock::UpdateTime(bool pForceUpdate)
{
	if (pForceUpdate)
	{
		Serial.println(F("Force time update..."));
		gTimeClient->update();
		if (gTimeClient->getRawTime() > 1000)
		{
			gLastUpdatedTime = millis();
			gLastTime = gTimeClient->getRawTime();
			Serial.print("Time is ");
			Serial.println(gTimeClient->getFormattedTime());
			return true;
		}
		else
			return false;
	}
	else
	{
		if (ShouldUpdateTime())
		{
			Serial.println(F("Updating time now..."));
			if (!UpdateTime(true))
			{
				Serial.println(F("Failed to update time..."));
				if (ShouldUpdateTime(10))
				{
					Serial.println(F("ERROR updating time for a long time now..."));
					// It's been way too long since last update
					return false;
				}
			}
			else
			{
				Serial.print("Time is ");
				Serial.println(gTimeClient->getFormattedTime());
			}
		}
		return true;
	}
}
bool RealTimeClock::ShouldUpdateTime()
{
	return ShouldUpdateTime(1);
}
bool RealTimeClock::ShouldUpdateTime(int pFactor)
{
	if (gLastUpdatedTime == 0)
		return true;
	else if (gLastUpdatedTime > millis())
		return true; // Millis counter just past the max value
	else if ((millis() - gLastUpdatedTime) > (UPDATE_TIME_INTERVAL * (unsigned long)pFactor))
		return true;
	else
	{
		/*
		Serial.print("Seconds to next clock update: ");
		Serial.println((UPDATE_TIME_INTERVAL - (millis() - gLastUpdatedTime)) / 1000);
		*/
		return false;
	}
}

//RealTimeClock::gTimeClient("time.nist.gov");