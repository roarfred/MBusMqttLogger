#include "Telegram.h"

#ifndef MBUS_H
#define MBUS_H

typedef void (*TelegramCallback)(Telegram &pTelegram);
typedef void (*ErrorCallback)(const char *pMessage);

class MBus
{
    public:
        MBus(int pAddress, TelegramCallback pTelegramCallback, ErrorCallback pErrorCallback);
        virtual ~MBus();
        void ReadDevice(int pMaxTelegrams);
		bool Debug;
protected:

    private:
		int address;
        Telegram* data;
        TelegramCallback telegramCallback;
        ErrorCallback errorCallback;
        bool SendNKE();
        bool RequestTelegram();
        byte buffer[255];
        int bytesRead;
        byte FCB; // should alter between 1 and 0 on successful transmissions of telegrams
};

#endif // MBUS_H
