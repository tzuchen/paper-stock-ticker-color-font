#include "config.h"
#include "pins.h"
#include "stock_list.h"
#include "display_driver.h"
#include "display_ui.h"     // 用 draw_time_strip() 組字串並呼叫 display_draw_time_bar()
#include "wifi_setup.h"
#include "market_time.h"
#include "fetch_quote.h"
#include "version.h"

#include <time.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <GxEPD2_BW.h>
#include "GxEPD2_290_T5D.h"
#include <EEPROM.h>
#include "Preferences.h"
#else
#include <WiFi.h>
#include <Preferences.h>
#endif

// ---- 狀態 ----
static int           curIdx    = 0;
static float         lastPrice = NAN;
static float         lastPct   = NAN;
static unsigned long lastFull  = 0;
static unsigned long lastPoll  = 0;

// ---- 偏好（記憶目前股票序號）----
Preferences prefs;

// ────── 單次更新：畫三行 + 時間條 → 送出 ──────
static void update_once(bool /*full*/) {
  const char* ticker = TICKERS[curIdx];
  float price = NAN, pct = NAN;

  { Quote q = fetch_quote(ticker); price = q.price; pct = q.pct; }

  display_draw_three_lines(ticker, price, pct, true);
  draw_time_strip();         // 時間條
  display_present();         // 一次送出

  lastPrice = price;
  lastPct   = pct;
}

// ────── 診斷畫面：固定內容，確認 eink 渲染是否正確 ──────
static void show_diagnostic() {
  band_to_white(0, 0, LOG_W, LOG_H);
  gfx_draw_centered(String("DIAG OK"), 0, 8, LOG_W, 48,
                    &FreeSansBold24pt7b, BW_BLACK, BW_WHITE);
  gfx_draw_centered(String("1234567890"), 0, 56, LOG_W, 88,
                    &FreeMonoBold18pt7b, BW_BLACK, BW_WHITE);
  gfx_draw_centered(String("ESP8266 eink test"), 0, 96, LOG_W, 120,
                    &FreeMonoBold9pt7b, BW_BLACK, BW_WHITE);
  display_present();
  Serial.println("[DIAG] diagnostic frame presented");
}

static void next_stock() {
  curIdx = (curIdx + 1) % N_TICKERS;
  prefs.putInt("curIdx", curIdx);
  update_once(true);
}

// ────── Boot 畫面顯示（版本＋Wi-Fi 狀態）──────
static void show_boot_banner_and_connect() {
  // 白底
  band_to_white(0, 0, LOG_W, LOG_H);

  // 上半：版本資訊
  {
    String v = String("Firmware ") + FIRMWARE_VERSION;
    String b = String("Build ") + BUILD_DATE + " " + BUILD_TIME;
    gfx_draw_centered(v, 0, 0, LOG_W, LOG_H/2 - 6,
                      &FreeSansBold18pt7b, BW_BLACK, BW_WHITE);
    gfx_draw_centered(b, 0, (LOG_H/2) - 4, LOG_W, LOG_H - 8,
                      &FreeMonoBold9pt7b, BW_BLACK, BW_WHITE);
  }

  // 下半：Wi-Fi 連線中…
  gfx_draw_centered("Connecting Wi-Fi...",
                    0, LOG_H/2, LOG_W, LOG_H,
                    &FreeSansBold12pt7b, BW_BLACK, BW_WHITE);
  display_present();

  // 連線
  wifi_connect(WIFI_SSID, WIFI_PSK);

  // 連上後再做 NTP/時區同步（避免無網時白等）
  markettime_setup_tz();
  markettime_wait_synced(15000);

  // 顯示結果
  const bool ok = (WiFi.status() == WL_CONNECTED);
  const char* msg = ok ? "Wi-Fi Connected!" : "Wi-Fi Failed!";
  band_to_white(0, LOG_H/2, LOG_W, LOG_H);
  gfx_draw_centered(msg,
                    0, LOG_H/2, LOG_W, LOG_H,
                    &FreeSansBold12pt7b, BW_BLACK, BW_WHITE);
  display_present();
  delay(1200);

  // 清白底，交還給主流程
  band_to_white(0, 0, LOG_W, LOG_H);
  display_present();
}

void setup() {
  Serial.begin(115200);
  #if defined(ESP8266)
  prefs.begin("epaper");
#else
  prefs.begin("epaper", false);
#endif

  // 若 BTN_BOOT 需上拉
  pinMode(BTN_BOOT, INPUT_PULLUP);

  int saved = prefs.getInt("curIdx", 0);
  if (saved >= 0 && saved < N_TICKERS) curIdx = saved;

  display_init();
  // 可選診斷：原生 API 測試圖形（只限 ESP8266；預設關閉，讓 ticker 繼續執行）
#if defined(ESP8266) && defined(EPD_NATIVE_DIAGNOSTIC)
  epd_bw_native_test();
#endif
  delay(5000);
  show_boot_banner_and_connect();

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
  if (now - lastPoll >= POLL_MS) {
    lastPoll = now;
    update_once(false);
  }

  // 週期全刷（抑制殘影）
  if (now - lastFull >= FULL_REFRESH_MS) {
    lastFull = now;
    update_once(true);
  }
}
