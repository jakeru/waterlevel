// waterlevel.cpp

// Written by Jakob Ruhe (jakob.ruhe@gmail.com) in January 2020.

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

#include "PingSerial.h"
#include <SoftwareSerial.h>

#include "config.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

// WiFi
static WiFiClient s_espClient;
static bool s_connected;

// MQTT
static PubSubClient s_mqttClient(s_espClient);
static uint32_t s_timeLastConnect;
static bool s_hasTriedToConnect;
static bool s_mqttConnected;

// SoftwareSerial
SoftwareSerial s_swSerial(PIN_MCU_FROM_US, PIN_MCU_TO_US);

// PingSerial
const uint16_t US_DIST_MIN_MM = 100;
const uint16_t US_DIST_MAX_MM = 5000;
PingSerial s_us100(s_swSerial, US_DIST_MIN_MM, US_DIST_MAX_MM);

// Webserver
AsyncWebServer s_webServer(80);

// State
static uint32_t s_lastBlinkAt;

const uint32_t SENSOR_DIST_INTERVAL = 2000;
static uint32_t s_lastDistRequest;
static int32_t s_distance;

const uint32_t SENSOR_TEMP_INTERVAL = 2000;
static uint32_t s_lastTempRequest;
static int32_t s_temperature;

static bool timeAtOrAfter(uint32_t t, uint32_t now)
{
    return (int32_t)(now - t) >= 0;
}

static void setStatusLed(bool state)
{
    digitalWrite(PIN_LED, state ? LOW : HIGH);
}

static void setupWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.hostname(WIFI_HOSTNAME);
    Serial.printf("Connecting to %s...\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

static void loopWiFi()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        if (!s_connected)
        {
            s_connected = true;
            Serial.print("Connected to WiFi: ");
            Serial.println(WIFI_SSID);
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
        }
    }
    else if (s_connected)
    {
        Serial.println("WiFi: Connection lost. Trying to reconnect...");
        s_connected = false;
    }
}

static void setupOTA()
{
    ArduinoOTA.onStart([]() {
        Serial.println("OTA started");
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nOTA completed");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("\nOTA Error: #%u: ", error);
        if (error == OTA_AUTH_ERROR)
            Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
            Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
            Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
            Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
            Serial.println("End Failed");
        else
            Serial.println("Unknown error");
    });
    ArduinoOTA.begin();
}

static void publishStatus()
{
    s_mqttClient.publish(MQTT_STATUS_TOPIC, "online", true);
}

static void publishDistance()
{
    s_mqttClient.publish(MQTT_DISTANCE_TOPIC, String(s_distance).c_str(), true);
}

static void publishTemperature()
{
    s_mqttClient.publish(MQTT_TEMPERATURE_TOPIC, String(s_temperature).c_str(), true);
}

static void setupMQTT()
{
    s_mqttClient.setServer(MQTT_SERVER, MQTT_SERVER_PORT);
}

static bool connectMQTT()
{
    Serial.printf("Connecting to mqtt %s:%d...\n",
                  MQTT_SERVER, MQTT_SERVER_PORT);
    if (!s_mqttClient.connect(
            MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD,
            MQTT_STATUS_TOPIC, 0, 1, "offline"))
    {
        Serial.print("Failed to connect to MQTT server: ");
        Serial.println(s_mqttClient.state());
        return false;
    }

    s_mqttConnected = true;
    Serial.println("Connected to MQTT server");
    publishStatus();
    publishDistance();
    publishTemperature();

    return true;
}

static void loopMQTT()
{
    uint32_t now = millis();
    s_mqttClient.loop();
    if (!s_mqttClient.connected())
    {
        if (s_mqttConnected)
        {
            Serial.println("Lost connection with MQTT server");
            s_mqttConnected = false;
        }
        if (!s_hasTriedToConnect || timeAtOrAfter(s_timeLastConnect + 60000, now))
        {
            connectMQTT();
            s_hasTriedToConnect = true;
            s_timeLastConnect = now;
        }
        return;
    }
}

static void setupSensor()
{
    s_us100.begin();
}

static void setupWebserver()
{
    s_webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.html");
    });
    s_webServer.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/plain", String(s_temperature).c_str());
    });
    s_webServer.on("/distance", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/plain", String(s_distance).c_str());
    });

    s_webServer.begin();
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(115200);
    Serial.println("Booting");

    if (!SPIFFS.begin())
    {
        Serial.println("Failed to initialize SPIFFS");
    }

    setupWiFi();
    setupOTA();
    setupMQTT();
    setupSensor();
    setupWebserver();
}

static void callbackForMQTT(char *topic, byte *bytes, unsigned int length)
{
    String payload;
    for (size_t i = 0; i < length; i++)
    {
        payload += (char)bytes[i];
    }
    Serial.printf("Topic: %s, payload: %s\n", topic, payload.c_str());
}

static void loopSensor()
{
    uint8_t data_available = s_us100.data_available();

    if (data_available & DISTANCE)
    {
        s_distance = s_us100.get_distance();
        Serial.print("Distance: ");
        Serial.println(s_distance);
        publishDistance();
    }

    if (data_available & TEMPERATURE)
    {
        s_temperature = s_us100.get_temperature();
        Serial.print("Temperature: ");
        Serial.println(s_temperature);
        publishTemperature();
    }

    uint32_t now = millis();

    if (timeAtOrAfter(s_lastDistRequest + SENSOR_DIST_INTERVAL, now))
    {
        s_lastDistRequest = now;
        s_us100.request_distance();
    }

    if (timeAtOrAfter(s_lastTempRequest + SENSOR_TEMP_INTERVAL, now))
    {
        s_lastTempRequest = now;
        s_us100.request_temperature();
    }
}

static void loopStatusLed()
{
    // Blink slow when we are connected to the WiFi.
    // Blink fast otherwise.
    const uint32_t offTime = s_connected ? 2000 : 200;
    const uint32_t onTime = 200;
    uint32_t now = millis();
    if (timeAtOrAfter(s_lastBlinkAt + offTime + onTime, now))
    {
        s_lastBlinkAt = now;
        setStatusLed(false);
    }
    else if (timeAtOrAfter(s_lastBlinkAt + offTime, now))
    {
        setStatusLed(true);
    }
}

void loop()
{
    ArduinoOTA.handle();
    loopWiFi();
    loopMQTT();
    loopSensor();
    loopStatusLed();
}
