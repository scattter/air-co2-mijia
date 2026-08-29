#pragma once

// 配置门户：开机进入配置模式时，ESP32 开启 SoftAP，并提供一个网页
// （captive portal），用户可在此修改连接配置。保存后回写 NVS 并重启。
//
// 设计要点：
//  - 不引入 ESPAsyncWebServer，使用 ESP32 Arduino 内置的 WebServer + DNSServer，
//    零额外依赖。
//  - 提供 / 表单页 与 /save POST；/generate_204 等常见检测路径重定向到 / 以触发
//    手机端"自动弹出登录页"。

#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>

#include "config.h"

struct PortalResult {
  bool saved;
};

// 启动配置门户（阻塞式）。cfg 传入当前配置作为表单默认值；
// 用户点保存后更新 cfg、写 NVS，返回。调用方随后重启设备。
bool runConfigPortal(AppConfig& cfg, const char* apSsidPrefix);

// ── 联网模式配置服务（STA）──────────────────────────────
// WiFi 连接成功后，开发板在 80 端口提供与配置门户完全相同的网页，
// 访问 http://<设备IP>/ 即可随时重新配置，无需按 BOOT 键进入 AP 门户。
//  - beginConfigServer(): 启动服务（可重复调用，内部会先停掉旧实例，
//    适合 WiFi 重连/换 IP 后再次调用）。
//  - handleConfigServer(): 在 loop() 中每次调用以处理请求；返回 true
//    表示用户点了【保存并重启】（cfg 已被更新），调用方应写 NVS 并重启。
bool beginConfigServer(AppConfig& cfg);
bool handleConfigServer();
