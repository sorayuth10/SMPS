#ifndef SMPSNode_h
#define SMPSNode_h

#include <LoRa.h>
#include <Arduino.h>

/**
 * SMPS - SMART PARKING SYSTEM PROJECT 
 * This class is for LoRa Node 
 * 
 *
 **/


class SMPSNode{

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
        void sendPacket();


    private:
        LoRaClass* _LoRa;
        uint8_t genCRC8();
        void updateCRC8();
};


#endif