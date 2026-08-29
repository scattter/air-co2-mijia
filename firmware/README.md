# firmware — 带 MQTT/Home Assistant 上报的固件副本

本目录是原 `air-co2`（Waveshare ESP32-S3-LCD-1.47B + SCD40）固件的**独立副本**，在原逻辑不变的基础上增量增加 **MQTT 上报**，让数据能进入 Home Assistant（经 Mosquitto broker）并与米家设备联动。

**原项目 `air-co2` 保持不动**，本副本单独维护。

## 与原文固件差异

- 增加 MQTT 依赖：`PubSubClient`（实现 MQTT 上报 + 屏幕开关下行控制）
- 每次有效读数后，额外把 `co2 / temp / humidity` 发布到 MQTT
- 屏幕开关可通过 HA 控制（值变化时发布状态）
- 使用 Home Assistant **MQTT Discovery** 自动建实体，无需手工配置 sensor
- 配置改为 **NVS 运行时配置**（`ConfigManager` + 配置门户），运行时可改连接，无需重烧

## 配置

固件使用 **NVS 运行时配置**（`ConfigManager` + 配置门户），不再依赖编译期宏：

- 首次烧录时，默认值取自 `config.example.h`（`WIFI_SSID_DEFAULT` 等占位符）。
- 真正生效的连接配置保存在 ESP32 的 **NVS** 中，运行时可通过**配置门户网页**修改，
  无需重新烧录即可改 Wi-Fi / 服务器 / MQTT / 屏幕默认设置。

### 如何进入配置门户（SoftAP + captive portal）

设备开机时会自动判断，命中任一条件即进入配置模式，开启名为 `AirCO2-<后4位>` 的
热点（密码 `airco212345`），手机连接该热点会自动弹出配置页（或访问 `http://192.168.4.1`）：

1. **首次使用 / 未配置**：NVS 中还没有保存过配置（SSID 仍是默认占位符）时自动进入；
2. **手动触发**：开机时**按住配置按钮**（默认 `PIN_CFG_BUTTON` = GPIO0，即开发板 BOOT 键）；
3. **恢复出厂设置（正常运行中）**：设备正常运行时**长按 BOOT 键 5 秒** → 清空 NVS 并重启，
   重启后因 NVS 为空自动进入配置门户。中途松开即取消（按住 3 秒起串口会倒计时提示）。

在网页里填写 Wi-Fi、服务器地址（`SERVER_URL`）、设备 ID，以及可选的 MQTT 配置后
点【保存并重启】，配置写入 NVS 并重启上线。

### 联网后重新配置（无需按 BOOT 键）

设备连接 Wi-Fi 成功后，会自动在 80 端口开启**联网配置服务**（与配置门户同一个
网页、同一套字段）：

- 浏览器打开 `http://<设备IP>/` 即可重新配置，改完点【保存并重启】写入 NVS 并重启；
- 设备 IP 可从路由器 DHCP 客户端列表查看，或看串口日志
  （开机时会打印 `[CFG] open http://<IP>/ to reconfigure`）；
- 页面无登录保护，局域网内均可访问；如不需要可把 `board_config.h` 里的
  `CONFIG_SERVER_ENABLED` 改为 `false` 关闭；
- 首次烧录/未配置、或想用 AP 方式配置时，原有入口仍然有效（开机自动进入或长按 BOOT）。

> 提示：`config.local.h` **不存在也不需要**（代码里没有 include 它的机制）。
> 如需改变编译期回退默认值（NVS 为空时使用的值），直接编辑 `config.example.h` 里
> 的 `*_DEFAULT` 宏即可（如把 `WIFI_SSID_DEFAULT` 改成你常用的 Wi-Fi，可省去首次配置）。

### 配置项一览（`AppConfig`）

| 字段 | 网页字段 | 说明 |
|---|---|---|
| `wifiSsid` / `wifiPass` | ssid / pass | Wi-Fi 连接 |
| `serverUrl` | url | HTTP 上报地址（`/api/air/sample`） |
| `deviceId` | devid | 设备唯一 ID |
| `mqttHost` | mqhost | MQTT broker 地址，**留空则禁用 MQTT** |
| `mqttPort` | mqport | MQTT 端口（默认 1883） |
| `mqttUser` / `mqttPass` | mquser / mqpass | MQTT 认证（可选） |
| `screenDefault` | screen | 默认屏幕开关 |

## Arduino 依赖

```bash
arduino-cli lib install "Sensirion I2C SCD4X"
arduino-cli lib install "PubSubClient"
```

## 编译

```bash
arduino-cli compile \
  --fqbn 'esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi' \
  .
```

## MQTT 主题

| 主题 | 方向 | 说明 |
|---|---|---|
| `home/airco2/<DEVICE_ID>/co2/state` | 上报 | CO2 ppm |
| `home/airco2/<DEVICE_ID>/temp/state` | 上报 | 温度 ℃ |
| `home/airco2/<DEVICE_ID>/humidity/state` | 上报 | 湿度 % |
| `home/airco2/<DEVICE_ID>/screen/state` | 上报 | `ON` / `OFF`（retain） |
| `home/airco2/<DEVICE_ID>/screen/set` | 下行 | 订阅 `ON` / `OFF` 控制屏幕 |

Discovery 主题：`homeassistant/sensor/...` 与 `homeassistant/switch/...`，固件连接 broker 后自动发布。
