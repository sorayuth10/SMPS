#ifndef SMPSGateway_h
#define SMPSGateway_h

#include <LoRa.h>
#include <Arduino.h>

/**
 * SMPS - SMART PARKING SYSTEM PROJECT 
 * This class is for LoRa Gateway 
 * 
 *
 **/


class SMPSGateway{

    public:

        struct DATA
        {
        uint8_t ID;
        uint16_t SENSOR;
        uint8_t CRC8;
        }__attribute__((packed));   //Disable memory 4-byte alignment

        DATA data;

        void setup(LoRaClass& lora, uint8_t id);
        void setSENSOR(uint16_t);
        uint8_t getDataCRC8();
        int processPacket(uint8_t *message);


    private:
        LoRaClass* _LoRa;
        uint8_t genCRC8(uint8_t addr[]);
        void updateCRC8();
};


#endif
