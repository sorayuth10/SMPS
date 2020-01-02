#include <SMPSNode.h>

void SMPSNode::setup(LoRaClass &lora, uint8_t id)
{
    this->_LoRa = &lora;
    data.ID = id;
}

void SMPSNode::setSENSOR(uint16_t SENSOR)
{
    data.SENSOR = SENSOR;
    updateCRC8();
}

uint8_t SMPSNode::getDataCRC8() { return genCRC8(); }

uint8_t SMPSNode::genCRC8()
{
    uint8_t *addr = &data.ID;
    uint8_t len = sizeof(data) - sizeof(data.CRC8); //Exclude CRC8 from data
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

void SMPSNode::updateCRC8()
{
    data.CRC8 = genCRC8();
}

void SMPSNode::sendPacket()
{
    LoRa.beginPacket(); // start packet

    uint8_t *addr = &data.ID;
    uint8_t len = sizeof(data);
    while (len--)
    {
        uint8_t dataByte = *addr++;
        LoRa.write(dataByte); // Send each byte from data
    }
    LoRa.endPacket(); // finish packet and send it
}
