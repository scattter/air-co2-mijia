#include <Arduino.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <SensirionI2cScd4x.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <sys/time.h>

#include "board_config.h"
#include "config.h"           // AppConfig / ConfigManager（NVS 运行时配置）
#include "config.example.h"   // *_DEFAULT 编译期默认值来源（首次烧录时使用）
#include "config_portal.h"    // 配置门户（SoftAP + captive portal）
#include "lcd.h"
#include "ui.h"

#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR 0

namespace {

constexpr uint32_t READ_MS = 250;
constexpr uint32_t WIFI_RETRY_MS = 10000;
constexpr uint32_t SCREEN_SYNC_MS = 3000;
constexpr uint32_t SENSOR_RETRY_MS = 30000;
constexpr uint32_t POST_GAP_MS = 250;
constexpr uint32_t POST_RETRY_MS = 5000;
constexpr uint16_t HTTP_TIMEOUT_MS = 2000;
constexpr uint16_t SAMPLE_CAP = 360;
constexpr uint8_t MAX_SENSOR_ERR = 3;
constexpr time_t MIN_EPOCH = 1700000000;
constexpr char FW_VERSION[] = "0.1.1-mqtt";

// MQTT（Home Assistant 集成）
constexpr uint32_t MQTT_RETRY_MS = 5000;
constexpr uint32_t MQTT_SCREEN_OFF_MS = 15000;  // 防止回声循环的最小间隔
constexpr char MQTT_TOPIC_ROOT[] = "home/airco2";
constexpr char MQTT_DISCOVERY_ROOT[] = "homeassistant/sensor";
constexpr char MQTT_DISCOVERY_SWITCH_ROOT[] = "homeassistant/switch";

struct AirSample {
  uint64_t seq;
  uint64_t timestamp;
  uint16_t co2;
  float temp;
  float humidity;
  bool screenOn;
};

SensirionI2cScd4x sensor;
Preferences prefs;
AirSample sampleBuf[SAMPLE_CAP];
char errorMsg[64];
uint32_t lastReadAt = 0;
uint32_t lastWifiAt = 0;
uint32_t lastScreenAt = 0;
uint32_t lastSensorAt = 0;
uint32_t lastPostAt = 0;
uint32_t postWait = 0;
uint16_t bufHead = 0;
uint16_t bufCount = 0;
uint8_t sensorErrs = 0;
bool sensorReady = false;
bool wifiOnline = false;
bool ntpReady = false;
bool prefsReady = false;
bool screenOn = true;
String screenUrl;

// ── 运行时配置（NVS 持久化，经 ConfigManager / 配置门户管理）──
ConfigManager configManager;
AppConfig cfg;
bool mqttEnabled = false;  // cfg.mqttHost 非空时为 true

// MQTT 客户端（mqttEnabled 为 true 时才启用）
WiFiClient mqttNet;
PubSubClient mqtt(mqttNet);
bool mqttReady = false;
bool mqttDiscoverySent = false;
uint32_t lastMqttAt = 0;
uint32_t lastScreenPubAt = 0;

// ── 出厂重置：正常运行时长按配置按钮（BOOT）5 秒 → 清空 NVS 并重启进配置门户 ──
// PIN_CFG_BUTTON < 0 时该功能一并禁用。
constexpr uint32_t RESET_CONFIRM_MS = 3000;  // 按住 3 秒起在串口倒计时提示
constexpr uint32_t RESET_HOLD_MS = 5000;     // 长按阈值：累计按住 5 秒触发
uint32_t btnDownAt = 0;        // 按钮变为按下的时刻（0 = 未在计时）
uint32_t lastResetTickAt = 0;  // 上一次倒计时提示的秒数
bool btnWasLow = false;        // 上一次读取到的按钮电平

void printBus(const char* action) {
  const int sda = digitalRead(PIN_SCD_SDA);
  const int scl = digitalRead(PIN_SCD_SCL);
  Serial.printf(
      "[I2C] %s: SDA(GPIO%d)=%s SCL(GPIO%d)=%s\n",
      action,
      PIN_SCD_SDA,
      sda == HIGH ? "HIGH" : "LOW",
      PIN_SCD_SCL,
      scl == HIGH ? "HIGH" : "LOW");
  if (sda == LOW || scl == LOW) {
    Serial.println("[I2C] warning: a LOW line may indicate a short, missing pull-up, or stuck device");
  }
}

void scanI2c() {
  bool found = false;
  uint8_t count = 0;
  Serial.println("[I2C] scanning addresses 0x01-0x7E");

  for (uint8_t address = 1; address < 127; address += 1) {
    Wire.beginTransmission(address);
    const uint8_t error = Wire.endTransmission();
    if (error == 0) {
      Serial.printf("[I2C] found device at 0x%02X\n", static_cast<unsigned int>(address));
      found = found || address == SCD40_I2C_ADDR_62;
      count += 1;
    } else if (error == 4) {
      Serial.printf(
          "[I2C] bus error while probing 0x%02X\n", static_cast<unsigned int>(address));
    }
  }

  Serial.printf(
      "[I2C] scan complete: devices=%u target=0x%02X %s\n",
      static_cast<unsigned int>(count),
      static_cast<unsigned int>(SCD40_I2C_ADDR_62),
      found ? "FOUND" : "NOT FOUND");
  if (!found) {
    Serial.println(
        "[I2C] check VDD->3V3, GND->GND/common ground, SDA->GPIO8, SCL->GPIO9, and 3.3V pull-ups");
  }
}

void printError(const char* action, int16_t error) {
  errorToString(error, errorMsg, sizeof(errorMsg));
  Serial.printf(
      "[SCD40] %s failed: code=%d (0x%04X) message=%s\n",
      action,
      error,
      static_cast<unsigned int>(static_cast<uint16_t>(error)),
      errorMsg);
  printBus("driver error");
}

bool initSensor() {
  Serial.printf(
      "[I2C] config: SDA=GPIO%d SCL=GPIO%d clock=%luHz target=0x%02X\n",
      PIN_SCD_SDA,
      PIN_SCD_SCL,
      static_cast<unsigned long>(SCD_CLOCK),
      static_cast<unsigned int>(SCD40_I2C_ADDR_62));
  const bool started = Wire.begin(PIN_SCD_SDA, PIN_SCD_SCL);
  Serial.printf("[I2C] Wire.begin: %s\n", started ? "OK" : "FAILED");
  if (!started) {
    Serial.println("[I2C] controller initialization failed; check SDA/SCL pin configuration");
    return false;
  }

  Wire.setClock(SCD_CLOCK);
  printBus("idle levels");
  scanI2c();
  sensor.begin(Wire, SCD40_I2C_ADDR_62);
  delay(30);

  int16_t error = sensor.stopPeriodicMeasurement();
  if (error != NO_ERROR) {
    printError("stopPeriodicMeasurement", error);
    return false;
  }
  delay(500);

  error = sensor.reinit();
  if (error != NO_ERROR) {
    printError("reinit", error);
    return false;
  }
  delay(20);

  uint64_t serialNumber = 0;
  error = sensor.getSerialNumber(serialNumber);
  if (error != NO_ERROR) {
    printError("getSerialNumber", error);
    return false;
  }

  error = sensor.startPeriodicMeasurement();
  if (error != NO_ERROR) {
    printError("startPeriodicMeasurement", error);
    return false;
  }

  Serial.printf(
      "[SCD40] ready, serial=0x%08lX%08lX, SDA=%u, SCL=%u\n",
      static_cast<uint32_t>(serialNumber >> 32),
      static_cast<uint32_t>(serialNumber),
      PIN_SCD_SDA,
      PIN_SCD_SCL);
  sensorErrs = 0;
  return true;
}

void publishScreenState();

void setScreen(bool on) {
  if (screenOn == on) return;

  screenOn = on;
  lcdSetOn(on);
  if (prefsReady) prefs.putBool("screen", on);
  Serial.printf("[LCD] screen %s\n", on ? "ON" : "OFF");
  if (mqttEnabled) publishScreenState();
}

String makeScreenUrl() {
  String url(cfg.serverUrl);
  const int apiAt = url.indexOf("/api/air/sample");
  if (apiAt < 0) {
    Serial.println("[LCD] screen control disabled: server url path is unsupported");
    return String();
  }

  url.remove(apiAt);
  url += "/api/air/devices/";
  url += cfg.deviceId;
  url += "/screen";
  return url;
}

bool hasWifiConfig() {
  return cfg.wifiSsid.length() > 0 && cfg.wifiPass.length() > 0;
}

void connectWifi() {
  if (!hasWifiConfig()) {
    Serial.println("[WiFi] configuration is missing SSID or password");
    drawNetState(NET_CONFIG);
    return;
  }

  lastWifiAt = millis();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(cfg.wifiSsid.c_str(), cfg.wifiPass.c_str());
  Serial.printf("[WiFi] connecting to %s\n", cfg.wifiSsid.c_str());
  drawNetState(NET_CONNECTING);
}

void updateWifi() {
  const bool connected = WiFi.status() == WL_CONNECTED;
  if (connected && !wifiOnline) {
    wifiOnline = true;
    Serial.printf("[WiFi] connected, IP=%s\n", WiFi.localIP().toString().c_str());
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("[NTP] synchronizing");
    drawNetState(NET_ONLINE);
    // 联网后同样开放配置网页：http://<设备IP>/ 可随时改 Wi-Fi/MQTT 等配置，
    // 无需按 BOOT 键（见 config_portal.cpp beginConfigServer）。
    if (CONFIG_SERVER_ENABLED) {
      beginConfigServer(cfg);
      Serial.printf("[CFG] open http://%s/ to reconfigure\n", WiFi.localIP().toString().c_str());
    }
    return;
  }

  if (!connected && wifiOnline) {
    wifiOnline = false;
    ntpReady = false;
    Serial.println("[WiFi] disconnected");
    drawNetState(NET_ERROR);
  }

  if (!connected && hasWifiConfig() && millis() - lastWifiAt >= WIFI_RETRY_MS) {
    connectWifi();
  }
}

// 正常运行时长按配置按钮（BOOT）RESET_HOLD_MS（5 秒）→ 清空 NVS 并重启。
// 重启后 NVS 为空，shouldEnterConfigPortal() 判定"未配置"→ 自动进入配置门户（AP）。
// 中途松开即取消计时。
void updateFactoryResetButton() {
  if (PIN_CFG_BUTTON < 0) return;

  const bool low = digitalRead(PIN_CFG_BUTTON) == LOW;
  if (low != btnWasLow) {
    btnWasLow = low;
    if (low) {
      btnDownAt = millis();
      lastResetTickAt = 0;
    } else {
      btnDownAt = 0;  // 松开：取消计时
      lastResetTickAt = 0;
    }
  }
  if (!btnWasLow || btnDownAt == 0) return;

  const uint32_t held = millis() - btnDownAt;
  if (held >= RESET_HOLD_MS) {
    Serial.println("[CFG] factory reset: clearing NVS and rebooting into config portal...");
    drawSensorState("FACTORY", COLOR_RED);
    configManager.clear();
    delay(800);
    ESP.restart();
  }
  if (held >= RESET_CONFIRM_MS) {
    const uint32_t remain = (RESET_HOLD_MS - held) / 1000 + 1;  // 3 → 2 → 1
    if (remain != lastResetTickAt) {
      lastResetTickAt = remain;
      Serial.printf("[CFG] factory reset in %lu s...\n", static_cast<unsigned long>(remain));
    }
  }
}

void syncScreen() {
  if (WiFi.status() != WL_CONNECTED || screenUrl.isEmpty() ||
      millis() - lastScreenAt < SCREEN_SYNC_MS) {
    return;
  }

  lastScreenAt = millis();
  String url = screenUrl;
  url += screenOn ? "?actual=true" : "?actual=false";

  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  if (!http.begin(url)) {
    Serial.println("[LCD] failed to initialize screen request");
    return;
  }

  const int status = http.GET();
  if (status == 200) {
    String response = http.getString();
    response.trim();
    if (response == "true") {
      setScreen(true);
    } else if (response == "false") {
      setScreen(false);
    } else {
      Serial.printf("[LCD] invalid screen response=%s\n", response.c_str());
    }
  } else if (status < 0) {
    Serial.printf("[LCD] screen request failed=%s\n", HTTPClient::errorToString(status).c_str());
  } else {
    Serial.printf("[LCD] screen request status=%d\n", status);
  }
  http.end();
}

uint64_t getTimestamp() {
  timeval now{};
  gettimeofday(&now, nullptr);
  if (now.tv_sec < MIN_EPOCH) return 0;

  if (!ntpReady) {
    ntpReady = true;
    Serial.println("[NTP] synchronized");
  }
  return static_cast<uint64_t>(now.tv_sec) * 1000ULL +
         static_cast<uint64_t>(now.tv_usec) / 1000ULL;
}

bool validSample(uint16_t co2, float temp, float humidity) {
  return co2 > 0 && co2 <= 10000 && isfinite(temp) && temp >= -40.0f && temp <= 85.0f &&
         isfinite(humidity) && humidity >= 0.0f && humidity <= 100.0f;
}

void pushSample(uint16_t co2, float temp, float humidity) {
  const uint64_t timestamp = getTimestamp();
  if (!timestamp) {
    Serial.println("[QUEUE] skipped: waiting for NTP");
    return;
  }

  if (bufCount == SAMPLE_CAP) {
    bufHead = (bufHead + 1) % SAMPLE_CAP;
    bufCount -= 1;
    Serial.println("[QUEUE] full: dropped oldest sample");
  }

  const uint16_t index = (bufHead + bufCount) % SAMPLE_CAP;
  sampleBuf[index] = {
      timestamp / 1000ULL,
      timestamp,
      co2,
      temp,
      humidity,
      screenOn,
  };
  bufCount += 1;
}

void dropSample() {
  if (!bufCount) return;
  bufHead = (bufHead + 1) % SAMPLE_CAP;
  bufCount -= 1;
}

void postSample() {
  if (WiFi.status() != WL_CONNECTED || !bufCount || millis() - lastPostAt < postWait) {
    return;
  }

  lastPostAt = millis();
  const AirSample& sample = sampleBuf[bufHead];

  char payload[320];
  snprintf(
      payload,
      sizeof(payload),
      "{\"deviceId\":\"%s\",\"seq\":%llu,\"co2\":%u,\"temp\":%.2f,"
      "\"humidity\":%.2f,\"sensor\":\"SCD40\",\"fw\":\"%s\","
      "\"timestamp\":%llu,\"screenOn\":%s}",
      cfg.deviceId.c_str(),
      static_cast<unsigned long long>(sample.seq),
      sample.co2,
      sample.temp,
      sample.humidity,
      FW_VERSION,
      static_cast<unsigned long long>(sample.timestamp),
      sample.screenOn ? "true" : "false");

  HTTPClient http;
  http.setTimeout(HTTP_TIMEOUT_MS);
  if (!http.begin(cfg.serverUrl.c_str())) {
    Serial.println("[HTTP] failed to initialize request");
    postWait = POST_RETRY_MS;
    return;
  }
  http.addHeader("Content-Type", "application/json");

  const int status = http.POST(reinterpret_cast<uint8_t*>(payload), strlen(payload));
  const String response = status > 0 ? http.getString() : HTTPClient::errorToString(status);
  Serial.printf(
      "[HTTP] status=%d queued=%u response=%s\n",
      status,
      static_cast<unsigned int>(bufCount),
      response.c_str());
  drawNetState(status > 0 ? NET_ONLINE : NET_ERROR, status);
  http.end();

  if (status == 200 || status == 202 || status == 400) {
    dropSample();
    postWait = POST_GAP_MS;
  } else {
    postWait = POST_RETRY_MS;
  }
}

void mqttSendDiscovery() {
  // MQTT Discovery：让 Home Assistant 自动建立实体，无需手动配置 sensor。
  // 唯一 id 在 topic 与配置的 unique_id 中保持一致。
  char mac[32];
  snprintf(mac, sizeof(mac), "%02X%02X%02X%02X%02X%02X", WiFi.macAddress()[0],
           WiFi.macAddress()[1], WiFi.macAddress()[2], WiFi.macAddress()[3],
           WiFi.macAddress()[4], WiFi.macAddress()[5]);

  // 设备识别唯一标识（用于 HA 中把多个实体归到同一设备）
  char base[96];
  snprintf(base, sizeof(base), "airco2_%s_%s", cfg.deviceId.c_str(), mac);
  for (char* p = base; *p; ++p) {
    if (!((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||
          (*p >= '0' && *p <= '9') || *p == '-' || *p == '_')) {
      *p = '_';
    }
  }

  struct DiscoveryConfig {
    const char* id;
    const char* name;
    const char* deviceClass;
    const char* unit;
    const char* valueTopic;
    const char* stateClass;
  };

  const DiscoveryConfig sensorList[] = {
      {"co2", "CO2", "carbon_dioxide", "ppm", "co2/state", "measurement"},
      {"temp", "Temperature", "temperature", "\u00b0C", "temp/state", "measurement"},
      {"humidity", "Humidity", "humidity", "%", "humidity/state", "measurement"},
  };

  for (const DiscoveryConfig& c : sensorList) {
    char topic[160];
    char payload[640];
    const String unique = String(base) + "_" + c.id;
    const String valueTopic = String(MQTT_TOPIC_ROOT) + "/" + cfg.deviceId + "/" + c.valueTopic;

    snprintf(topic, sizeof(topic), "%s/%s/%s/%s/config", MQTT_DISCOVERY_ROOT, c.id, base, c.id);
    snprintf(payload, sizeof(payload),
             "{\"name\":\"%s %s\",\"unique_id\":\"%s\",\"device_class\":\"%s\","
             "\"unit_of_measurement\":\"%s\",\"state_topic\":\"%s\",\"state_class\":\"%s\","
             "\"device\":{\"identifiers\":[\"%s\"],\"name\":\"Air CO2 Monitor\","
             "\"manufacturer\":\"DIY\",\"model\":\"ESP32-S3 SCD40\",\"sw_version\":\"%s\"}}",
             cfg.deviceId.c_str(), c.name, unique.c_str(), c.deviceClass, c.unit,
             valueTopic.c_str(), c.stateClass, base, FW_VERSION);
    mqtt.publish(topic, payload, true);
  }

  // 屏幕开关（可在 HA / HA App 里开关）
  {
    const char* cid = "screen";
    char topic[160];
    char payload[512];
    const String unique = String(base) + "_" + cid;
    const String stateTopic =
        String(MQTT_TOPIC_ROOT) + "/" + cfg.deviceId + "/" + cid + "/state";
    const String cmdTopic = String(MQTT_TOPIC_ROOT) + "/" + cfg.deviceId + "/" + cid + "/set";

    snprintf(topic, sizeof(topic), "%s/%s/%s/config", MQTT_DISCOVERY_SWITCH_ROOT, base, cid);
    snprintf(payload, sizeof(payload),
             "{\"name\":\"%s Screen\",\"unique_id\":\"%s\",\"state_topic\":\"%s\","
             "\"command_topic\":\"%s\",\"payload_on\":\"ON\",\"payload_off\":\"OFF\","
             "\"state_on\":\"ON\",\"state_off\":\"OFF\","
             "\"device\":{\"identifiers\":[\"%s\"],\"name\":\"Air CO2 Monitor\","
             "\"manufacturer\":\"DIY\",\"model\":\"ESP32-S3 SCD40\",\"sw_version\":\"%s\"}}",
             cfg.deviceId.c_str(), unique.c_str(), stateTopic.c_str(), cmdTopic.c_str(), base,
             FW_VERSION);
    mqtt.publish(topic, payload, true);
  }

  mqttDiscoverySent = true;
}

void publishScreenState() {
  if (!mqttReady || !mqtt.connected()) return;
  if (millis() - lastScreenPubAt < MQTT_SCREEN_OFF_MS) return;
  lastScreenPubAt = millis();
  const String stateTopic = String(MQTT_TOPIC_ROOT) + "/" + cfg.deviceId + "/screen/state";
  mqtt.publish(stateTopic.c_str(), screenOn ? "ON" : "OFF", true);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char msg[16];
  if (length >= sizeof(msg)) length = sizeof(msg) - 1;
  memcpy(msg, payload, length);
  msg[length] = '\0';

  const String cmdTopic = String(MQTT_TOPIC_ROOT) + "/" + cfg.deviceId + "/screen/set";
  if (String(topic) != cmdTopic) return;

  if (strcmp(msg, "ON") == 0) {
    setScreen(true);
  } else if (strcmp(msg, "OFF") == 0) {
    setScreen(false);
  }
}

void mqttReconnect() {
  if (!mqttEnabled || WiFi.status() != WL_CONNECTED) return;
  if (mqtt.connected()) return;

  mqtt.setServer(cfg.mqttHost.c_str(), cfg.mqttPort);
  mqtt.setCallback(mqttCallback);
  String clientId = String("airco2-") + cfg.deviceId;
  bool ok = false;
  if (cfg.mqttUser.length() > 0) {
    ok = mqtt.connect(clientId.c_str(), cfg.mqttUser.c_str(), cfg.mqttPass.c_str());
  } else {
    ok = mqtt.connect(clientId.c_str());
  }

  if (ok) {
    mqttReady = true;
    const String cmdTopic = String(MQTT_TOPIC_ROOT) + "/" + cfg.deviceId + "/screen/set";
    mqtt.subscribe(cmdTopic.c_str());
    Serial.println("[MQTT] connected");
    if (!mqttDiscoverySent) mqttSendDiscovery();
    publishScreenState();
  } else {
    mqttReady = false;
    Serial.printf("[MQTT] connect failed rc=%d\n", mqtt.state());
  }
}

void mqttPublishSample(uint16_t co2, float temp, float humidity) {
  if (!mqttReady || !mqtt.connected()) return;
  const String root = String(MQTT_TOPIC_ROOT) + "/" + cfg.deviceId + "/";
  char buf[32];
  snprintf(buf, sizeof(buf), "%d", co2);
  mqtt.publish((root + "co2/state").c_str(), buf);
  snprintf(buf, sizeof(buf), "%.2f", temp);
  mqtt.publish((root + "temp/state").c_str(), buf);
  snprintf(buf, sizeof(buf), "%.2f", humidity);
  mqtt.publish((root + "humidity/state").c_str(), buf);
}

void updateMqtt() {
  if (!mqttEnabled) return;
  if (!mqtt.connected()) {
    if (WiFi.status() == WL_CONNECTED && millis() - lastMqttAt >= MQTT_RETRY_MS) {
      lastMqttAt = millis();
      mqttReconnect();
    }
    return;
  }
  mqtt.loop();
  publishScreenState();
}

void sensorError() {
  if (sensorErrs < MAX_SENSOR_ERR) sensorErrs += 1;
  if (sensorErrs < MAX_SENSOR_ERR) return;

  sensorReady = false;
  lastSensorAt = millis();
  Serial.println("[SCD40] too many read errors; scheduling reinitialization");
}

void retrySensor() {
  if (sensorReady || millis() - lastSensorAt < SENSOR_RETRY_MS) return;

  lastSensorAt = millis();
  Serial.println("[SCD40] retrying initialization");
  sensorReady = initSensor();
  if (sensorReady) {
    drawSensorState("SCD40 OK", COLOR_GREEN);
  } else {
    drawSensorState("SENSOR ERR", COLOR_RED);
    drawAirLevel(AIR_UNKNOWN);
  }
}

void readSensor() {
  bool dataReady = false;
  int16_t error = sensor.getDataReadyStatus(dataReady);
  if (error != NO_ERROR) {
    printError("getDataReadyStatus", error);
    drawSensorState("SENSOR ERR", COLOR_RED);
    drawAirLevel(AIR_UNKNOWN);
    sensorError();
    return;
  }
  if (!dataReady) return;

  uint16_t co2 = 0;
  float temp = 0.0f;
  float humidity = 0.0f;
  error = sensor.readMeasurement(co2, temp, humidity);
  if (error != NO_ERROR) {
    printError("readMeasurement", error);
    drawSensorState("SENSOR ERR", COLOR_RED);
    drawAirLevel(AIR_UNKNOWN);
    sensorError();
    return;
  }
  if (!validSample(co2, temp, humidity)) {
    Serial.printf("[SCD40] invalid sample: CO2=%u temp=%.2f humidity=%.2f\n", co2, temp, humidity);
    drawSensorState("SENSOR ERR", COLOR_RED);
    drawAirLevel(AIR_UNKNOWN);
    return;
  }

  Serial.printf("[SCD40] CO2=%u ppm temp=%.2f C humidity=%.2f %%\n", co2, temp, humidity);
  sensorErrs = 0;
  drawSensorState("SCD40 OK", COLOR_GREEN);
  drawAirSample(co2, temp, humidity);
  pushSample(co2, temp, humidity);
  if (mqttEnabled) mqttPublishSample(co2, temp, humidity);
}

// 判断是否需要进入配置门户（SoftAP + captive portal）。
// 触发条件：
//   1) 首次使用：NVS 中还没有有效配置，SSID 仍是 config.example.h 默认占位符；
//   2) 手动触发：开机时按住 PIN_CFG_BUTTON（默认 GPIO0 / BOOT 键）不放。
bool shouldEnterConfigPortal() {
  const bool unconfigured =
      cfg.wifiSsid.length() == 0 || cfg.wifiSsid.equals(WIFI_SSID_DEFAULT);
  if (unconfigured) {
    Serial.println("[CFG] no saved config (SSID is still the default placeholder)");
    return true;
  }

  if (PIN_CFG_BUTTON < 0) return false;
  pinMode(PIN_CFG_BUTTON, INPUT_PULLUP);
  delay(50);  // 等待引脚电平稳定
  if (digitalRead(PIN_CFG_BUTTON) == LOW) {
    Serial.println("[CFG] config button pressed at boot");
    return true;
  }
  return false;
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n[AirCO2] firmware 0.1.1-mqtt starting");

  // 1) 加载运行时配置：NVS 优先，未配置则回退到 config.example.h 编译期默认值。
  AppConfig defaults;
  loadDefaults(defaults);
  configManager.begin(defaults);
  configManager.load(cfg);
  mqttEnabled = cfg.mqttHost.length() > 0;
  Serial.printf("[CFG] wifi=%s server=%s device=%s mqtt=%s\n",
                cfg.wifiSsid.c_str(), cfg.serverUrl.c_str(), cfg.deviceId.c_str(),
                mqttEnabled ? cfg.mqttHost.c_str() : "(disabled)");

  // 2) 屏幕初始状态：默认取配置 screenDefault，若 NVS 有上次运行状态则优先。
  screenOn = cfg.screenDefault;
  prefsReady = prefs.begin(kConfigNamespace, false);
  if (prefsReady && prefs.isKey("screen")) {
    screenOn = prefs.getBool("screen", screenOn);
  }
  lcdInit(screenOn);
  drawAirScreen();
  drawSensorState("START", COLOR_YELLOW);

  // 3) 进入配置门户（首次未配置 / 开机长按配置按钮），可运行时修改
  //    Wi-Fi/服务器/MQTT/屏幕默认设置，保存到 NVS 后重启。
  if (shouldEnterConfigPortal()) {
    Serial.println("[CFG] opening config portal (SoftAP) ...");
    drawNetState(NET_CONFIG);
    if (runConfigPortal(cfg, "AirCO2")) {
      if (!configManager.save(cfg)) {
        Serial.println("[CFG] failed to save configuration to NVS");
      }
      Serial.println("[CFG] configuration saved; rebooting...");
      delay(500);
      ESP.restart();
    }
    Serial.println("[CFG] portal exited without saving; continuing with current config");
  }

  // 4) 传感器 & 网络
  sensorReady = initSensor();
  if (!sensorReady) {
    lastSensorAt = millis();
    Serial.println("[SCD40] initialization failed; restart after checking wiring");
    drawSensorState("SENSOR ERR", COLOR_RED);
    drawAirLevel(AIR_UNKNOWN);
  } else {
    drawSensorState("SCD40 OK", COLOR_GREEN);
  }
  screenUrl = makeScreenUrl();
  connectWifi();
}

void loop() {
  updateFactoryResetButton();  // 长按 BOOT 5 秒 → 清空 NVS 并重启进配置门户

  // 联网配置服务：用户在 http://<设备IP>/ 点了【保存并重启】→ 写 NVS 并重启
  if (CONFIG_SERVER_ENABLED && handleConfigServer()) {
    Serial.println("[CFG] configuration saved via web; rebooting...");
    if (!configManager.save(cfg)) {
      Serial.println("[CFG] failed to save configuration to NVS");
    }
    delay(500);
    ESP.restart();
  }

  updateWifi();
  if (mqttEnabled) updateMqtt();
  syncScreen();
  retrySensor();
  postSample();
  if (!sensorReady || millis() - lastReadAt < READ_MS) {
    delay(10);
    return;
  }

  lastReadAt = millis();
  readSensor();
}
