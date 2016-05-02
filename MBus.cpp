#include <Arduino.h>
#include "Telegram.h"
#include "MBus.h"

byte sample_response[] = {
	/* TELEGRAM HEADER */
	0x68, // Start of long telegram
	0x71, // Length
	0x71, // Length repeated
	0x68, // Confirm start of long telegram
	
	0x08, // C-Field
	0x00, // A-field
	0x72, // CI-Field

    /* FIXED DATA HEADER */
	0x56, 0x01, 0x00, 0x36, // Ident
	0x49, 0x6A,             // Manufacturer
	0x88,                   // Version
	0x04,                   // Medium
	0x1D,                   // Access No
	0x00,                   // Status
	0x00, 0x00,             // Signature

	
	0x0C,				0x78,	0x56, 0x01, 0x00, 0x36,		// Data: EnhancedIdentification
	0x04,				0x06,	0xE0, 0x00, 0x00, 0x00,		// Data: Energy1
	0x82, 0x04,			0x6C,	0xFF, 0xFF,					// Data: DateAndTime (Storage 8)
	0xC2, 0x84, 0x00,	0x6C,	0xFF, 0xFF,					// Data: DateAndTime (Storage 9)
	0x84, 0x04,			0x06,	0x00, 0x00, 0x00, 0x80,		// Data: Energy1 (Storage 8)
	0xC4, 0x84, 0x00,	0x06,	0x00, 0x00, 0x00, 0x80,		// Data: Energy1 (Storage 9)
	0x82, 0x0A,			0x6C,	0x01, 0x24,					// Data: DateAndTime (Storage 20)
	0x84, 0x0A,			0x06,	0x47, 0x00, 0x00, 0x00,		// Data: Energy1 (Storage 20)
	0x04,				0x13,	0x0F, 0x4C, 0x04, 0x00,		// Data: Volume
	0x02,				0x59,	0xB6, 0x0E,					// Data: FlowTemperature
	0x02,				0x5D,	0xDC, 0x0E,					// Data: ReturnTemperature
	0x02,				0x61,	0x00, 0x00, 				// Data: TemperatureDifference
	0x04,				0x2D,	0x00, 0x00, 0x00, 0x00,		// Data: Power1
	0x04,				0x3B,	0xDC, 0x02, 0x00, 0x00,		// Data: VolumeFlow 
	0x04,				0x6D,	0x07, 0x13, 0x0D, 0x24,		// Data: ExtendedTimePoint
	0x04,				0x26,	0x84, 0x01, 0x00, 0x00,		// Data: OnTime
	0x02,				0xFD,	0x17, 0x00, 0x00,			// Data: ?
	
	0x1F,	// More data?
	0x01,	// Checksum
	0x16	// End of Telegram
};

void debug(const char* pMessage, byte vRequest[], int pLength)
{
	Serial1.print(pMessage);
	for (int i = 0; i < pLength; i++)
	{
		Serial1.print(" ");
		Serial1.print(vRequest[i], HEX);
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
		for (int i = 0; i < sizeof(sample_response); i++)
			buffer[i] = sample_response[i];
	}

	if (!vCompleted)
	{
		debug("Received: ", buffer, this->bytesRead);
	}

    return vCompleted;
}


