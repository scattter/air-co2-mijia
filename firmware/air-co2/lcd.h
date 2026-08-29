#pragma once

#include <Arduino.h>

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b);
void lcdInit(bool on = true);
void lcdSetOn(bool on);
void lcdFillRect(int x, int y, int w, int h, uint16_t color);
void lcdDrawHLine(int x, int y, int w, uint16_t color);
void lcdDrawVLine(int x, int y, int h, uint16_t color);
void lcdDrawRect(int x, int y, int w, int h, uint16_t color);
void drawText(int x, int y, const char* text, uint16_t color, uint16_t bg, int scale);
