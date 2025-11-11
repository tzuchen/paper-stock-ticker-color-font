#include "config.h"
#include "pins.h"
#include "stock_list.h"
#include "display_driver.h"
#include "display_ui.h"     // 用 draw_time_strip() 來組字串並呼叫 display_draw_time_bar()
#include "wifi_setup.h"
#include "market_time.h"
#include "fetch_quote.h"

#include <WiFi.h>
#include <time.h>
#include <Preferences.h>

// ---- 狀態 ----
static int          curIdx        = 0;
static float        lastPrice     = NAN;
static float        lastPct       = NAN;
static unsigned long lastFull     = 0;
static unsigned long lastPoll     = 0;

// ---- 偏好（記憶目前股票序號）----
Preferences prefs;

// ---- 單次更新：畫三行 + 畫時間條 → 送出 ----
static void update_once(bool /*full*/) {
  const char* ticker = TICKERS[curIdx];
  float price = NAN, pct = NAN;

  { Quote q = fetch_quote(ticker); price = q.price; pct = q.pct; }

  // 畫面（先畫到 buffer）
  display_draw_three_lines(ticker, price, pct, true);

  // 時間條（透過 display_ui.h 內的 draw_time_strip 組字串並呼叫 display_draw_time_bar）
  draw_time_strip();

  // 一次送出顯示
  display_present();

  lastPrice = price;
  lastPct   = pct;
}

static void next_stock() {
  curIdx = (curIdx + 1) % N_TICKERS;
  prefs.putInt("curIdx", curIdx);
  update_once(true);
}

void setup() {
  // Setup NTP and timezone
  markettime_setup_tz();
  markettime_wait_synced(15000);

  Serial.begin(115200);
  prefs.begin("epaper", false);

  int saved = prefs.getInt("curIdx", 0);
  if (saved >=0 && saved < N_TICKERS) curIdx = saved;

  display_init();
  wifi_connect(WIFI_SSID, WIFI_PSK);

  update_once(true);
  lastFull = lastPoll = millis();
}

void loop() {
  unsigned long now = millis();

  // BOOT 按鍵切換股票
  if (digitalRead(BTN_BOOT) == LOW) {
    static unsigned long lastBtn = 0;
    if (now - lastBtn > 250) { next_stock(); lastBtn = now; }
  }

  // 定時抓價
  if (now - lastPoll >= POLL_MS) { lastPoll = now; update_once(false); }

  // 週期全刷（抑制殘影）
  if (now - lastFull >= FULL_REFRESH_MS) { lastFull = now; update_once(true); }
}