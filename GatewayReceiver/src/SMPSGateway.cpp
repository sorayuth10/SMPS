#include <SMPSGateway.h>

void SMPSGateway::setup(LoRaClass &lora, uint8_t id)
{
    this->_LoRa = &lora;
    data.ID = id;
}

void SMPSGateway::setSENSOR(uint16_t SENSOR)
{
    data.SENSOR = SENSOR;
    updateCRC8();
}

uint8_t SMPSGateway::getDataCRC8() { return genCRC8(&data.ID); }

uint8_t SMPSGateway::genCRC8(uint8_t *addr)
{

    uint8_t len = sizeof(data) - sizeof(data.CRC8); //Exclude CRC8 at last byte from data
    uint8_t crc = 0;
    while (len--)
    {
        uint8_t inbyte = *addr++;
        for (uint8_t i = 8; i; i--)
        {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix)
                crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    data.CRC8 = crc;

    return crc;
}

void SMPSGateway::updateCRC8()
{
    data.CRC8 = genCRC8(&data.ID);
}

int SMPSGateway::processPacket(uint8_t incomingData[])
{
    // if (sizeof(incomingData) != sizeof(data))
    //     return false;
    // Data size checking above is not matched for some reason

    DATA temp;
    uint8_t *addr = &temp.ID;
    uint8_t len = sizeof(temp);
    for (int i = 0; i<len; i++)
    {
        *addr++ = incomingData[i];
    }

    if (genCRC8(&temp.ID) != temp.CRC8)
        return false;
    
    data = temp;
    return true;

}