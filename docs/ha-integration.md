# 接入米家生态：Home Assistant + ha_xiaomi_home

本文说明如何把设备进入 HA 后，与你的米家设备在同生态内联动，以及在 HA App 里远程查看/控制。

## 重要前提（务必先读）

- **ha_xiaomi_home 是单向集成**：它把**米家设备导入 Home Assistant**，用来在 HA 里读取/控制你的米家设备。
- **它不支持**把 HA 里的 DIY 第三方实体（如你的 CO2 传感器）**反向推送进米家 App**。
- **米家 App 只显示经过小米/米家认证（入网 MIoT）的设备**，个人 DIY 硬件无官方渠道进入米家 App。

因此本项目定位为：**用 HA 作为"汇合点"，把你的 DIY 传感器与米家设备放在同一个自动化体系里联动**，
并通过 **HA App（Android/iOS）** 远程查看与控制。这覆盖了"接入智能家居生态、联动米家净化器"的绝大部分实际诉求。

## 安装 ha_xiaomi_home（官方集成）

方式 A — HACS（推荐）：

1. 在 HA 安装 [HACS](https://hacs.xyz/)（前置）。
2. HACS → 集成 → 搜索 `Xiaomi Home` → 下载，重启 HA。

方式 B — git clone + install.sh：

```bash
cd /path/to/config            # 你 HA 的 config 目录（本项目 deploy/homeassistant）
cd config
git clone https://github.com/XiaoMi/ha_xiaomi_home.git
cd ha_xiaomi_home
./install.sh /config
```

## 配置

1. HA：`设置 > 设备与服务 > 添加集成 > Xiaomi Home`
2. 选择你的**米家账号地区**（中国大陆 / Europe / India / Russia / Singapore / United States）——你的账号在哪个机房就选哪个。
3. 授权登录小米账号。
4. 导入后，你的米家设备会出现在 HA 的实体里（如 `fan.xxx` 净化器、`light.xxx` 等）。

> 局域网本地化控制：若局域网内有[小米中枢网关](https://www.mi.com/shop/buy/detail?product_id=15755&cfrom=search)或支持中枢网关的设备，
> ha_xiaomi_home 可走本地化控制（不依赖云端）。否则指令经小米云下发。本项目 compose 已用 host 网络以利发现。

## 联动示例（CO2 超标 → 开净化器）

见 [`../deploy/automations/automations.yaml`](../deploy/automations/automations.yaml)。
把里面的净化器实体 id 换成你实际导入后的 id（在 HA：`设置 > 设备与服务 > 实体` 搜索找）。

## 用 HA App 远程查看控制本设备

1. 手机安装 **Home Assistant** App（官方 Companion App）：
   - Android：Google Play 搜 "Home Assistant"，或用 F-Droid；
     国内手机（如 Realme/ColorOS）没有谷歌服务时，直接装 GitHub APK：
     https://github.com/home-assistant/android/releases
   - iOS：App Store 搜 "Home Assistant"。
2. App 打开 → 填 HA 地址 `http://<NAS IP>:8123`（同一局域网）→ 登录 HA 账号。
3. 将本设备的 CO2/温湿度/屏幕开关实体加到首页仪表盘。
4. 即可在手机/电脑上实时查看浓度、远程开关屏幕，并能在自动化里引用。
5. 远程（不在家）访问：局域网内免配置；跨网络需 Nabu Casa 云、端口映射+HTTPS 或
   Tailscale 等组网。Android 通知默认走 Google FCM（大陆网络常收不到），
   新版 App + HA 2024.4+ 支持本地推送（Local Push），同局域网可稳定接收。

## 如果你仍希望"在米家 App 里看到"

官方途径不可行。可参考的非官方替代（**有封号/违背 ToS 风险，不推荐**）：
MiService 或自定义网关伪造设备 token —— 不稳定且可能影响小米账号。

## 参考

- 官方集成源码：https://github.com/XiaoMi/ha_xiaomi_home
- 官方中文文档：https://github.com/XiaoMi/ha_xiaomi_home/blob/main/doc/README_zh.md
