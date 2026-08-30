#pragma once
#include <WiFiClientSecure.h>
#if defined(ESP8266)
// 同 wifi_setup.h：此 core 安裝中 src/ 缺 WiFi.h，直接包含 ESP8266WiFi.h（同 guard WiFi_h）。
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif             
#if defined(ESP8266)
// 此 core 的 HTTPClient.h 位於 ESP8266HTTPClient 庫 src/ 下，偵測器未收進 include path，
// 故以絕對路徑包含（ESP32 走原路徑）。
#include <ESP8266HTTPClient.h>
#else
#include <HTTPClient.h>
#endif
#include <ArduinoJson.h>

// 換資料源：0=Yahoo直連, 1=Yahoo經r.jina.ai, 2=Yahoo v8 chart API
#define PROVIDER 2

struct Quote { float price; float pct; bool ok; };

#if PROVIDER == 2
// -------------------- Yahoo v8 chart API --------------------
static String yahoo_v8_url(const char* sym) {
  return String("https://query1.finance.yahoo.com/v8/finance/chart/") + sym + "?range=1d&interval=1d";
}

inline Quote fetch_quote(const char* sym) {
  Quote q{0,0,false};
  if (WiFi.status() != WL_CONNECTED) { Serial.println("[fetch] no WiFi"); return q; }

  WiFiClientSecure cli; cli.setInsecure(); cli.setTimeout(15000);
  HTTPClient http; http.setTimeout(15000);

  String url = yahoo_v8_url(sym);
  if (!http.begin(cli, url)) { Serial.println("[fetch] begin fail"); return q; }
  http.addHeader("User-Agent", "Mozilla/5.0");
  http.addHeader("Accept", "application/json");
  int code = http.GET();
  Serial.printf("[fetch][Yahoo v8] HTTP %d\n", code);
  if (code != 200) { http.end(); return q; }

  String body = http.getString();
  http.end();

  StaticJsonDocument<4096> doc;
  if (deserializeJson(doc, body)) { Serial.println("[fetch] JSON parse fail"); return q; }

  JsonVariant meta = doc["chart"]["result"][0]["meta"];
  if (meta.isNull()) { Serial.println("[fetch] no meta"); return q; }

  float price = meta["regularMarketPrice"] | -1.0f;
  float prev  = meta["chartPreviousClose"] | -1.0f;
  if (price <= 0) { Serial.printf("[fetch] bad price for %s\n", sym); return q; }

  float pct = NAN;
  if (prev > 0) pct = (price - prev) / prev * 100.0f;

  q.price = price;
  q.pct   = pct;
  q.ok    = true;
  Serial.printf("[fetch][Yahoo v8] %s -> %.2f (%.2f%%)\n", sym, q.price, q.pct);
  return q;
}

#else
// -------------------- Yahoo 版本（保留，之後想切回再用） --------------------
#define USE_JINA_PROXY (PROVIDER==1)

static String build_url_v7(const char* sym) {
  String u = String("https://query1.finance.yahoo.com/v7/finance/quote?symbols=") + sym;
#if USE_JINA_PROXY
  return String("https://r.jina.ai/http/") + u;   // 注意：必須 /http/https://...；若仍 400，代表被封
#else
  return u;
#endif
}

static String build_url_v10(const char* sym) {
  String u = String("https://query2.finance.yahoo.com/v10/finance/quoteSummary/") + sym + "?modules=price";
#if USE_JINA_PROXY
  return String("https://r.jina.ai/http/") + u;
#else
  return u;
#endif
}

inline bool parse_v7(const String& payload, Quote& q) {
  StaticJsonDocument<12288> doc; if (deserializeJson(doc, payload)) return false;
  JsonVariant res = doc["quoteResponse"]["result"][0]; if (res.isNull()) return false;
  float p = res["regularMarketPrice"] | res["postMarketPrice"] | res["preMarketPrice"] | -1.0;
  float cp = res["regularMarketChangePercent"] | res["postMarketChangePercent"] | res["preMarketChangePercent"] | NAN;
  if (p <= 0) return false; q.price=p; q.pct=cp; q.ok=true; return true;
}

inline bool parse_v10(const String& payload, Quote& q) {
  StaticJsonDocument<12288> doc; if (deserializeJson(doc, payload)) return false;
  JsonVariant p = doc["quoteSummary"]["result"][0]["price"]; if (p.isNull()) return false;
  float last = p["regularMarketPrice"]["raw"] | p["postMarketPrice"]["raw"] | p["preMarketPrice"]["raw"] | -1.0;
  float chg  = p["regularMarketChangePercent"]["raw"] | NAN;
  if (last <= 0) return false; q.price=last; q.pct=chg; q.ok=true; return true;
}

inline Quote fetch_quote(const char* sym) {
  Quote q{0,0,false};
  if (WiFi.status() != WL_CONNECTED) { Serial.println("[fetch] no WiFi"); return q; }

  WiFiClientSecure cli; cli.setInsecure(); cli.setTimeout(15000);
  HTTPClient http; http.setTimeout(15000); http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  const char* ua = "Mozilla/5.0 (Windows NT 10.0; Win64; x64)";

  String url1 = build_url_v7(sym);
  if (http.begin(cli, url1)) {
    http.addHeader("User-Agent", ua); http.addHeader("Accept", "application/json");
    int c = http.GET(); Serial.printf("[fetch][Yahoo v7] HTTP %d\n", c);
    if (c==200) { String body=http.getString(); http.end(); if (parse_v7(body,q)) return q; }
    http.end();
  }
  String url2 = build_url_v10(sym);
  if (http.begin(cli, url2)) {
    http.addHeader("User-Agent", ua); http.addHeader("Accept", "application/json");
    int c = http.GET(); Serial.printf("[fetch][Yahoo v10] HTTP %d\n", c);
    if (c==200) { String body=http.getString(); http.end(); if (parse_v10(body,q)) return q; }
    http.end();
  }
  return q;
}
#endif