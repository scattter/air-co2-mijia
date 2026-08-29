# air-co2-mijia

让现有 **`air-co2`**（ESP32-S3 + SCD40 空气监测仪，数据上报到 NAS）融入 **Home Assistant**，进而在 HA 里与**米家生态**联动的独立项目。

> 本项目**不修改**原 `air-co2` 工程。它提供：独立固件副本（在原固件基础上增量增加 MQTT 上报）、Home Assistant + Mosquitto 的一键 Docker 部署、以及 CO2 超标联动米家设备（如空气净化器）的自动化示例。

## 为什么需要它（重要认知）

你最初的诉求是"把设备接进米家 App"。经过核实：

- 小米官方的 **[ha_xiaomi_home](https://github.com/XiaoMi/ha_xiaomi_home)** 集成是**单向**的：它把**米家设备导入 Home Assistant**，**不支持**把 HA 里的 DIY 第三方实体反向推送到米家 App。
- **米家 App 只显示经过小米/米家认证（入网 MIoT）的设备**。个人 DIY 硬件没有任何官方渠道能作为独立设备出现在米家 App 里。
- 因此可行的、官方稳定路径是：**设备进 HA，在 HA 里与你的米家设备联动，通过 HA App 远程查看/控制**。HA 与米家设备的双向控制由官方 ha_xiaomi_home 提供。

正是这一点决定了本项目设计成为"HA 桥接"而非"进米家 App"。

## 架构

```
ESP32 (SCD40)
   ├── HTTP POST → NAS Fastify (SQLite + 原网页)   [原项目, 保留]
   └── MQTT ──► Mosquitto broker ──► Home Assistant 实体 (自动发现)
                                           │
                                           ▼  ha_xiaomi_home 集成
                                    读取/控制你的米家设备（净化器等）
                                           │
                                           ▼
                         HA 自动化：CO2 超标 → 打开米家空气净化器
                           HA App 远程查看/控制本设备
```

## 使用步骤（概览）

1. **部署 Home Assistant + Mosquitto**：见 [`deploy/`](./deploy/)
2. **烧录固件**（NVS 运行时配置 + HTTP/MQTT 上报）：见 [`firmware/`](./firmware/)，首次开机自动进入**配置门户**（或长按 BOOT 键），在网页里填写 Wi-Fi / 服务器 / MQTT 即可，无需改代码重烧
3. **HA 配置米家集成**（hap `ha_xiaomi_home`）：见 [`docs/ha-integration.md`](./docs/ha-integration.md)
4. **添加联动自动化**：见 [`deploy/automations/`](./deploy/automations/)
5. **验证**：见 [`docs/troubleshooting.md`](./docs/troubleshooting.md)

## 目录结构

```
air-co2-mijia/
├── deploy/                 # Docker 部署
│   ├── docker-compose.yml  # homeassistant + mosquitto
│   ├── homeassistant/      # HA 配置
│   ├── mosquitto/          # broker 配置
│   └── automations/        # HA 自动化示例（含米家联动）
├── firmware/               # 独立固件副本（原 air-co2 + MQTT）
├── docs/                   # 说明文档
└── README.md
```
