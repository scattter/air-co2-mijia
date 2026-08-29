#pragma once

#include <Arduino.h>

enum AirLevel { AIR_GOOD, AIR_NOTICE, AIR_HIGH, AIR_UNKNOWN };
enum NetState { NET_CONFIG, NET_CONNECTING, NET_ONLINE, NET_ERROR };

AirLevel getAirLevel(uint16_t co2);
void drawAirScreen();
void drawAirSample(uint16_t co2, float temp, float humidity);
void drawAirLevel(AirLevel level);
void drawSensorState(const char* text, uint16_t color);
void drawNetState(NetState state, int httpCode = 0);
