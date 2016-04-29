#include "UserData.h"

#ifndef TELEGRAM_H
#define TELEGRAM_H


enum TelegramTypeEnum
{
    None,
    Single,
    Short,
    Control,
    Long
};

class Telegram
{
    public:
        Telegram();
        virtual ~Telegram();
        bool parse(byte pData[]);
        static unsigned char checkSum(byte pData[], int pStart, int pLength);

        TelegramTypeEnum TelegramFormat;
        byte CField;
        byte AField;
        byte CIField;
        long IdentNumber;
        char Manufacturer[4];
        byte Version;
        byte Medium;
        byte Access;
        byte Status;
        bool MoreTelegrams;
        UserData* Data;

    protected:
    private:
        bool parseShort(byte pData[]);
        bool parseControl(byte pData[]);
        bool parseLong(byte pData[]);
        bool parseUserData(byte pData[], int pStart, int pLength);
        bool parseVariableDataStructure(byte pData[], int pStart, int pLength);

        bool parseManufacturer(byte pData[], int pStart);

        int LSBFIRSTtoInt16(byte pData[], int pStart);
        long LSBFIRSTtoBCD(byte pData[], int pStart, int pLength);
        long MSBFIRSTtoBCD(byte pData[], int pStart, int pLength);

        bool getMode();

        UserData* readData(byte pData[], int pStart, int pLength);
};

#endif // TELEGRAM_H
