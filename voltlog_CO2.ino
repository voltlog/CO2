/*
This is an experimental sketch that I wrote for my CO2 Monitoring System.
This runs on an ESp32 based development board (LOLIN32) and reads data from a CCS811/HDC1080 breakout board over I2C
As well as an MH-Z19B sensor via UART.

Copyright: Voltlog 2019
*/
#include <PubSubClient.h>
#include <WiFi.h>
#include <Arduino.h>
#include "ClosedCube_HDC1080.h"
#include "Adafruit_CCS811.h"
#include "MHZ19.h"   

//MQTT_MAX_PACKET_SIZE 256 patched in <PubSubClient.h>

// Wifi credentials below
const char* ssid = "wifi";
const char* password = "password";

// MQTT server config
#define MQTT_SERVER "192.168.0.120"
#define MQTT_PORT 1883 
#define MQTT_USER "co2" 
#define MQTT_PASSWORD "password" 
#define MQTT_PUBLISH_DELAY 60000    //60seconds
#define MQTT_CLIENT_ID "co2Node"
#define MQTT_TOPIC_STATE "co2/status"

// Initialize the client
WiFiClient co2Node;
PubSubClient mqttClient(co2Node);

#define RX_PIN 25                                         // Rx pin which the MHZ19 Tx pin is attached to
#define TX_PIN 26                                          // Tx pin which the MHZ19 Rx pin is attached to
#define BAUDRATE 9600                                      // Native to the sensor (do not change)

MHZ19 myMHZ19;                                             // Constructor for MH-Z19 class
HardwareSerial mySerial(1);                               // Init UART for MHZ19

//Init CCS811 sensor
Adafruit_CCS811 ccs;

//Init HDC1080 sensor
ClosedCube_HDC1080 hdc1080;

//Needed variables
long now = millis();
long lastMsgTime = 0;
int mhz19_temp;
int mhz19_co2;
int hdc1080_temp;
int hdc1080_humidity;
int ccs811_co2;
int ccs811_tvoc;
int ccs811_temp;
char data_string[164];
    
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  Serial.println("Voltlog CO2 & Air Quality Monitor");

  //MH-Z19 setup uart interface
  mySerial.begin(BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN); 
  myMHZ19.begin(mySerial);                                
  myMHZ19.autoCalibration();                              
 
  //Check CCS811 sensor is alive
  if(!ccs.begin()){
    Serial.println("Failed to start CCS811 sensor! Please check your wiring.");
    while(1);
  }

  // Wait for the sensor to be ready
  //while(!ccs.available());

  //calibrate temperature sensor inside CCS811
  while(!ccs.available());
  float temp = ccs.calculateTemperature();
  ccs.setTempOffset(temp - 25.0);

  //Check HDC1080 sensor is alive
  hdc1080.begin(0x40);

  //Setup wifi and mqtt client
  setup_wifi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  
  delay(5000);
}

void loop() {
if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();

  long now = millis();
  if (now - lastMsgTime > MQTT_PUBLISH_DELAY) {
    lastMsgTime = now;
    
    //Get CCS811 data
    if(ccs.available()){
      ccs811_temp = ccs.calculateTemperature();
      if(!ccs.readData()){
        ccs811_co2 = ccs.geteCO2();
        ccs811_tvoc = ccs.getTVOC();
      }
      else{
        Serial.println("ERROR, could not read CCS811!");
        while(1);
      }
    }
  
    //Get HDC1080 data
    hdc1080_temp = hdc1080.readTemperature();
    hdc1080_humidity = hdc1080.readHumidity();

    //Get MH-Z19 data
    mhz19_co2 = myMHZ19.getCO2();                             
    mhz19_temp = myMHZ19.getTemperature();                    

    // Publishes Temperature and Humidity values
    sprintf(data_string, "\n{\"mhz19_temp\":%u,\"mhz19_co2\":%u,\"hdc1080_temp\":%u,\"hdc1080_humidity\":%u,\"ccs811_co2\":%u,\"ccs811_tvoc\":%u,\"ccs811_temp\":%u\n}", mhz19_temp, mhz19_co2, hdc1080_temp, hdc1080_humidity, ccs811_co2, ccs811_tvoc, ccs811_temp);
    mqttPublish("co2", data_string);
  }
}

void mqttReconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");

    // Attempt to connect
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD, MQTT_TOPIC_STATE, 1, true, "disconnected", false)) {
      Serial.println("connected");

      // Once connected, publish an announcement...
      mqttClient.publish(MQTT_TOPIC_STATE, "connected", true);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void mqttPublish(char *topic, char *payload) {
  Serial.print("Publishing to MQTT: topic:");
  Serial.print(topic);
  Serial.print(" payload: ");
  Serial.println(payload);

  mqttClient.publish(topic, payload, true);
}
