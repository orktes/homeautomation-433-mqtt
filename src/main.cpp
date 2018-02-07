#include "Arduino.h"
#include <RCSwitch.h> // library for controling Radio frequency switch
#include <WiFi.h>
#include <PubSubClient.h>

#define RF_RECEIVER_PIN 13 // D13 on DOIT ESP32
#define RF_EMITTER_PIN 12 // D12 on DOIT ESP32
#define RF_EMITTER_REPEAT 20
#define SERIAL_BAUD 115200

#define MQTT_SERVER "10.0.1.22"
#define MQTT_PORT 1883
#define MQTT_TOPIC "haaga/status/433toMQTT"
#define MQTT_CLIENT_ID "homeautomation-433"

#define WIFI_SSID "YOUR_WIFI_HERE"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD_HERE"

WiFiClient wifiClient;
RCSwitch mySwitch = RCSwitch();
PubSubClient client(wifiClient);

void print(String str)
{
  Serial.println(str);
}

void setupWifi()
{
  delay(10);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    print(F("."));
  }

  print(F("Connected to wifi"));
}

void setupMQTT()
{
  client.setServer(MQTT_SERVER, MQTT_PORT);
}

void setupRF()
{
  // RF init parameters
  mySwitch.enableTransmit(RF_EMITTER_PIN);
  mySwitch.setRepeatTransmit(RF_EMITTER_REPEAT);
  mySwitch.enableReceive(RF_RECEIVER_PIN);
}


void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected()) {
    print("Attempting MQTT connection...");

    if (client.connect(MQTT_CLIENT_ID)) {
      print("connected");
    } else {
      print("connect failed");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

boolean loopRF()
{
  if (mySwitch.available())
  {

    unsigned long MQTTvalue = 0;
    int MQTTprotocol = 0;
    int MQTTbits = 0;
    int MQTTlength = 0;

    MQTTvalue = mySwitch.getReceivedValue();
    MQTTprotocol = mySwitch.getReceivedProtocol();
    MQTTbits = mySwitch.getReceivedBitlength();
    MQTTlength = mySwitch.getReceivedDelay();
    mySwitch.resetAvailable();

    if (MQTTvalue !=0 ) {
        char jsonData[80];
        sprintf(
          jsonData,
          "{\"bits\":%d,\"length\":%d,\"protocol\":%d,\"value\":%lu}",
          MQTTbits,
          MQTTlength,
          MQTTprotocol,
          MQTTvalue
        );

        print(String(jsonData));

        return client.publish(MQTT_TOPIC, jsonData);
    }
  }

  return false;
}

void setup()
{
  Serial.begin(SERIAL_BAUD);

  setupWifi();
  setupMQTT();
  setupRF();
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }

  client.loop();
  loopRF();
}
