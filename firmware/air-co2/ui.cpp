#include "ui.h"

#include <string.h>

#include "board_config.h"
#include "lcd.h"

namespace {

const uint16_t UI_BG = rgb565(0x07, 0x14, 0x25);
const uint16_t HEADER_BG = rgb565(0x11, 0x18, 0x27);
const uint16_t CARD_BG = rgb565(0x14, 0x1c, 0x2c);
const uint16_t INFO_BG = rgb565(0x10, 0x23, 0x31);
char sensorText[20] = "";
char netText[24] = "";

void drawCentered(int y, const char* text, uint16_t color, uint16_t bg, int scale) {
  const int width = strlen(text) * 6 * scale;
  drawText((LCD_WIDTH - width) / 2, y, text, color, bg, scale);
}

void drawBorder(uint16_t color) {
  lcdDrawRect(8, 58, 156, 112, color);
  lcdDrawRect(9, 59, 154, 110, color);
}

}  // namespace

AirLevel getAirLevel(uint16_t co2) {
  if (co2 <= 800) return AIR_GOOD;
  if (co2 <= 1000) return AIR_NOTICE;
  return AIR_HIGH;
}

void drawAirScreen() {
  lcdFillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, UI_BG);
  lcdFillRect(8, 10, 156, 38, HEADER_BG);
  lcdDrawRect(8, 10, 156, 38, COLOR_WHITE);
  drawCentered(19, "CO2 MONITOR", COLOR_WHITE, HEADER_BG, 2);

  lcdFillRect(8, 58, 156, 112, CARD_BG);
  drawBorder(COLOR_GRAY);
  drawCentered(69, "CO2", COLOR_GRAY, CARD_BG, 2);
  drawCentered(103, "----", COLOR_WHITE, CARD_BG, 4);
  drawCentered(143, "PPM", COLOR_GRAY, CARD_BG, 1);

  lcdFillRect(8, 178, 156, 30, COLOR_GRAY);
  drawCentered(190, "NO DATA", COLOR_BLACK, COLOR_GRAY, 1);

  lcdFillRect(8, 218, 74, 54, INFO_BG);
  lcdFillRect(90, 218, 74, 54, INFO_BG);
  lcdDrawRect(8, 218, 74, 54, COLOR_GRAY);
  lcdDrawRect(90, 218, 74, 54, COLOR_GRAY);
  drawText(16, 226, "TEMP", COLOR_GRAY, INFO_BG, 1);
  drawText(98, 226, "HUM", COLOR_GRAY, INFO_BG, 1);
  drawText(16, 248, "--.- C", COLOR_WHITE, INFO_BG, 1);
  drawText(98, 248, "--.- RH", COLOR_WHITE, INFO_BG, 1);

  lcdFillRect(8, 282, 156, 26, COLOR_BLACK);
  lcdDrawRect(8, 282, 156, 26, COLOR_GRAY);
  sensorText[0] = '\0';
  netText[0] = '\0';
}

void drawAirLevel(AirLevel level) {
  uint16_t color = COLOR_GRAY;
  const char* text = "NO DATA";
  if (level == AIR_GOOD) {
    color = COLOR_GREEN;
    text = "GOOD";
  } else if (level == AIR_NOTICE) {
    color = COLOR_YELLOW;
    text = "NOTICE";
  } else if (level == AIR_HIGH) {
    color = COLOR_RED;
    text = "HIGH";
  }

  drawBorder(color);
  lcdFillRect(8, 178, 156, 30, color);
  drawCentered(190, text, level == AIR_NOTICE ? COLOR_BLACK : COLOR_WHITE, color, 1);
}

void drawAirSample(uint16_t co2, float temp, float humidity) {
  char value[12];
  snprintf(value, sizeof(value), "%u", co2);
  lcdFillRect(12, 93, 148, 42, CARD_BG);
  drawCentered(103, value, COLOR_WHITE, CARD_BG, 4);

  char tempText[16];
  char humText[16];
  snprintf(tempText, sizeof(tempText), "%.1f C", temp);
  snprintf(humText, sizeof(humText), "%.1f RH", humidity);
  lcdFillRect(12, 244, 66, 20, INFO_BG);
  lcdFillRect(94, 244, 66, 20, INFO_BG);
  drawText(16, 248, tempText, COLOR_WHITE, INFO_BG, 1);
  drawText(98, 248, humText, COLOR_WHITE, INFO_BG, 1);
  drawAirLevel(getAirLevel(co2));
}

void drawSensorState(const char* text, uint16_t color) {
  if (strcmp(sensorText, text) == 0) return;
  snprintf(sensorText, sizeof(sensorText), "%s", text);
  lcdFillRect(12, 286, 70, 16, COLOR_BLACK);
  drawText(14, 291, sensorText, color, COLOR_BLACK, 1);
}

void drawNetState(NetState state, int httpCode) {
  char text[24];
  uint16_t color = COLOR_GRAY;
  if (state == NET_CONFIG) {
    snprintf(text, sizeof(text), "WIFI CONFIG");
  } else if (state == NET_CONNECTING) {
    snprintf(text, sizeof(text), "WIFI WAIT");
    color = COLOR_YELLOW;
  } else if (state == NET_ONLINE && httpCode > 0) {
    snprintf(text, sizeof(text), "POST %d", httpCode);
    color = httpCode == 200 || httpCode == 202 ? COLOR_GREEN : COLOR_YELLOW;
  } else if (state == NET_ERROR && httpCode < 0) {
    snprintf(text, sizeof(text), "POST ERR");
    color = COLOR_RED;
  } else if (state == NET_ONLINE) {
    snprintf(text, sizeof(text), "WIFI OK");
    color = COLOR_GREEN;
  } else {
    snprintf(text, sizeof(text), "WIFI ERR");
    color = COLOR_RED;
  }

  if (strcmp(netText, text) == 0) return;
  snprintf(netText, sizeof(netText), "%s", text);
  lcdFillRect(86, 286, 74, 16, COLOR_BLACK);
  drawText(88, 291, netText, color, COLOR_BLACK, 1);
}
