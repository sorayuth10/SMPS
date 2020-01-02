#include <Arduino.h>
#include "SMPSGateway.h"
#include "LoRa.h"
#include "SSD1306.h"
#include <WiFi.h>
#include <FirebaseESP32.h>
#include "esp_wpa2.h"
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
#define HR_TO_RESET 12
#define FIREBASE_HOST "smartparking-eee98.firebaseio.com" //Do not include "https://" in FIREBASE_HOST
#define FIREBASE_AUTH "ZKYd8Ifebx1VSXZTMMuASkkcaW7zhfCNZihkworC"
#define EAP_ANONYMOUS_IDENTITY ""
#define EAP_IDENTITY "59011361"
#define EAP_PASSWORD "4f73616c7974753535" //HEX
//Define FirebaseESP32 data object

FirebaseData firebaseData;
String path = "/ESP32";
const char* ssid = "@KMITL";
int counter = 0;

SMPSGateway gateway; //init Lapss core library
SSD1306 display (0x3c, 4, 15);

String rssi = "RSSI -";
String packSize = "-";
String packet = " ";
uint16_t SENSOR = 0;
uint8_t node_id;

unsigned long lastUpdate = millis();

void showDebugMessage(String text){
  display.clear ();
  display.setTextAlignment (TEXT_ALIGN_LEFT);
  display.setFont (ArialMT_Plain_10);
  display.drawString (0, 0, text);
  display.display();
  Serial.println(text);
}

void sendData(){
  const int JSONSIZE = JSON_OBJECT_SIZE(7);
  StaticJsonDocument<JSONSIZE> jsonDoc;
  jsonDoc["name"] =  "Node "+ String(node_id,DEC);
  jsonDoc["Sensor"] = SENSOR;
  serializeJsonPretty(jsonDoc, Serial);

  HTTPClient http; //Declare object of class HTTPClient
  String postData;
  http.begin(lapssEndpoint); //Specify request destination
  // http.setTimeout(5);
  serializeJson(jsonDoc,postData);
  http.addHeader("Content-Type", "application/json");
  Serial.println(postData);

  int httpCode =   http.POST(postData);


  if (httpCode == 200)
  {
    showDebugMessage("Send OK");
    delay(500);
  }
  else
  {
    showDebugMessage("Send Failed Error code :"+ String(httpCode,DEC));
    delay(500);
    ESP.restart();    // Reset the MCU if the connection failed.
  }

  http.end(); //Close connection


  //EMON CMS


  postData = "node=LAPSSNode&json={temperature:" + String(temperature, 1) + ",humidity:" + String(humidity, 1) + ",PM25:" + String(PM25,DEC) + "}&apikey=" + emonAPIKey;
  http.begin("http://emoncms.org/input/post?" + postData); //Specify request destination

  httpCode = http.GET();         //Send the request
  String payload = http.getString(); //Get the response payload

  // Serial.println(httpCode); //Print HTTP return code
  // Serial.println(postData);  //Print request response payload

  if (httpCode == 200)
  {
    showDebugMessage("EMON Send OK");
    delay(500);
  }
  else
  {
    showDebugMessage("EMON Send Failed Error code :"+ String(httpCode,DEC));
    delay(500);
  }

  http.end(); //Close connection

}

//When WiFi Manager enters config mode (Failed to connect to wifi)
void setConfigModeCallback(WiFiManager *myWiFiManager){
  showDebugMessage("Enter Wifi setup mode : " + myWiFiManager->getConfigPortalSSID());
}

void displayData () {
  display.clear ();
  display.setTextAlignment (TEXT_ALIGN_LEFT);
  display.setFont (ArialMT_Plain_10);
  display.drawString (0, 0, rssi);

  display.setFont(ArialMT_Plain_16);
  display.drawString (0, 25, String(temperature,1)+"°c  " + String(humidity,1) + "%\n" + "PM25: "+ PM25 +" ug/cm3");

  int lastUpdateSec = (millis() - lastUpdate) /1000;

  display.setTextAlignment (TEXT_ALIGN_RIGHT);
  display.setFont (ArialMT_Plain_10);
  
  display.drawString (127, 0, "By node " + String( node_id));
  display.drawString (127, 12, String(lastUpdateSec) +"s ago");


  display.display ();
}


void onReceive(int packetSize){
   // received a packet
  Serial.print("Received packet \n");

  if (packetSize != sizeof(gateway.data)){       //Wrong size of data
    Serial.println("Size mismatch");
    return;
  }

  uint8_t data[packetSize];
  
  // read packet
  for (int i = 0; i < packetSize; i++) {
    data[i] = (uint8_t)LoRa.read();
  }
  if(!gateway.processPacket(data)){   //invalid CRC
      showDebugMessage("Data corrupted");
    return;
  }
  
  rssi = "RSSI "+ String(LoRa.packetRssi(),DEC);

  Serial.printf("Received from node ID : %d with RSSI : %d\n",gateway.data.ID, LoRa.packetRssi());
  Serial.printf("\tSensor: %d\n",gateway.data.SENSOR);
  
  node_id = gateway.data.ID;
  SENSOR  = gateway.data.SENSOR;
  lastUpdate = millis();
  displayData();
  sendData();
}

void setup()
{
  Serial.begin(9600);   // Serial to PC
//  Serial2.begin(9600);  // Serial for PMS Dust sensor

  //Init oled
  pinMode (16, OUTPUT);
  digitalWrite (16, LOW); // set GPIO16 low to reset OLED
  delay (50);
  digitalWrite (16, HIGH); // while OLED is running, GPIO16 must go high,
  display.init ();
  display.flipScreenVertically ();
  display.setFont (ArialMT_Plain_10);
  
  //Init LoRa Chip
  showDebugMessage("Init LoRa");
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(BAND))
  {
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }
  Serial.println("init lora Ok");

  //Init Wifi
  Serial.print("Connecting to Wi-Fi");
  Serial.println(ssid);
  WiFi.disconnect(true);  //disconnect form wifi to set new wifi connection
  WiFi.mode(WIFI_STA); //init wifi mode
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_ANONYMOUS_IDENTITY, strlen(EAP_ANONYMOUS_IDENTITY)); 
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));
  esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT(); //set config settings to default
  esp_wifi_sta_wpa2_ent_enable(&config); //set config settings to enable function
  WiFi.begin(ssid); //connect to wifi
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  //Set database read timeout to 1 minute (max 15 minutes)
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  //tiny, small, medium, large and unlimited.
  //Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
  Firebase.setwriteSizeLimit(firebaseData, "tiny");
  delay (1500);
}

void loop(){
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
   onReceive(packetSize);
   Firebase.setInt(firebaseData, path + "/Val", analogRead(SENSOR_PIN)); //send to firebase
  }
  delay(100);
  displayData();
  if (millis() > 1000*60*60*HR_TO_RESET){
    ESP.restart();
  }
}
