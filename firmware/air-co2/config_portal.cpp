#include "config_portal.h"

namespace {

char apSsid[40];

WebServer server(80);
DNSServer dns;
bool portalDone = false;
AppConfig* outCfg = nullptr;

void sendHtml(const char* html) { server.sendContent(html, strlen(html)); }

void renderForm() {
  // 转义，防注入（简单替代必转义字符）。
  auto esc = [](const String& s) {
    String r;
    for (unsigned i = 0; i < s.length(); ++i) {
      char c = s[i];
      switch (c) {
        case '&': r += "&amp;"; break;
        case '<': r += "&lt;"; break;
        case '>': r += "&gt;"; break;
        case '"': r += "&quot;"; break;
        case '\'': r += "&#39;"; break;
        default: r += c;
      }
    }
    return r;
  };

  String page = String();
  page.reserve(3800);

  page += F(
      "<!DOCTYPE html><html lang=\"zh-CN\"><head><meta charset=\"utf-8\">"
      "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
      "<title>Air CO2 配置</title><style>"
      "body{font-family:-apple-system,sans-serif;max-width:520px;margin:20px auto;padding:0 12px;color:#222}"
      "h1{font-size:20px}.f{margin:14px 0}.f label{display:block;margin-bottom:4px;font-size:14px;color:#555}"
      "input{width:100%;box-sizing:border-box;padding:9px;border:1px solid #ccc;border-radius:6px;font-size:15px}"
      ".sec{margin:20px 0 6px;font-weight:600;color:#333;border-bottom:1px solid #eee;padding-bottom:4px}"
      "button{width:100%;padding:12px;background:#0d6efd;color:#fff;border:0;border-radius:6px;font-size:16px;margin-top:8px}"
      ".hint{font-size:12px;color:#888;margin-top:3px}</style></head><body>");

  page += F("<h1>Air CO2 设备配置</h1><form method=\"POST\" action=\"/save\">");
  // 注：含运行时拼接（esc()/String()）的语句不能包在 F() 里（F 会把整段当闪存常量），
  // 转为普通 RAM 字符串，安全且不影响功能。
  page +=
      "<div class=\"sec\">无线网络 (Wi-Fi)</div>"
      "<div class=\"f\"><label>Wi-Fi 名称 (SSID)</label><input name=\"ssid\" "
      "value=\"" + esc(outCfg->wifiSsid) + "\" required></div>"
      "<div class=\"f\"><label>Wi-Fi 密码</label><input type=\"password\" name=\"pass\" "
      "value=\"" + esc(outCfg->wifiPass) + "\"></div>";

  page +=
      "<div class=\"sec\">数据服务器</div>"
      "<div class=\"f\"><label>服务器地址 (SERVER_URL)</label><input name=\"url\" "
      "value=\"" + esc(outCfg->serverUrl) + "\"><div class=\"hint\">例：http://192.168.0.109:7311/api/air/sample</div></div>"
      "<div class=\"f\"><label>设备 ID</label><input name=\"devid\" "
      "value=\"" + esc(outCfg->deviceId) + "\"></div>";

  page +=
      "<div class=\"sec\">Home Assistant (MQTT，可选)</div>"
      "<div class=\"f\"><label>MQTT 服务器地址 (留空禁用)</label><input name=\"mqhost\" "
      "value=\"" + esc(outCfg->mqttHost) + "\"></div>"
      "<div class=\"f\"><label>MQTT 端口</label><input type=\"number\" name=\"mqport\" "
      "value=\"" + String(outCfg->mqttPort) + "\"></div>"
      "<div class=\"f\"><label>MQTT 用户名</label><input name=\"mquser\" "
      "value=\"" + esc(outCfg->mqttUser) + "\"></div>"
      "<div class=\"f\"><label>MQTT 密码</label><input type=\"password\" name=\"mqpass\" "
      "value=\"" + esc(outCfg->mqttPass) + "\"></div>";

  page +=
      "<div class=\"sec\">其它</div>"
      "<div class=\"f\"><label>默认屏幕开关</label><select name=\"screen\">"
      "<option value=\"1\"" + (outCfg->screenDefault ? String(" selected") : String("")) + ">开</option>"
      "<option value=\"0\"" + (outCfg->screenDefault ? String("") : String(" selected")) + ">关</option>"
      "</select></div>";

  page += F("<button type=\"submit\">保存并重启</button></form>"
            "<p class=\"hint\">保存后设备将重启，并使用新配置连接。</p></body></html>");

  server.send(200, "text/html; charset=utf-8", page);
}

void handleRoot() { renderForm(); }

void handleSave() {
  if (server.method() != HTTP_POST) {
    server.send(400, "text/plain", "Method Not Allowed");
    return;
  }

  AppConfig& c = *outCfg;
  c.wifiSsid = server.arg("ssid");
  c.wifiPass = server.arg("pass");
  if (server.hasArg("url")) c.serverUrl = server.arg("url");
  if (server.hasArg("devid")) c.deviceId = server.arg("devid");
  c.mqttHost = server.arg("mqhost");
  c.mqttPort = (uint16_t)server.arg("mqport").toInt();
  c.mqttUser = server.arg("mquser");
  c.mqttPass = server.arg("mqpass");
  if (server.hasArg("screen")) c.screenDefault = server.arg("screen") == "1";

  portalDone = true;
  server.send(
      200, "text/html; charset=utf-8",
      "<!DOCTYPE html><html><meta charset=\"utf-8\"><body style='font-family:sans-serif;"
      "text-align:center;padding-top:80px'><h2>已保存</h2><p>设备即将重启，约需 20 秒。</p>"
      "<p>重启后请连接你的 Wi-Fi 网络等待设备自动上线。</p></body></html>");
}

void handleNotFound() {
  // captive portal：把未识别路径（含系统探测地址）跳回配置页
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// 注册网页路由：配置门户（AP 模式）与联网配置服务（STA 模式）共用同一套页面。
void registerRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/generate_204", handleNotFound);
  server.on("/hotspot-detect.html", handleNotFound);
  server.on("/connecttest.txt", handleNotFound);
  server.onNotFound(handleNotFound);
}

}  // namespace

bool runConfigPortal(AppConfig& cfg, const char* apSsidPrefix) {
  outCfg = &cfg;
  portalDone = false;

  String suffix = String((uint32_t)ESP.getEfuseMac(), HEX);
  if (suffix.length() > 4) suffix = suffix.substring(suffix.length() - 4);
  snprintf(apSsid, sizeof(apSsid), "%s-%s", apSsidPrefix, suffix.c_str());

  Serial.printf("[CFG] starting config portal AP: %s\n", apSsid);
  WiFi.mode(WIFI_AP);
  const bool ok = WiFi.softAP(apSsid, "airco212345");
  if (!ok) {
    Serial.println("[CFG] failed to start softAP");
    return false;
  }
  delay(500);
  IPAddress myIP = WiFi.softAPIP();
  Serial.printf("[CFG] AP IP: %s\n", myIP.toString().c_str());

  dns.start(53, "*", myIP);
  registerRoutes();
  server.begin();

  Serial.println("[CFG] portal running. Connect to AP and open http://192.168.4.1");
  Serial.println("[CFG] （浏览器访问任意地址会自动跳转配置页）");

  uint32_t lastBlink = millis();
  const uint32_t kTimeoutMs = 10UL * 60UL * 1000UL;  // AP 最长存活 10 分钟，超时重启回正常模式
  while (!portalDone) {
    dns.processNextRequest();
    server.handleClient();
    // 超时自动退出
    if (millis() - lastBlink > kTimeoutMs) {
      Serial.println("[CFG] portal timeout, restarting...");
      break;
    }
    delay(1);
  }

  server.close();
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  return portalDone;
}

bool beginConfigServer(AppConfig& cfg) {
  outCfg = &cfg;
  portalDone = false;
  // WiFi 重连/换 IP 后可能再次调用：先停掉旧实例再重新绑定端口。
  server.stop();
  server.close();
  registerRoutes();
  server.begin();
  Serial.println("[CFG] config server listening on port 80 (STA)");
  return true;
}

bool handleConfigServer() {
  server.handleClient();
  if (portalDone) {
    portalDone = false;
    return true;  // 用户点了【保存并重启】，调用方应写 NVS 并重启
  }
  return false;
}
