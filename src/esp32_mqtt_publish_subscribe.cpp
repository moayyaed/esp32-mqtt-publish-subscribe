/**
 * This esp32_mqtt_publish_subscribe.cpp implements a MQTT Publish/Subscribe
 * mechanism.
 *  
 * MIT License
 * 
 * T-Call_ESP32_SIM800L - Samples code
 * Copyright (c) 2021 Antonio Musarra's Blog - https://www.dontesta.it
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in the
 * Software without restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
 * Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <ESP32Ping.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <PubSubClient.h>
#include "time.h"

// Macro to read build flags
#define ST(A) #A
#define STR(A) ST(A)

#ifdef WIFI_SSID
const char *ssid = STR(WIFI_SSID);
#endif

#ifdef WIFI_PASSWORD
const char *password = STR(WIFI_PASSWORD);
#endif

#ifdef MQTT_SERVER
const char *mqtt_server = STR(MQTT_SERVER);
#endif

#ifdef MQTT_PORT
const int mqtt_port = atoi(STR(MQTT_PORT));
#endif

#ifdef MQTT_USERNAME
const char *mqtt_username = STR(MQTT_USERNAME);
#endif

#ifdef MQTT_PASSWORD
const char *mqtt_password = STR(MQTT_PASSWORD);
#endif

#ifdef DEVICE_NAME
const char *device_name = STR(DEVICE_NAME);
#endif

// Relay pre-defined command
#define RELAY_00_COMMAND_ON "relay 0 on"
#define RELAY_00_COMMAND_OFF "relay 0 off"
#define RELAY_01_COMMAND_ON "relay 1 on"
#define RELAY_01_COMMAND_OFF "relay 1 off"
#define RELAY_02_COMMAND_ON "relay 2 on"
#define RELAY_02_COMMAND_OFF "relay 2 off"
#define RELAY_03_COMMAND_ON "relay 3 on"
#define RELAY_03_COMMAND_OFF "relay 3 off"

// Relay Identification
enum Relay
{
  Relay_00 = 0,
  Relay_01 = 1,
  Relay_02 = 2,
  Relay_03 = 3
};

// Relay Id to Pin
enum RelayPin
{
  Relay_00_Pin = 26,
  Relay_01_Pin = 25,
  Relay_02_Pin = 27,
  Relay_03_Pin = 14
};

// Setting for NTP Time
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

/**
 * MQTT Broker Connection details
 * 1. Defined the topic for telemetry data (temperature and humidity)
 * 2. Defined the topic about status of the relay 0
 * 3. Defined the topic about status of the relay 1
 * 4. Defined the topic about status of the relay 2
 * 5. Defined the topic about status of the relay 3
 * 6. Defined the topic where the action command for the relay will be received
 */
const char *topic_telemetry_data = "esp32/telemetry_data";
const char *topic_relay_00_status = "esp32/relay_00_status";
const char *topic_relay_01_status = "esp32/relay_01_status";
const char *topic_relay_02_status = "esp32/relay_02_status";
const char *topic_relay_03_status = "esp32/relay_03_status";
const char *topic_command = "esp32/command";

// Prefix for the MQTT Client Identification
String clientId = "esp32-client-";

// Defined the value for the status of the relay (active 1, 0 otherwise)
const char *relay_status_on = "1";
const char *relay_status_off = "0";

// BME280
Adafruit_BME280 bme;

// Initi variables for values from sensor BME280
float temperature = 0;
float humidity = 0;
int pressure = 0;

// Interval in ms of the reads
int counter = 0;
long interval = 5000;
long lastMessage = 0;

// Declare the custom functions
void callback(char *topic, byte *message, unsigned int length);
void setup_wifi();
void update_relay_status(int relayId, const char *status);

// Init WiFi/WiFiUDP, NTP and MQTT Client
WiFiUDP ntpUDP;
WiFiClient espClient;
NTPClient timeClient(ntpUDP);
PubSubClient client(mqtt_server, mqtt_port, callback, espClient);

/**
 * MQTT Callback
 */
void callback(char *topic, byte *message, unsigned int length)
{
  Log.notice(F("Message arrived on topic: %s" CR), topic);
  Log.notice(F("Message Content:"));

  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }

  Serial.println(messageTemp);

  // If a message is received on the topic esp32/command (es. Relay off or on).
  // Changes the output state according to the message
  if ((String)topic == (String)topic_command)
  {
    if (messageTemp == RELAY_00_COMMAND_ON)
    {
      digitalWrite(Relay_00_Pin, LOW);
      update_relay_status(Relay_00, relay_status_on);

      Log.notice(F("Switch On relay 0" CR));
    }
    else if (messageTemp == RELAY_00_COMMAND_OFF)
    {
      digitalWrite(Relay_00_Pin, HIGH);
      update_relay_status(Relay_00, relay_status_off);

      Log.notice(F("Switch Off relay 0" CR));
    }
    else if (messageTemp == RELAY_01_COMMAND_ON)
    {
      digitalWrite(Relay_01_Pin, LOW);
      update_relay_status(Relay_01, relay_status_on);

      Log.notice(F("Switch On relay 1" CR));
    }
    else if (messageTemp == RELAY_01_COMMAND_OFF)
    {
      digitalWrite(Relay_01_Pin, HIGH);
      update_relay_status(Relay_01, relay_status_off);

      Log.notice(F("Switch Off relay 1" CR));
    }
    else if (messageTemp == RELAY_02_COMMAND_ON)
    {
      digitalWrite(Relay_02_Pin, LOW);
      update_relay_status(Relay_02, relay_status_on);

      Log.notice(F("Switch On relay 2" CR));
    }
    else if (messageTemp == RELAY_02_COMMAND_OFF)
    {
      digitalWrite(Relay_02_Pin, HIGH);
      update_relay_status(Relay_02, relay_status_off);

      Log.notice(F("Switch Off relay 2" CR));
    }
    else if (messageTemp == RELAY_03_COMMAND_ON)
    {
      digitalWrite(Relay_03_Pin, LOW);
      update_relay_status(Relay_03, relay_status_on);

      Log.notice(F("Switch On relay 3" CR));
    }
    else if (messageTemp == RELAY_03_COMMAND_OFF)
    {
      digitalWrite(Relay_03_Pin, HIGH);
      update_relay_status(Relay_03, relay_status_off);

      Log.notice(F("Switch Off relay 3" CR));
    }
    else
    {
      Log.warning(F("No command recognized" CR));
    }
  }
}

/**
 * Update Relay status on the topic
 * 
 * relayId: Identifiier of the relay
 * status: Status of the relay. (o or 1)
 */
void update_relay_status(int relayId, const char *status)
{
  // Allocate the JSON document
  // Inside the brackets, 200 is the RAM allocated to this document.
  // Don't forget to change this value to match your requirement.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<200> relayStatus;

  relayStatus["deviceName"] = device_name;
  relayStatus["time"] = timeClient.getEpochTime();
  relayStatus["relayId"] = relayId;
  relayStatus["status"] = status;

  char relayStatusAsJson[200];
  serializeJson(relayStatus, relayStatusAsJson);

  switch (relayId)
  {
  case Relay_00:
    client.publish(topic_relay_00_status, relayStatusAsJson);
    break;
  case Relay_01:
    client.publish(topic_relay_01_status, relayStatusAsJson);
    break;
  case Relay_02:
    client.publish(topic_relay_02_status, relayStatusAsJson);
    break;
  case Relay_03:
    client.publish(topic_relay_03_status, relayStatusAsJson);
    break;
  }
}

/**
 * Setup lifecycle
 */
void setup()
{
  Serial.begin(115200);

  // Initialize with log level and log output.
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);

  // Start I2C communication
  if (!bme.begin(0x76))
  {
    Serial.println("Could not find a BME280 sensor, check wiring!");
    while (1)
      ;
  }

  clientId += String(random(0xffff), HEX);

  // Connect to WiFi
  setup_wifi();

  // Setup PIN Mode for Relay
  pinMode(Relay_00_Pin, OUTPUT);
  pinMode(Relay_01_Pin, OUTPUT);
  pinMode(Relay_02_Pin, OUTPUT);
  pinMode(Relay_03_Pin, OUTPUT);

  // Init Relay
  digitalWrite(Relay_00_Pin, HIGH);
  digitalWrite(Relay_01_Pin, HIGH);
  digitalWrite(Relay_02_Pin, HIGH);
  digitalWrite(Relay_03_Pin, HIGH);

  // Init NTP
  timeClient.begin();
  timeClient.setTimeOffset(0);
}

/**
 * Setup the WiFi Connection
 */
void setup_wifi()
{
  // We start by connecting to a WiFi network
  Log.notice(F("Connecting to WiFi network: %s (password: %s)" CR), ssid, password);

  WiFi.begin(ssid, password);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(clientId.c_str());

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");

  Serial.println("WiFi connected :-)");
  Serial.print("IP Address: ");
  Serial.print(WiFi.localIP());
  Serial.println("");
  Serial.print("Mac Address: ");
  Serial.print(WiFi.macAddress());
  Serial.println("");
  Serial.print("Hostname: ");
  Serial.print(WiFi.getHostname());
  Serial.println("");
  Serial.print("Gateway: ");
  Serial.print(WiFi.gatewayIP());
  Serial.println("");

  bool success = Ping.ping(mqtt_server, 3);

  if (!success)
  {
    Log.error(F("Ping failed to %s" CR), mqtt_server);
    return;
  }

  Log.notice(F("Ping OK to %s" CR), mqtt_server);
}

/**
 * Reconnect to MQTT Broker
 */
void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Log.notice(F("Attempting MQTT connection to %s" CR), mqtt_server);

    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password))
    {
      Log.notice(F("Connected as clientId %s :-)" CR), clientId.c_str());

      // Subscribe
      client.subscribe(topic_command);
      Log.notice(F("Subscribe to the topic command %s " CR), topic_command);

      // Init Status topic for Relay
      update_relay_status(Relay_00, relay_status_off);
      update_relay_status(Relay_01, relay_status_off);
      update_relay_status(Relay_02, relay_status_off);
      update_relay_status(Relay_03, relay_status_off);
    }
    else
    {
      Log.error(F("{failed, rc=%d try again in 5 seconds}" CR), client.state());
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/**
 * Loop lifecycle
 */
void loop()
{
  while (!timeClient.update())
  {
    timeClient.forceUpdate();
  }

  long now = millis();
  
  if (!client.connected())
  {
    reconnect();
  }
  
  client.loop();

  if (now - lastMessage > interval)
  {
    lastMessage = now;

    // Allocate the JSON document
    // Inside the brackets, 200 is the RAM allocated to this document.
    // Don't forget to change this value to match your requirement.
    // Use arduinojson.org/v6/assistant to compute the capacity.
    StaticJsonDocument<200> telemetry;

    /**
     * Reading humidity, temperature and pressure
     * Temperature is always a floating point, in Centigrade. Pressure is a 
     * 32 bit integer with the pressure in Pascals. You may need to convert 
     * to a different value to match it with your weather report. Humidity is 
     * in % Relative Humidity
     */
    temperature = bme.readTemperature();
    humidity = bme.readHumidity();
    pressure = bme.readPressure();

    telemetry["clientId"] = clientId.c_str();
    telemetry["deviceName"] = device_name;
    telemetry["time"] = timeClient.getEpochTime();
    telemetry["temperature"] = temperature;
    telemetry["humidity"] = humidity;
    telemetry["pressure"] = pressure;
    telemetry["interval"] = interval;
    telemetry["counter"] = ++counter;

    char telemetryAsJson[200];
    serializeJson(telemetry, telemetryAsJson);

    client.publish(topic_telemetry_data, telemetryAsJson);

    serializeJsonPretty(telemetry, Serial);
    Serial.println();
  }
}