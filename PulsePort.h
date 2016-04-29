// PulsePort.h

#ifndef _PULSEPORT_h
#define _PULSEPORT_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <EEPROM.h>

typedef void(*ISR_Function)();

class PulsePort
{
public:
	PulsePort(const char* pName, int pPin, int pPulsesPerKWh, unsigned long pMinPulseLength, int pAverageOverMinutes, int pEepromAddress);

	void Begin();
	void End();
	void CommitTicksToTotal();
	void UpdateAverage();				// Function to update the average buffer
	float GetAverage();					// Function to retrieve the average
	float Current();					// Function to retrieve the current value, calculated from the last two pulses
	float TicksInWattHours();			// Converts Ticks to Watt Hours
	float TotalTicksInWattHours();		// Converts Ticks to Watt Hours

	void SaveTotalValue();												// Save the total to EEPROM
	void ReadTotalValue();												// Read the total from EEPROM
	static void SaveTotalValue(int pAddress, unsigned long pValue);		// Save the total to EEPROM (static, so it can be used from outside)
	static bool ReadTotalValue(int pAddress, unsigned long& pValue);	// Read the total from EEPROM (static, so it can be used from outside)
	static const int EEPROM_TOTAL_PULSES_BASE_ADDRESS = 250;			// EEPROM address where config class is stored

	~PulsePort();

	volatile unsigned long Ticks;		// The number of pulses received since last report
	unsigned long TotalTicks;			// The total number of pulses received since we started counting
	const char* Name;					// The name of the meter

private:
	int gPin;							// The pin number on the arduino board
	int gPulsesPerKWh;					// Number of pulses per kWh
	unsigned long gLastPulse;			// Time of last accepted pulse. Used to filter any ripple
	unsigned long gMinPulseLength;		// The configured lenght of a pulse. Used to filter any ripple

	unsigned long gLastPulseMillis;		// Time of last accepted pulse. Used to calculate current
	unsigned long gPreviousPulseMillis;	// Time of the second last accepted pulse. Used to calculate current
	int gAverageOverMinutes;			// The number of minutes the average is calculated over
	unsigned long gStartTime;			// Time when the reading started, used for calculating average
	unsigned long gAverageData[60];		// Buffer for hourly average
	byte gLastAverageDataIndex = 0;		// Pointer to the buffer
	bool gAverageDataFilled = false;	// Flag to tell if we've filled the buffer yet

	int gEepromAddress;							// Address where to store the item in EEPROM
	static const byte EEPROM_CHECK_SUM = 37;	// Just a random number to verify if numbers are in EEPROM

	void Pulse_ISR();					// The routine called when we get a pulse

	/*
		Static setup to re-route the ISR routine to a specific instance
	*/
	static PulsePort* Ports[];
	static int PortCount;
	static ISR_Function GetISR(int pPin);
	static void ISR(int pPin);
	static void ISR0();
	static void ISR1();
	static void ISR2();
	static void ISR3();
	static void ISR4();
	static void ISR5();
	static void ISR6();
	static void ISR7();
	static void ISR8();
	static void ISR9();
	static void ISR10();
	static void ISR11();
	static void ISR12();
	static void ISR13();
	static void ISR14();
	static void ISR15();
	static void ISR16();
};

#endif