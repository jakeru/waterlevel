// config.h

#pragma once

// Pins
const int PIN_WEMOS_D1 = 5;
const int PIN_WEMOS_D2 = 4;
const int PIN_WEMOS_D4 = 2;
const int PIN_WEMOS_D5 = 14;
const int PIN_WEMOS_D6 = 12;

// The status LED
const int PIN_LED = PIN_WEMOS_D4;

// Ultrasound Sensor
const int PIN_MCU_TO_US = PIN_WEMOS_D5;
const int PIN_MCU_FROM_US = PIN_WEMOS_D6;

// WiFi settings
#define WIFI_SSID "WIFI_SSID_MUST_BE_SET"
#define WIFI_PASSWORD "WIFI_PASSWORD_MUST_BE_SET"
#define WIFI_HOSTNAME "waterlevel"

// MQTT server
#define MQTT_SERVER "atom.home"
#define MQTT_SERVER_PORT 1883

// MQTT client name and account credentials
#define MQTT_CLIENT_ID "waterlevel"
#define MQTT_USER ""
#define MQTT_PASSWORD ""

// MQTT topics
#define MQTT_BASE_TOPIC "waterlevel/"
#define MQTT_STATUS_TOPIC MQTT_BASE_TOPIC "status"
#define MQTT_DISTANCE_TOPIC MQTT_BASE_TOPIC "distance"
#define MQTT_TEMPERATURE_TOPIC MQTT_BASE_TOPIC "temperature"
