// 
// 
// 

#include "PulsePort.h"



PulsePort::PulsePort(const char* pName, int pPin, int pPulsesPerKWh, unsigned long pMinPulseLength, int pAverageOverMinutes, int pEepromAddress)
{
	Name = pName;
	
	gPin = pPin;
	gMinPulseLength = pMinPulseLength;
	gPulsesPerKWh = pPulsesPerKWh;
	gAverageOverMinutes = pAverageOverMinutes;

	gEepromAddress = pEepromAddress;

	gLastPulse = 0;
	gLastPulseMillis = 0;
	gPreviousPulseMillis = 0;
	Ticks = 0;
	TotalTicks = 0;
}

PulsePort::~PulsePort() {

}

void PulsePort::Begin()
{
	pinMode(gPin, INPUT);
	attachInterrupt(gPin, GetISR(gPin), FALLING);
	PulsePort::Ports[PulsePort::PortCount++] = this;
	gStartTime = millis();
}
void PulsePort::End()
{
	bool vAnyOtherOnSamePin = false;
	bool vFound = false;
	for (int i = 0; i < PulsePort::PortCount; i++)
	{
		vAnyOtherOnSamePin |= (PulsePort::Ports[i] != this && PulsePort::Ports[i]->gPin == gPin);
		if (!vFound)
		{
			vFound = PulsePort::Ports[i] == this;
		}
		else
		{
			PulsePort::Ports[i - 1] = PulsePort::Ports[i];
		}
	}
	PulsePort::PortCount--;
	if (!vAnyOtherOnSamePin)
		detachInterrupt(gPin);
}

void PulsePort::CommitTicksToTotal()
{
	TotalTicks += Ticks;
	Ticks = 0;
}
void PulsePort::Pulse_ISR()
{
	delayMicroseconds(gMinPulseLength / 10); // Wait a 10th of the pulse and ensure the value is still there
	if (digitalRead(gPin) == LOW && (gLastPulse + gMinPulseLength < micros() || micros() < gLastPulse))
	{
		Ticks++;
		gLastPulse = micros();
		gPreviousPulseMillis = gLastPulseMillis;
		gLastPulseMillis = millis();
	}
}

float PulsePort::TicksInWattHours() 
{
	return (float)Ticks / (float)gPulsesPerKWh * 1000.0;
}
float PulsePort::TotalTicksInWattHours()
{
	return (float)TotalTicks / (float)gPulsesPerKWh * 1000.0;
}


void PulsePort::UpdateAverage()
{
	// Get current "minute" (e.g. position in average buffer)
	byte vMinute = ((millis() - gStartTime) / (gAverageOverMinutes * 1000)) % 60;
	
	// Just return if we haven't reached the next minute
	if (vMinute == gLastAverageDataIndex)
		return;

	// Get the end position of the buffer to fill
	byte vTo = vMinute;
	if (vTo < gLastAverageDataIndex)
	{
		gAverageDataFilled = true;
		vTo = vMinute + 60;
	}

	// Fill all spaces between with this data
	for (byte i = gLastAverageDataIndex ; i <= vTo; i++)
	{
		gAverageData[i % 60] = TotalTicks;
	}

	// Update the current index
	gLastAverageDataIndex = vTo % 60;
}

float PulsePort::GetAverage()
{
	// If we just started, return zero
	if (gLastAverageDataIndex <= 1 && !gAverageDataFilled)
		return 0.0f;

	float vSum = 0.0f;
	byte vStartIndex = 0;								// Assume starting at the first
	byte vEndIndex = gLastAverageDataIndex;				// and ending at the last index
	if (gAverageDataFilled)								// But, if we already filled the buffer
	{
		vStartIndex = (gLastAverageDataIndex + 1) % 60;	// The first item is the one after the index
		vEndIndex = vStartIndex + 59;
	}
	
	for (int i = vStartIndex + 1; i < vEndIndex; i++)
	{
		vSum += gAverageData[i % 60] - gAverageData[(i-1) % 60];
	}

	return vSum / (float)(vEndIndex - vStartIndex - 1) * 60.0 / (float)gPulsesPerKWh * 1000.0 / ((float)gAverageOverMinutes / 60.0);
}

float PulsePort::Current()
{
	unsigned long vTimeInMilliSeconds;
	if (gPreviousPulseMillis == 0)
	{
		return 0.0;
	}

	/* Should work even if gLastPulse is less than gPreviousPulse */
	vTimeInMilliSeconds = gLastPulseMillis - gPreviousPulseMillis;

	/*
	--- UNITS IN CALCULATION ---
	vValue				- W
	1000.0				- kW/W
	gPulsesPerKWh		- p/kW
	1000.0				- ms/s
	60.0				- s/m
	60.0				- m/h
	vTimeInMilliSeconds - ms
	*/
	float vValue = (1000.0 / (float)gPulsesPerKWh) * ((1000.0 * 60.0 * 60.0) / (float)vTimeInMilliSeconds);
	return vValue;
}


void PulsePort::SaveTotalValue()
{
	SaveTotalValue(gEepromAddress, TotalTicks);
}
void PulsePort::ReadTotalValue()
{
	unsigned long vValue = 0;
	if (ReadTotalValue(gEepromAddress, vValue))
		TotalTicks = vValue;
}

void PulsePort::SaveTotalValue(int pAddress, unsigned long pValue)
{
	int vAddress = EEPROM_TOTAL_PULSES_BASE_ADDRESS + pAddress;
	EEPROM.begin(512);
	EEPROM.put(vAddress, EEPROM_CHECK_SUM);
	vAddress++;

	EEPROM.put(vAddress, pValue);

	EEPROM.commit();
	EEPROM.end();
}
bool PulsePort::ReadTotalValue(int pAddress, unsigned long& pValue)
{
	bool vResult = false;
	int vAddress = EEPROM_TOTAL_PULSES_BASE_ADDRESS + pAddress;
	EEPROM.begin(512);
	byte vCheckSum = 0;
	EEPROM.get(vAddress, vCheckSum);

	if (vCheckSum == EEPROM_CHECK_SUM)
	{
		vAddress++;
		EEPROM.get(vAddress, pValue);

		printf("Read initial value from EEPROM: %ld", pValue);
		vResult = true;
	}
	else
	{
		printf("Checksum didn't match. No values in EEPROM");
	}

	EEPROM.end();
	return vResult;
}



ISR_Function PulsePort::GetISR(int pPin)
{
	switch (pPin)
	{
		case 0: return ISR0;
		case 1: return ISR1;
		case 2: return ISR2;
		case 3: return ISR3;
		case 4: return ISR4;
		case 5: return ISR5;
		case 6: return ISR6;
		case 7: return ISR7;
		case 8: return ISR8;
		case 9: return ISR9;
		case 10: return ISR10;
		case 11: return ISR11;
		case 12: return ISR12;
		case 13: return ISR13;
		case 14: return ISR14;
		case 15: return ISR15;
		case 16: return ISR16;

	}
}

void PulsePort::ISR(int pPin)
{
	for (int i = 0; i < PortCount; i++)
	{
		if (Ports[i]->gPin == pPin)
			Ports[i]->Pulse_ISR();
	}
}

void PulsePort::ISR0() { ISR(0); }
void PulsePort::ISR1() { ISR(1); }
void PulsePort::ISR2() { ISR(2); }
void PulsePort::ISR3() { ISR(3); }
void PulsePort::ISR4() { ISR(4); }
void PulsePort::ISR5() { ISR(5); }
void PulsePort::ISR6() { ISR(6); }
void PulsePort::ISR7() { ISR(7); }
void PulsePort::ISR8() { ISR(8); }
void PulsePort::ISR9() { ISR(9); }
void PulsePort::ISR10() { ISR(10); }
void PulsePort::ISR11() { ISR(11); }
void PulsePort::ISR12() { ISR(12); }
void PulsePort::ISR13() { ISR(13); }
void PulsePort::ISR14() { ISR(14); }
void PulsePort::ISR15() { ISR(15); }
void PulsePort::ISR16() { ISR(16); }

int PulsePort::PortCount = 0;
PulsePort* PulsePort::Ports[100]; // = new PulsePort*[100];