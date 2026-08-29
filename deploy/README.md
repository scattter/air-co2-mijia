# deploy — Home Assistant + Mosquitto 一键部署

为 `air-co2` 设备提供 Home Assistant 与 MQTT broker，作为其进入 HA 生态、联动米家设备的桥接层。

## 部署

在 NAS 上（请在能联网的机器执行拉镜像）：

```bash
cd deploy
docker compose up -d
docker compose ps
```

- **Home Assistant**：`http://<NAS IP>:8123`
- **Mosquitto**：`1883`

## 完成后要做的事

1. **首次进入 HA**：打开 `:8123`，按向导创建账号。
2. **HA 加 MQTT 集成**：若 `configuration.yaml` 已写好 `mqtt:` 块，重启 HA 后会自动连上 broker；
   也可在 `设置 > 设备与服务 > 添加集成 > MQTT` 手动配置。
3. **烧写固件**（启用 MQTT）见 [`../firmware/`](../firmware/)。
4. **接入米家设备**（可选，用于联动）见 [`../docs/ha-integration.md`](../docs/ha-integration.md)。
5. **添加自动化** 把 [`automations/automations.yaml`](./automations/automations.yaml) 内容贴入
   HA 的 `设置 > 自动化与场景 > 创建自动化 > 编辑为 YAML`，并按注释修改净化器实体 id。

## 目录

```
deploy/
├── docker-compose.yml          # homeassistant + mosquitto
├── homeassistant/
│   ├── configuration.yaml      # HA 主配置（已含 mqtt 集成）
│   └── scripts.yaml
├── mosquitto/config/
│   └── mosquitto.conf          # broker 配置
└── automations/
    └── automations.yaml        # CO2 联动米家净化器 示例
```

## 安全提示

- 默认 broker 允许局域网匿名连接（`allow_anonymous true`）。建议在路由器固定内网后再启用
  mosquitto 密码认证（见 mosquitto.conf 注释）。
- Home Assistant 使用 `privileged` + host 网络以便本地化控制；若不接中枢网关/需要 USB，可精简。
