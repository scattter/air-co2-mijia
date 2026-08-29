# 验证与排错

## 1. 先确认 broker 正常

```bash
# 在 NAS 上
curl -s http://<NAS IP>:1883 -o /dev/null -w "%{http_code}\n"   # 连接应返回（TCP 层面）
docker compose -f deploy/docker-compose.yml logs mosquitto | tail
```

用 MQTT 客户端订阅测试（可选）：
```bash
mosquitto_sub -h <NAS IP> -t 'home/airco2/#' -v
```

## 2. 确认固件已连接并上报

- 固件串口日志应出现 `[MQTT] connected`。
- 若无，检查：是否在**配置门户**里填了 MQTT 服务器（`mqhost`）、端口、用户名密码是否匹配、ESP 与 NAS 同网段。未配置 MQTT 时 `mqttHost` 留空则不启用 MQTT。

订阅确认数值：
```bash
mosquitto_sub -h <NAS IP> -t 'home/airco2/lcd147-air-01/co2/state' -v
```

## 3. 确认 HA 里出现实体

- HA：`设置 > 设备与服务 > MQTT`，应能看到自动发现的 4 个实体（CO2 / Temperature / Humidity / Screen）。
- CO2 上报频率约固件每读一帧发布一次（默认 250ms 轮询 + 传感器就绪周期，实际几分钟一条即可）。
- 若实体没出现：检查 `mqtt:` 段的 `discovery: true`，重启 HA，确认 broker 可连（`设置 > 设备与服务 > MQTT` 是否显示已连接）。

## 4. 米家设备联动不生效

- 确认 `ha_xiaomi_home` 已把净化器导入（`设备与服务 > Xiaomi Home` 下有该设备）。
- 确认自动化里实体 id 正确（在 `设置 > 设备与服务 > 实体` 搜索）。
- 用 HA 的"运行操作"测试 `fan.turn_on` 能否控制净化器；若走云端指令，确认账号登录态正常。

## 5. 屏幕开关控制无反应 / 状态不更新

- 屏幕开关走 MQTT + 原有 NAS 轮询两套机制，互不覆盖：MQTT 下行会直接改设备；NAS `actual` 上报仍同步。
- MQTT 开关回显有 15s 防抖（`MQTT_SCREEN_OFF_MS`），故状态变化最多延迟 15s 更新。

## 6. 常见问题定位

| 现象 | 排查 |
|---|---|
| HA 里 MQTT 显示未连接 | broker IP/端口；mosquitto 容器是否起来（`docker compose ps`）|
| 固件 MQTT connect rc != 0 | rc 含义见 PubSubClient：1=协议不接受(常见为用户名密码)、2=标识符冲突、5=未授权、0 之外的数字看文档 |
| 实体出现但数值 NaN/不更新 | 确认发布主题里的 DEVICE_ID 与 discovery 一致；`mqttPublishSample` 在有效读数后才发 |
| 自动化不触发 | `for:` 持续时间过短/长；阈值方向（above/below）；实体 id 变了（改过 unique_id 会使旧实体失效）|
