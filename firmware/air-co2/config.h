#pragma once

// 应用配置结构 + NVS 持久化管理。
// 配置默认值来自 config.example.h（编译期），运行时可经门户网页改写并保存到 NVS，
// 重启后读取 NVS 覆盖默认值 —— 无需重新烧录即可改变 Wi-Fi / 服务器 / MQTT 等连接。

#include <Arduino.h>
#include <Preferences.h>

struct AppConfig {
  // 连接配置
  String wifiSsid;
  String wifiPass;
  String serverUrl;
  String deviceId;
  // MQTT（HA 集成），host 为空表示不启用 MQTT
  String mqttHost;
  uint16_t mqttPort;
  String mqttUser;
  String mqttPass;
  // 其它
  bool screenDefault;   // 默认屏幕开关状态
};

// 在 config.example.h 中声明的编译期默认值
void loadDefaults(AppConfig& cfg);

// NVS 命名空间：ConfigManager 与顶层"屏幕状态"Preferences 共用同一个命名空间，
// 二者键名互不冲突（配置键 ssid/pass/srv/devid/... 与运行时屏幕键 screen）。
inline constexpr const char* kConfigNamespace = "airco2";

class ConfigManager {
 public:
  void begin(const AppConfig& defaults);
  void load(AppConfig& out);        // 读取 NVS（无则用 defaults，回写）
  bool save(const AppConfig& cfg);  // 保存到 NVS
  void clear();                     // 清空 NVS 配置

 private:
  Preferences prefs_;
};
