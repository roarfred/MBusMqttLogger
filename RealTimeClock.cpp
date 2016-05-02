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
	Serial1.print("Connecting to NTP server");
	while (!UpdateTime(true)) {
		Serial1.print(".");
	}
	Serial1.println("");
}
bool RealTimeClock::UpdateTime(bool pForceUpdate)
{
	if (pForceUpdate)
	{
		Serial1.println("Force time update...");
		gTimeClient->update();
		if (gTimeClient->getRawTime() > 1000)
		{
			gLastUpdatedTime = millis();
			gLastTime = gTimeClient->getRawTime();
			Serial1.println("Time is ");
			Serial1.println(gTimeClient->getFormattedTime().c_str());
			return true;
		}
		else
			return false;
	}
	else
	{
		if (ShouldUpdateTime())
		{
			Serial1.println("Updating time now...");
			if (!UpdateTime(true))
			{
				Serial1.println("Failed to update time...");
				if (ShouldUpdateTime(10))
				{
					Serial1.println("ERROR updating time for a long time now...");
					// It's been way too long since last update
					return false;
				}
			}
			else
			{
				Serial1.println("Time is ");
				Serial1.println(gTimeClient->getFormattedTime());
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
		return false;
	}
}
