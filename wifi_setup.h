#pragma once
#include "stock_list.h"

#if defined(ESP8266)
// ESP8266 core 的 WiFi.h 其實是 ESP8266WiFi 庫的 alias（同 guard WiFi_h），
// 且此 core 安裝中 src/ 沒有 WiFi.h 實體檔，故直接包含 ESP8266WiFi.h。
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#else
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <EEPROM.h>
#endif

namespace wifi_config {
static const char* AP_NAME = "SPY-Ticker-Setup";
static const char* AP_PASSWORD = "configure";

static const uint32_t MAGIC = 0x53505957UL; // "SPYW"
static const int EEPROM_SIZE = 512;
static const int EEPROM_OFFSET = 16; // prefs shim 使用 0..3，stock list 從 160 開始。

struct Stored {
  uint32_t magic;
  char ssid[33];
  char password[65];
};

inline bool load(String& ssid, String& password) {
  Stored s;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(EEPROM_OFFSET, s);
  if (s.magic != MAGIC || s.ssid[0] == '\0' || s.ssid[32] != '\0' || s.password[64] != '\0') {
    return false;
  }
  ssid = s.ssid;
  password = s.password;
  return true;
}

inline void save(const String& ssid, const String& password) {
  Stored s = {};
  s.magic = MAGIC;
  ssid.substring(0, sizeof(s.ssid) - 1).toCharArray(s.ssid, sizeof(s.ssid));
  password.substring(0, sizeof(s.password) - 1).toCharArray(s.password, sizeof(s.password));
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(EEPROM_OFFSET, s);
  EEPROM.commit();
}

inline String htmlEscape(const String& in) {
  String out = in;
  out.replace("&", "&amp;");
  out.replace("<", "&lt;");
  out.replace(">", "&gt;");
  out.replace("\"", "&quot;");
  return out;
}

inline void portal() {
#if defined(ESP8266)
  ESP8266WebServer server(80);
#else
  WebServer server(80);
#endif
  DNSServer dns;
  String savedSsid, savedPassword;
  load(savedSsid, savedPassword);
  stock_list::load();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_NAME, AP_PASSWORD);
  dns.start(53, "*", WiFi.softAPIP());

  server.onNotFound([&server]() { server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); });
  server.on("/", HTTP_GET, [&server, &savedSsid]() {
    int n = WiFi.scanNetworks();
    String options;
    for (int i = 0; i < n; ++i) {
      String name = WiFi.SSID(i);
      if (name.length() == 0) continue;
      options += "<option value=\"" + htmlEscape(name) + "\"";
      if (name == savedSsid) options += " selected";
      options += ">" + htmlEscape(name) + " (" + String(WiFi.RSSI(i)) + " dBm)</option>";
    }
    WiFi.scanDelete();

    String page = F("<!doctype html><html><meta name='viewport' content='width=device-width,initial-scale=1'>"
                    "<title>SPY Ticker Setup</title><style>body{font:18px sans-serif;max-width:520px;margin:32px auto;padding:0 18px}"
                    "input,select,button{box-sizing:border-box;font-size:18px;width:100%;padding:10px;margin:8px 0}button{background:#111;color:white}"
                    "label{font-weight:700;display:block;margin-top:14px}.hint{color:#555;font-size:14px}</style>"
                    "<h2>SPY Ticker 設定</h2><form method='POST' action='/save'>"
                    "<label>Wi-Fi</label><select name='ssid'>");
    page += options;
    page += F("</select><label>密碼</label><input name='password' type='password' autocomplete='off' placeholder='不變更可留空'>");
    page += F("<label>股票清單</label><input name='tickers' autocomplete='off' value=\"");
    page += htmlEscape(stock_list::csv());
    page += F("\"><div class='hint'>用逗號分隔，最多 10 個，例如 SPY,AAPL,TSLA,AMD</div>"
              "<button type='submit'>儲存並重啟</button></form><p class='hint'>若看不到 Wi-Fi，請靠近路由器後重新整理。</p></html>");
    server.send(200, "text/html; charset=utf-8", page);
  });
  server.on("/save", HTTP_POST, [&server, &savedSsid, &savedPassword]() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    String tickers = server.arg("tickers");

    if (ssid.length() == 0 || ssid.length() > 32 || password.length() > 64) {
      server.send(400, "text/plain; charset=utf-8", "SSID 或密碼長度不正確");
      return;
    }
    if (password.length() == 0 && ssid == savedSsid) password = savedPassword;
    if (!stock_list::saveFromCsv(tickers)) {
      server.send(400, "text/plain; charset=utf-8", "股票清單格式不正確，請用逗號分隔，最多 10 個，每個代號最多 11 字元");
      return;
    }

    save(ssid, password);
    server.send(200, "text/html; charset=utf-8", "<h2>已儲存，裝置即將重啟...</h2>");
    delay(1200);
    ESP.restart();
  });
  server.begin();
  Serial.printf("[WiFi] setup AP: %s / password: %s / open http://%s\n",
                AP_NAME, AP_PASSWORD, WiFi.softAPIP().toString().c_str());

  while (true) {
    dns.processNextRequest();
    server.handleClient();
    delay(2);
  }
}
} // namespace wifi_config

inline void wifi_connect(const char* fallbackSsid, const char* fallbackPassword) {
  String savedSsid, savedPassword;
  const bool hasSaved = wifi_config::load(savedSsid, savedPassword);
  const char* ssid = hasSaved ? savedSsid.c_str() : fallbackSsid;
  const char* password = hasSaved ? savedPassword.c_str() : fallbackPassword;

  Serial.printf("[WiFi] connecting to %s%s\n", ssid, hasSaved ? " (saved)" : " (default)");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(300);
    Serial.print(".");
  }
  Serial.printf("\nWiFi %s\n", (WiFi.status()==WL_CONNECTED)?"connected":"failed");
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] starting phone setup portal");
    wifi_config::portal();
  }
}
