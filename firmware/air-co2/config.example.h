#pragma once

// ────────────────────────────────────────────────────────────────────
// 编译期默认配置（仅作首次烧录、NVS 为空时的回退值）。
// 真正使用的连接配置保存在 ESP32 的 NVS 中，可通过"配置门户网页"修改，
// 无需重新烧录即可改变。
// 若要改变回退默认值，直接编辑下面这些 *_DEFAULT 宏即可（无需复制本文件）。
// ────────────────────────────────────────────────────────────────────

// 默认 Wi-Fi（实际联网以 NVS/网页配置为准）
#define WIFI_SSID_DEFAULT "YOUR_WIFI_SSID"
#define WIFI_PASS_DEFAULT "YOUR_WIFI_PASSWORD"

// 默认上报服务器地址
#define SERVER_URL_DEFAULT "http://192.168.0.110:7311/api/air/sample"
#define DEVICE_ID_DEFAULT "lcd147-air-01"

// 默认 MQTT（Home Assistant 集成），host 为空表示默认不启用
#define MQTT_HOST_DEFAULT ""
#define MQTT_PORT_DEFAULT 1883
#define MQTT_USER_DEFAULT ""
#define MQTT_PASS_DEFAULT ""

// 默认屏幕开关状态
#define SCREEN_DEFAULT true
