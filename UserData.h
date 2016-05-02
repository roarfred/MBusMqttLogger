#include <Arduino.h>
#include <time.h>

#ifndef USERDATA_H
#define USERDATA_H


const static char* UserDataTypeNames[] =
{
    "Unknown",
    "Energy1",
    "Energy2",
    "Volume",
    "Mass",
    "OnTime",
    "OperatingTime",
    "Power1",
    "Power2",
    "VolumeFlow",
    "VolumeFlowExt1",
    "VolumeFlowExt2",
    "MassFlow",
    "FlowTemperature",
    "ReturnTemperature",
    "TemperatureDifference",
    "ExternalTemperature"
    "Pressure",
    "Date",
    "DateAndTime",
    "ExtendedTimePoint",
    "ExtendedDateAndTime",
    "UnitsForHCA",
    "Reserved",
    "AveragingDuration",
    "ActualityDuration",
    "FabricationNo",
    "EnhancedIdentification",
    "Address"
};

enum UserDataType
{
    UserDataType_Unknown,
    UserDataType_Energy1,
    UserDataType_Energy2,
    UserDataType_Volume,
    UserDataType_Mass,
    UserDataType_OnTime,
    UserDataType_OperatingTime,
    UserDataType_Power1,
    UserDataType_Power2,
    UserDataType_VolumeFlow,
    UserDataType_VolumeFlowExt1,
    UserDataType_VolumeFlowExt2,
    UserDataType_MassFlow,
    UserDataType_FlowTemperature,
    UserDataType_ReturnTemperature,
    UserDataType_TemperatureDifference,
    UserDataType_ExternalTemperature,
    UserDataType_Pressure,
    UserDataType_Date,
    UserDataType_DateAndTime,
    UserDataType_ExtendedTimePoint,
    UserDataType_ExtendedDateAndTime,
    UserDataType_UnitsForHCA,
    UserDataType_Reserved,
    UserDataType_AveragingDuration,
    UserDataType_ActualityDuration,
    UserDataType_FabricationNo,
    UserDataType_EnhancedIdentification,
    UserDataType_Address
};


const static char* UserDataUnitNames[] =
{
    "unknown",
    "W",
    "Wh",
    "J",
    "J/h",
    "m3", //"m³",
    "m3/h", //"m³/h",
    "m3/m", //"m³/m",
    "m3/s", //"m³/s",
    "kg",
    "kg/h",
    "s",
    "m",
    "h",
    "days",
    "C", //"°C",
    "K", //"°K",
    "Bar"
    "",
    ""
};
enum UserDataUnit
{
    UserDataUnit_Unknown,
    UserDataUnit_Watts,
    UserDataUnit_WattHours,
    UserDataUnit_Joules,
    UserDataUnit_JoulesPerHour,
    UserDataUnit_CubicMeters,
    UserDataUnit_CubicMetersPerHour,
    UserDataUnit_CubicMetersPerMinute,
    UserDataUnit_CubicMetersPerSecond,
    UserDataUnit_KiloGrams,
    UserDataUnit_KiloGramsPerHour,
    UserDataUnit_Seconds,
    UserDataUnit_Minutes,
    UserDataUnit_Hours,
    UserDataUnit_Days,
    UserDataUnit_Celcius,
    UserDataUnit_Kelvin,
    UserDataUnit_Bar,
    UserDataUnit_Date,
    UserDataUnit_BCD
};

class UserData
{
    private:
		time_t toUnixTime(int pYear, int pMonth, int pDay, int pHour, int pMinute, int pSeconds);
    public:
        byte DIF[11];
        byte VIF[11];
        int Storage;
        int DataLength;
        byte* Data;

        UserDataType Type;
        float Factor;
        UserDataUnit Unit;
        float Value;

        UserData* Next;

        UserData();
        virtual ~UserData();
        float DataAsFloat(float pFactor);
        unsigned long DataAsInteger();
        unsigned long DataAsBCDInteger();
        time_t DataAsDate_F();
		time_t DataAsDate_G();
		time_t DataAsDate_I();
		time_t DataAsDate_J();

        void print(tm tmp);
        void debug();
        void parse();
};
#endif // USERDATA_H
