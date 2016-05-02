#include <Arduino.h>
#include "Telegram.h"

Telegram::Telegram()
{
    this->Data = NULL;
    this->MoreTelegrams = false;
}

Telegram::~Telegram()
{
    delete(this->Data);
}

bool Telegram::parse(byte pData[])
{
    if (pData[0] == 0xE5)
    {
        this->TelegramFormat = Single;
        return true;
    }
    else if (pData[0] == 0x10)
    {
        this->TelegramFormat = Short;
        return this->parseShort(pData);
    }
    else if (pData[0] == 0x68 && pData[1] == 0x03)
    {
        this->TelegramFormat = Control;
        return this->parseControl(pData);
    }
    else if (pData[0] == 0x68 && pData[1] > 0x03)
    {
        this->TelegramFormat = Long;
        return this->parseLong(pData);
    }
    else
    {
		Serial1.println("Invalid start of data: 0x");
		Serial1.println(pData[0]);
        return false;
    }
}


bool Telegram::parseShort(byte pData[])
{
    if (pData[4] != 0x16)
    {
		Serial1.print("Invalid end of data: 0x");
		Serial1.println(pData[4]);
        return false;
    }
    else if (checkSum(pData, 1, 2) != pData[3])
    {
		Serial1.print("Invalid checksum. Was 0x");
		Serial1.print(pData[3], HEX);
		Serial1.print(". Should be 0x");
		Serial1.println(checkSum(pData, 1, 2), HEX);
        return false;
    }
    else
    {
        this->CField = pData[1];
        this->AField = pData[2];

        if (pData[1] == 0x40)
        {
            // Initialization of slave
            return true;
        }
        else if (pData[1] == 0x5B || pData[1] == 0x7B)
        {
            // Request class 2 data
            return true;
        }
        else if (pData[1] == 0x5A || pData[1] == 0x7A)
        {
            // Request class 1 data
            return true;
        }
        else
        {
			Serial1.print("Invalid control field. Was 0x");
			Serial1.print(pData[1], HEX);
			Serial1.println(". Valid values are 0x40 (slave init), 0x5B/0x7B (req class 2 data) or 0x5A/0x7A (req class 1 data)");
            return false;
        }
    }
}

bool Telegram::parseControl(byte pData[])
{
    return parseLong(pData);
}

bool Telegram::parseLong(byte pData[])
{
    int vLength = pData[1];

    // There should be two equal bytes to describe the length
    if (pData[2] != vLength)
    {
		Serial1.print("Invalid length field. Second byte did not match. Was 0x");
		Serial1.print(pData[2], HEX);
		Serial1.print(". Should be 0x");
		Serial1.println(pData[1], HEX);
        return false;
    }
    // The second last byte should be the checksum
    else if (checkSum(pData, 4, vLength) != pData[4 + vLength])
    {
		Serial1.print("Invalid checksum. Was 0x");
		Serial1.print(pData[4 + vLength], HEX);
		Serial1.print(". Should be 0x");
		Serial1.println(checkSum(pData, 4, vLength), HEX);
        return false;
    }
    // The very last byte should be 0x16
    else if (pData[5 + vLength] != 0x16)
    {
		Serial1.println("Expected stop 0x16. Was 0x");
		Serial1.println(pData[5 + vLength], HEX);
        return false;
    }
    else
    {
        this->CField = pData[4];
        this->AField = pData[5];
        this->CIField = pData[6];
        this->MoreTelegrams = pData[vLength - 3] == 0x1f;
        return parseUserData(pData, 7, vLength - 3);
    }
}

bool Telegram::getMode()
{
    return this->CIField & 0x4 == 0x4;
}

bool Telegram::parseUserData(byte pData[], int pStart, int pLength)
{
	Serial1.println("Mode is: ");
    if (getMode() == MSBFIRST) 
		Serial1.println("MSBFIRST");
    else 
		Serial1.println("LSBFIRST");

    if (this->CIField == 0x70)
    {
        // report of general application errors
		Serial1.println("parseUserData::CI [report of general application errors] is NOT IMPLEMENTED YET");
        return false;
    }
    else if (this->CIField == 0x71)
    {
        // report of alarm status
		Serial1.println("parseUserData::CI [report of alarm status] is NOT IMPLEMENTED YET");
        return false;
    }
    else if (this->CIField == 0x72 || this->CIField == 0x76)
    {
        // variable data respond
		Serial1.println("parseUserData::CI [variable data respond] is IMPLEMENTED");
        return parseVariableDataStructure(pData, pStart, pLength);
    }
    else if (this->CIField == 0x73 || this->CIField == 0x77)
    {
        // fixed data respond
		Serial1.println("parseUserData::CI [fixed data respond] is NOT IMPLEMENTED YET");
        return false;
    }
    else
    {
		Serial1.println("parseUserData::NOT IMPLEMENTED for CI 0x");
		Serial1.println(this->CIField, HEX);
        return false;
    }
}

bool Telegram::parseVariableDataStructure(byte pData[], int pStart, int pLength)
{
    this->IdentNumber = (this->getMode() == LSBFIRST) ? LSBFIRSTtoBCD(pData, pStart, 4) : MSBFIRSTtoBCD(pData, pStart, 4);
    parseManufacturer(pData, pStart + 4);
    this->Version = pData[pStart + 6];
    this->Medium = pData[pStart + 7];
    this->Access = pData[pStart + 8];
    this->Status = pData[pStart + 9];
    // Signature is in pData[pStart + 10] and pData[pStart + 11], only used for encryption
    this->Data = readData(pData, pStart + 12, pLength);
    return true;
}

UserData* Telegram::readData(byte pData[], int pStart, int pLength)
{
    if (pStart <= pLength)
    {
        UserData* vData = new UserData();
        vData->DIF[0] = pData[pStart];

        int i=1;
        while (pData[pStart] & 0b10000000)
        {
            vData->DIF[i++] = pData[++pStart];
        }

        vData->VIF[0] = pData[++pStart];
        i=1;
        while (pData[pStart] & 0b10000000)
        {
            vData->VIF[i++] = pData[++pStart];
        }

        if (vData->DIF[0] & 0xF == 0xF)
        {
            // handle special cases here (p. 16, Table 7)
			Serial1.println("SPECIAL CASE NOT SUPPORTED: DIF is 0x");
			Serial1.println(vData->DIF[0], HEX);
        }
        else
        {
            int vBytesOfData = -1;

            switch(vData->DIF[0] & 0xF)
            {
                case 0b0000:    // No data
                    vBytesOfData = 0;
                    break;
                case 0b0001:    // 8 bit Integer/Binary
                    vBytesOfData = 1;
                    break;
                case 0b0010:    // 16 bit Integer/Binary
                    vBytesOfData = 2;
                    break;
                case 0b0011:    // 24 bit Integer/Binary
                    vBytesOfData = 3;
                    break;
                case 0b0100:    // 32 bit Integer/Binary
                    vBytesOfData = 4;
                    break;
                case 0b0101:    // 32 bit Real
                    vBytesOfData = 4;
                    break;
                case 0b0110:    // 48 bit Integer/Binary
                    vBytesOfData = 6;
                    break;
                case 0b0111:    // 64 bit Integer/Binary
                    vBytesOfData = 8;
                    break;

                case 0b1000:    // Selection for Readout (???)
                    break;
                case 0b1001:    // 2 digit BCD
                    vBytesOfData = 1;
                    break;
                case 0b1010:    // 4 digit BCD
                    vBytesOfData = 2;
                    break;
                case 0b1011:    // 6 digit BCD
                    vBytesOfData = 3;
                    break;
                case 0b1100:    // 8 digit BCD
                    vBytesOfData = 4;
                    break;
                case 0b1101:    // Variable length
                    break;
                case 0b1110:    // 12 digit BCD
                    vBytesOfData = 6;
                    break;
                case 0b1111:    // Special functions, already handled above
                    break;
            }

            if (vBytesOfData >= 0)
            {
                vData->DataLength = vBytesOfData;
                vData->Data = &pData[++pStart];

                pStart += vBytesOfData;
                vData->Next = readData(pData, pStart, pLength);
            }
            else
                vData->DataLength = 0;
        }

        vData->parse();
        return vData;
    }
    else
    {
        return NULL;
    }
}

int Telegram::LSBFIRSTtoInt16(byte pData[], int pStart)
{
    return ((int)pData[pStart+1] << 8) + (int)pData[pStart];
}

bool Telegram::parseManufacturer(byte pData[], int pStart)
{
    int vValue = LSBFIRSTtoInt16(pData, pStart);

    int vValues[3];
    vValues[0] = vValue % 32;
    vValue -= vValues[0];
    vValue /= 32;
    vValues[1] = vValue % 32;
    vValue -= vValues[1];
    vValue /= 32;
    vValues[2] = vValue % 32;

    this->Manufacturer[0] = 64 + vValues[2];
    this->Manufacturer[1] = 64 + vValues[1];
    this->Manufacturer[2] = 64 + vValues[0];
    this->Manufacturer[3] = 0;

	Serial1.println("Man: ");
	Serial1.println(this->Manufacturer);

    return true;
}

long Telegram::LSBFIRSTtoBCD(byte pData[], int pStart, int pLength)
{
    long vFactor = 1;
    long vNumber = 0;
    for (int i=pStart; i<(pStart + pLength); i++)
    {
        byte vMSB = pData[i] >> 4;
        byte vLSB = (pData[i] & 0x0F);

        vNumber += (vMSB * 10 + vLSB) * vFactor;
        vFactor *= 100;
    }
    return vNumber;
}

long Telegram::MSBFIRSTtoBCD(byte pData[], int pStart, int pLength)
{
    long vFactor = 1;
    long vNumber = 0;
    for (int i=pLength-1; i>=pStart; i++)
    {
        byte vMSB = pData[i] >> 4;
        byte vLSB = (pData[i] & 0x0F);

        vNumber += (vMSB * 10 + vLSB) * vFactor;
        vFactor *= 100;
    }
    return vNumber;
}

byte Telegram::checkSum(byte pData[], int pStart, int pLength)
{
    byte vSum = 0;
    for (int i = pStart; i < (pStart + pLength); i++)
    {
        vSum += pData[i];
    }
    return vSum;
}
