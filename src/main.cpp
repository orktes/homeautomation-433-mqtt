#include "Arduino.h"
#include <RCSwitch.h> // library for controling Radio frequency switch
#include <WiFi.h>
#include <PubSubClient.h>

#include "config.h"

#define RF_RECEIVER_PIN 13 // D13 on DOIT ESP32
#define RF_EMITTER_PIN 12 // D12 on DOIT ESP32
#define RF_EMITTER_REPEAT 20
#define SERIAL_BAUD 115200



WiFiClient wifiClient;
RCSwitch mySwitch = RCSwitch();
PubSubClient client(wifiClient);
QueueHandle_t queue;

struct Message {
  unsigned long value = 0;
  int protocol = 0;
  int bits = 0;
  int delay = 0;
};

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

    String taskMessage = "RF Task running on core ";
    taskMessage = taskMessage + xPortGetCoreID();
    print(taskMessage);

    Message msg;

    msg.value = mySwitch.getReceivedValue();
    msg.protocol = mySwitch.getReceivedProtocol();
    msg.bits = mySwitch.getReceivedBitlength();
    msg.delay = mySwitch.getReceivedDelay();
    mySwitch.resetAvailable();

    if (msg.value != 0) {
      xQueueSend(queue, &msg, portMAX_DELAY);
      return true;
    }
  }

  return false;
}

void queueReadLoop(void *pvParameters)
{
  Message msg;
  while (true)
  {
    String taskMessage = "MQTT worker running on core ";
    taskMessage = taskMessage + xPortGetCoreID();
    print(taskMessage);

    xQueueReceive(queue, &msg, portMAX_DELAY);
    char jsonData[80];
    sprintf(
      jsonData,
      "{\"bits\":%d,\"delay\":%d,\"protocol\":%d,\"value\":%lu}",
      msg.bits,
      msg.delay,
      msg.protocol,
      msg.value
    );
    print(String(jsonData));
    client.publish(MQTT_TOPIC, jsonData);
  }
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }

  client.loop();

  while (loopRF()) {}
}

void setup()
{
  Serial.begin(SERIAL_BAUD);

  queue = xQueueCreate( 10, sizeof( Message ) );

  setupWifi();
  setupMQTT();
  setupRF();

  xTaskCreatePinnedToCore(queueReadLoop, "queueReadLoop", 4096, NULL, 1, NULL, 1);
}
