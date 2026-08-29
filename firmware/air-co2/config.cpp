#include "config.h"
#include "config.example.h"

void loadDefaults(AppConfig& cfg) {
  cfg.wifiSsid = WIFI_SSID_DEFAULT;
  cfg.wifiPass = WIFI_PASS_DEFAULT;
  cfg.serverUrl = SERVER_URL_DEFAULT;
  cfg.deviceId = DEVICE_ID_DEFAULT;
  cfg.mqttHost = MQTT_HOST_DEFAULT;
  cfg.mqttPort = MQTT_PORT_DEFAULT;
  cfg.mqttUser = MQTT_USER_DEFAULT;
  cfg.mqttPass = MQTT_PASS_DEFAULT;
  cfg.screenDefault = SCREEN_DEFAULT;
}

namespace {
constexpr const char* KEY_VER = "cfg_ver";
constexpr const char* KEY_SSID = "ssid";
constexpr const char* KEY_PASS = "pass";
constexpr const char* KEY_URL = "srv";
constexpr const char* KEY_DEV = "devid";
constexpr const char* KEY_MHOST = "mqttp";
constexpr const char* KEY_MPORT = "mqttpt";
constexpr const char* KEY_MUSER = "mqttu";
constexpr const char* KEY_MPASS = "mqttpw";
constexpr const char* KEY_SCREEN = "scr";
constexpr int32_t CONFIG_VERSION = 1;
}  // namespace

String prefs_get_string(Preferences& p, const char* key, const String& def) {
  return p.isKey(key) ? p.getString(key, "") : def;
}

void ConfigManager::begin(const AppConfig& defaults) {
  prefs_.begin(kConfigNamespace, false);
  // 配置版本升级时，若已存配置与当前结构冲突可在此处理。
  prefs_.putInt(KEY_VER, CONFIG_VERSION);
  (void)defaults;
}

void ConfigManager::load(AppConfig& out) {
  AppConfig d;
  loadDefaults(d);

  out.wifiSsid = prefs_get_string(prefs_, KEY_SSID, d.wifiSsid);
  out.wifiPass = prefs_get_string(prefs_, KEY_PASS, d.wifiPass);
  out.serverUrl = prefs_get_string(prefs_, KEY_URL, d.serverUrl);
  out.deviceId = prefs_get_string(prefs_, KEY_DEV, d.deviceId);
  out.mqttHost = prefs_get_string(prefs_, KEY_MHOST, d.mqttHost);
  out.mqttPort = (uint16_t)prefs_.getUShort(KEY_MPORT, d.mqttPort);
  out.mqttUser = prefs_get_string(prefs_, KEY_MUSER, d.mqttUser);
  out.mqttPass = prefs_get_string(prefs_, KEY_MPASS, d.mqttPass);
  out.screenDefault = prefs_.getBool(KEY_SCREEN, d.screenDefault);
}

bool ConfigManager::save(const AppConfig& cfg) {
  bool ok = true;
  ok &= prefs_.putString(KEY_SSID, cfg.wifiSsid);
  ok &= prefs_.putString(KEY_PASS, cfg.wifiPass);
  ok &= prefs_.putString(KEY_URL, cfg.serverUrl);
  ok &= prefs_.putString(KEY_DEV, cfg.deviceId);
  ok &= prefs_.putString(KEY_MHOST, cfg.mqttHost);
  ok &= prefs_.putUShort(KEY_MPORT, cfg.mqttPort);
  ok &= prefs_.putString(KEY_MUSER, cfg.mqttUser);
  ok &= prefs_.putString(KEY_MPASS, cfg.mqttPass);
  ok &= prefs_.putBool(KEY_SCREEN, cfg.screenDefault);
  ok &= prefs_.putInt(KEY_VER, CONFIG_VERSION);
  return ok;
}

void ConfigManager::clear() { prefs_.clear(); }
