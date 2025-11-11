#pragma once
#include <WiFiClientSecure.h>
#include <WiFi.h>             
#include <HTTPClient.h>
#include <ArduinoJson.h>

// 換資料源：0=Yahoo直連, 1=Yahoo經r.jina.ai, 2=Stooq(建議先用這個)
#define PROVIDER 2

struct Quote { float price; float pct; bool ok; };

#if PROVIDER == 2
// -------------------- Stooq CSV 版 --------------------
static String stooq_url(const char* sym) {
  // 符號要加 .us，例如 SPY → spy.us；大小寫不拘
  String s = String(sym); s.toLowerCase(); s += ".us";
  return String("https://stooq.com/q/l/?s=") + s + "&f=sd2t2ohlcv&h&e=csv";
}

inline Quote fetch_quote(const char* sym) {
  Quote q{0,0,false};
  if (WiFi.status() != WL_CONNECTED) { Serial.println("[fetch] no WiFi"); return q; }

  WiFiClientSecure cli; cli.setInsecure(); cli.setTimeout(15000);
  HTTPClient http; http.setConnectTimeout(15000);

  String url = stooq_url(sym);
  if (!http.begin(cli, url)) { Serial.println("[fetch] begin fail"); return q; }
  http.addHeader("User-Agent","Mozilla/5.0");
  int code = http.GET();
  Serial.printf("[fetch][Stooq] HTTP %d\n", code);
  if (code != 200) { http.end(); return q; }

  String csv = http.getString();
  http.end();
  // 例：Symbol,Date,Time,Open,High,Low,Close,Volume\nspy.us,2025-09-15,21:00:09,xxx,xxx,xxx,499.12,xxxxx
  int nl = csv.indexOf('\n');
  if (nl < 0) return q;
  String line = csv.substring(nl+1);
  line.trim();
  if (line.length() == 0 || line.indexOf(",N/D,") >= 0) return q; // N/D = no data (休市、代號錯)

  // 簡單切欄
  // idx: 0=Symbol,1=Date,2=Time,3=Open,4=High,5=Low,6=Close,7=Volume
  float vals[8] = {0};
  int idx = 0, start = 0;
  for (int i=0; i<=line.length(); ++i) {
    if (i==line.length() || line[i]==',') {
      String token = line.substring(start, i);
      if (idx==3 || idx==4 || idx==5 || idx==6) vals[idx] = token.toFloat();
      start = i+1; idx++; if (idx>7) break;
    }
  }
  float open = vals[3];
  float last = vals[6];
  if (last <= 0) return q;

  float pct = NAN;
  if (open > 0) pct = (last - open) / open * 100.0f;

  q.price = last;
  q.pct   = pct;
  q.ok    = true;
  Serial.printf("[fetch][Stooq] %s -> %.2f (%.2f%%)\n", sym, q.price, q.pct);
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
  HTTPClient http; http.setConnectTimeout(15000); http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

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