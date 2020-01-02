#include <Arduino.h>
#include <LoRa.h>
#include "SMPSNode.h"
//#include "DHT.h"
//#include "PMS.h"
//#include "WiFiManager.h"
//#include "ArduinoJson.h"
//#include "HTTPClient.h"

//Define variable
#define SCK 5   // GPIO5 - SX1278's SCK
#define MISO 19 // GPIO19 - SX1278's MISO
#define MOSI 27 // GPIO27 - SX1278's MOSI
#define SS 18   // GPIO18 - SX1278's CS
#define RST 14  // GPIO14 - SX1278's RESET
#define DI0 26  // GPIO26 - SX1278's IRQ (interrupt request)
#define BAND 915E6
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 300      /* Time ESP32 will go to sleep (in seconds) */
#define SENSOR_UPDATE_INTERVAL 3  /* Fetch dust sensors every TIME_TO_SLEEP * PMS_UPDATE_INTERVAL seconds */
#define SENSOR_PIN 36
#define ONBOARD_LED 2
#define NODE_ID 1

//Init Object
SMPSNode node;

RTC_DATA_ATTR uint16_t SENSOR = 0;
RTC_DATA_ATTR uint32_t bootCount = 0;

void sendData()
{
  Serial.printf("Sensors Data:\n");
  Serial.printf("\tSENSOR: %d\n", node.data.SENSOR);

  //Print the data as Hex
  Serial.printf("Data bytes: ");
  uint8_t *addr = &(node.data).ID;
  uint8_t len = sizeof(node.data);
  while (len--)
  {
    uint8_t inbyte = *addr++;
    Serial.printf("%02X ", inbyte);
  }
  Serial.println("");
  node.sendPacket();
  delay(1000);
}

//Fetch data from sensors and send to gateway.
void fetchSENSOR()
{
  //Setup Sensor
  pinMode(SENSOR_PIN, OUTPUT);

//  Serial.print("Warming up sensor");
//  for (int i = 0; i < 60; i++)
//  {
//    Serial.printf(". ");
//    delay(1000);
//  }
//  Serial.println();

  node.setSENSOR(analogRead(SENSOR_PIN));
}

void setup()
{
  Serial.begin(9600);  // Serial to PC

  //Turn on onboard LED to indicate that it's working
   pinMode(ONBOARD_LED,OUTPUT);
   digitalWrite(ONBOARD_LED,HIGH);

  //Init LoRa Chip
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(BAND))
  {
    Serial.println("Starting LoRa failed!");
    while (1)
    ;
  }
    Serial.println("init lora Ok");
  
  node.setup(LoRa, NODE_ID);

  fetchSENSOR();
  if (bootCount % SENSOR_UPDATE_INTERVAL == 0){
    fetchSENSOR();
  }else{ 
    //if isn't the time for fetching new dust senser value, return last value instead.
    node.setSENSOR(SENSOR);
  }

  sendData();
  //Sleep ESP32
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Sleep the MCU");
  digitalWrite(ONBOARD_LED,LOW);

  //esp_deep_sleep_start();
}

void loop()
{
}
