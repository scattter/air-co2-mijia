#pragma once

#include <Arduino.h>

constexpr int PIN_LCD_MOSI = 45;
constexpr int PIN_LCD_SCLK = 40;
constexpr int PIN_LCD_CS = 42;
constexpr int PIN_LCD_DC = 41;
constexpr int PIN_LCD_RST = 39;
constexpr int PIN_LCD_BL = 46;

constexpr int PIN_SCD_SDA = 8;
constexpr int PIN_SCD_SCL = 9;
constexpr uint32_t SCD_CLOCK = 100000;

// 配置门户触发按钮（开机长按进入配置模式）。默认 GPIO0 = 开发板 BOOT 键。
// 设为 -1 可禁用"按钮触发"，只保留"首次未配置自动进入门户"。
constexpr int PIN_CFG_BUTTON = 0;

// 联网后是否启用网页配置服务：WiFi 连上后访问 http://<设备IP>/ 即可随时
// 重新配置（含 MQTT 服务器/账号密码），无需按 BOOT 键。
// 注意：该页面无登录保护，局域网内任何人可访问；如介意可设为 false。
constexpr bool CONFIG_SERVER_ENABLED = true;

constexpr int LCD_WIDTH = 172;
constexpr int LCD_HEIGHT = 320;
constexpr int LCD_X_OFFSET = 34;
constexpr int LCD_Y_OFFSET = 0;

constexpr uint16_t COLOR_BLACK = 0x0000;
constexpr uint16_t COLOR_WHITE = 0xffff;
constexpr uint16_t COLOR_RED = 0xf800;
constexpr uint16_t COLOR_GREEN = 0x07e0;
constexpr uint16_t COLOR_YELLOW = 0xffe0;
constexpr uint16_t COLOR_GRAY = 0x8410;
