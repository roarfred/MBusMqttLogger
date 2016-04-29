#include <Arduino.h>
#include "Telegram.h"
#include "MBus.h"

byte sample_response[] = {
	/* TELEGRAM HEADER */
	0x68, 0x9A, 0x9A, 0x68,
	0x08, // C-Field
	0x00, // A-field
	0x72, // CI-Field

		  /* FIXED DATA HEADER */
	0x56, 0x01, 0x00, 0x36, // Ident
	0x49, 0x6A,             // Manufacturer
	0x88,                   // Version
	0x04,                   // Medium
	0x1E,                   // Access No
	0x00,                   // Status
	0x00, 0x00,             // Signature

	0x0C, 0x78, 0x56, 0x01, 0x00, 0x36,
	0x82, 0x80, 0x01, 0x6C, 0x01, 0x24,
	0x84, 0x80, 0x01, 0x06, 0x47, 0x00, 0x00, 0x00,
	0x84, 0x80, 0x01, 0x13, 0x88, 0xE5, 0x00, 0x00,
	0xC2, 0x80, 0x01, 0x6C, 0x01, 0x23,
	0xC4, 0x80, 0x01, 0x06, 0x00, 0x00, 0x00, 0x00,
	0xC4, 0x80, 0x01, 0x13, 0x00, 0x00, 0x00, 0x00,
	0x82, 0x81, 0x01, 0x6C, 0xFF, 0xFF,
	0x84, 0x81, 0x01, 0x06, 0x00, 0x00, 0x00, 0x80,
	0x84, 0x81, 0x01, 0x13, 0x00, 0x00, 0x00, 0x80,
	0xC2, 0x81, 0x01, 0x6C, 0xFF, 0xFF,
	0xC4, 0x81, 0x01, 0x06, 0x00, 0x00, 0x00, 0x80,
	0xC4, 0x81, 0x01, 0x13, 0x00, 0x00, 0x00, 0x80,
	0x82, 0x82, 0x01, 0x6C, 0xFF, 0xFF,
	0x84, 0x82, 0x01, 0x06, 0x00, 0x00, 0x00, 0x80,
	0x84, 0x82, 0x01, 0x13, 0x00, 0x00, 0x00, 0x80,
	0xC2, 0x82, 0x01, 0x6C, 0xFF, 0xFF,
	0xC4, 0x82, 0x01, 0x06, 0x00, 0x00, 0x00, 0x80,
	0xC4, 0x82, 0x01, 0x13, 0x00, 0x00, 0x00, 0x80,
	0x1F,
	0x47,
	0x16
};

void debug(const char* pMessage, byte vRequest[], int pLength)
{
	Serial1.print(pMessage);
	for (int i = 0; i < pLength; i++)
	{
		Serial1.print(vRequest[i], HEX);
		Serial1.print(" ");
	}
	Serial1.println();
}


MBus::MBus(int pAddress, TelegramCallback pTelegramCallback, ErrorCallback pErrorCallback)
{
    this->address = pAddress;
    this->telegramCallback = pTelegramCallback;
    this->errorCallback = pErrorCallback;
}

MBus::~MBus()
{
    //dtor
}

void MBus::ReadDevice(int pMaxTelegrams)
{
    Serial.begin(2400, SERIAL_8E1);
    while (!Serial) {}

    if (SendNKE())
    {
        errorCallback("NKE OK!!");
		delay(1000);

        int vRemaining = pMaxTelegrams;
        bool vMoreTelegrams = true;
        this->FCB = 0x20;
        while (vRemaining-- != 0 && RequestTelegram())
        {
            errorCallback("Telegram OK!!");

            Telegram *t = new Telegram();
            if (t->parse(buffer))
            {
                errorCallback("Telegram parsed OK!!");
                telegramCallback(*t);
                vMoreTelegrams = t->MoreTelegrams;
            }
            else
            {
                vMoreTelegrams = false;
            }
            delete(t);
        }
    }
    else
    {
        errorCallback("Error sending NKE");
    }
}

bool MBus::SendNKE()
{
	// Empty the buffer 
	while (Serial.available())
		Serial.read();

    byte vRequest[5] = { 0x10, 0x40, this->address, 0x00, 0x16 };
    vRequest[3] = Telegram::checkSum(vRequest, 1, 3);
    Serial.write(vRequest, 5);
	debug("Wrote to serial: ", vRequest, 5);

	unsigned long time = millis();
    while (!Serial.available() && (millis() - time) < 2000);
    if (Serial.read() == 0xe5 || this->Debug)
    {
        return true;
    }
    else
    {
        if ((millis() - time) >= 2000)
            errorCallback("NKE timeout");
        else
            errorCallback("NKE response error");
        return false;
    }
}

bool MBus::RequestTelegram()
{
    this->bytesRead = 0;

    byte vRequest[5] = {
        0x10,
        (0x5b | this->FCB),
        this->address,
        0x00,
        0x16
    };
    vRequest[3] = Telegram::checkSum(vRequest, 1, 3);

    Serial.write(vRequest, 5);
	debug("Wrote to serial: ", vRequest, 5);

    bool vCompleted = false;
    unsigned long time = millis();

    while (!vCompleted && (millis() - time) < 5000)
    {
        if (Serial.available())
        {
            buffer[this->bytesRead] = Serial.read();
            vCompleted = (buffer[this->bytesRead++] == 0x16);
			if (vCompleted)
			{
				this->FCB ^= 0x20;
				debug("Received: ", buffer, this->bytesRead);
			}
        }
    }

	if (this->Debug)
	{
		vCompleted = true;
//		buffer = sample_response;
	}

	if (!vCompleted)
	{
		debug("Received: ", buffer, this->bytesRead);
	}

    return vCompleted;
}


