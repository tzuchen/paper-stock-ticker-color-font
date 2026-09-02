#include "config.h"
#include "pins.h"
#include "stock_list.h"
#include "display_driver.h"
#include "display_ui.h"     // 用 draw_time_strip() 組字串並呼叫 display_draw_time_bar()
#include "wifi_setup.h"
#include "market_time.h"
#include "fetch_quote.h"
#include "ota_update.h"
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
static unsigned long lastOta   = 0;

// V2.44 的 GPIO0 同時是按鍵 2 與 eink DC：螢幕傳輸時必須是輸出，
// 傳輸完成後釋放成 INPUT_PULLUP 才能讀到按鍵。
static void buttons_for_read() {
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);
}

static void display_for_write() {
  pinMode(EPD_DC, OUTPUT);
}


// ---- 偏好（記憶目前股票序號）----
Preferences prefs;

// ────── 單次更新：畫三行 + 時間條 → 送出 ──────
static void update_once(bool /*full*/) {
  display_for_write();
  const char* ticker = TICKERS[curIdx];
  float price = NAN, pct = NAN;

  { Quote q = fetch_quote(ticker); price = q.price; pct = q.pct; }

  display_draw_three_lines(ticker, price, pct, true);
  draw_time_strip();         // 時間條
  display_present();         // 一次送出
  buttons_for_read();        // GPIO0 與螢幕共用，刷新後釋放給按鍵

  lastPrice = price;
  lastPct   = pct;
}

// ────── 診斷畫面：固定內容，確認 eink 渲染是否正確 ──────
static void show_diagnostic() {
  display_for_write();
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
  Serial.printf("[BTN] stock button -> %s (index %d)\n", TICKERS[curIdx], curIdx);
  update_once(true);
}

static void prev_stock() {
  curIdx = (curIdx + N_TICKERS - 1) % N_TICKERS;
  prefs.putInt("curIdx", curIdx);
  Serial.printf("[BTN] stock button <- %s (index %d)\n", TICKERS[curIdx], curIdx);
  update_once(true);
}

// ────── Boot 畫面顯示（版本＋Wi-Fi 狀態）──────
static void show_boot_banner_and_connect() {
  display_for_write();
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

  if (WiFi.status() == WL_CONNECTED) {
    band_to_white(0, LOG_H/2, LOG_W, LOG_H);
    gfx_draw_centered("Checking update...",
                      0, LOG_H/2, LOG_W, LOG_H,
                      &FreeSansBold12pt7b, BW_BLACK, BW_WHITE);
    display_present();
    ota_update::checkAndUpdate();
  }

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
  buttons_for_read();
}

void setup() {
  Serial.begin(115200);
  #if defined(ESP8266)
  prefs.begin("epaper");
#else
  prefs.begin("epaper", false);
#endif

  int saved = prefs.getInt("curIdx", 0);
  if (saved >= 0 && saved < N_TICKERS) curIdx = saved;

  display_init();
  buttons_for_read();
  // 可選診斷：原生 API 測試圖形（只限 ESP8266；預設關閉，讓 ticker 繼續執行）
#if defined(ESP8266) && defined(EPD_NATIVE_DIAGNOSTIC)
  epd_bw_native_test();
#endif
  delay(5000);
  show_boot_banner_and_connect();

  update_once(true);
  lastFull = lastPoll = lastOta = millis();
}

void loop() {
  unsigned long now = millis();



  // 兩顆功能鍵：去彈跳後一次按下只換一次，避免按住時連續換股
  {
    static bool lastRawPrev = HIGH, stablePrev = HIGH;
    static bool lastRawNext = HIGH, stableNext = HIGH;
    static unsigned long changedPrev = 0, changedNext = 0;
    static unsigned long nextPressedAt = 0;
    static bool nextLongAction = false;
    static const unsigned long SETUP_HOLD_MS = 3000;

    bool rawPrev = digitalRead(BTN_PREV);
    if (rawPrev != lastRawPrev) { lastRawPrev = rawPrev; changedPrev = now; }
    if (rawPrev != stablePrev && now - changedPrev >= 40) {
      stablePrev = rawPrev;
      Serial.printf("[BTN] GPIO%d -> %s\n", BTN_PREV, stablePrev == LOW ? "PRESSED" : "RELEASED");
      if (stablePrev == LOW) prev_stock();
    }

    bool rawNext = digitalRead(BTN_NEXT);
    if (rawNext != lastRawNext) { lastRawNext = rawNext; changedNext = now; }
    if (rawNext != stableNext && now - changedNext >= 40) {
      stableNext = rawNext;
      Serial.printf("[BTN] GPIO%d -> %s\n", BTN_NEXT, stableNext == LOW ? "PRESSED" : "RELEASED");
      if (stableNext == LOW) {
        nextPressedAt = now;
        nextLongAction = false;
      } else if (!nextLongAction) {
        // 短按：下一檔；長按已在按住期間進入設定模式，不再換股。
        next_stock();
      }
    }

    // 紅鍵長按 3 秒進入手機設定 AP。
    // 左側 RESET 是硬體 reset，無法用韌體計時，因此使用實測 GPIO3 的紅鍵。
    if (stableNext == LOW && !nextLongAction && now - nextPressedAt >= SETUP_HOLD_MS) {
      nextLongAction = true;
#if defined(ESP8266)
      Serial.println("[BTN] red button held 3s -> starting phone setup portal");
      wifi_config::portal();
#else
      Serial.println("[BTN] red button held 3s -> setup portal unavailable on this build");
#endif
    }
  }

  // 定時抓價
  if (now - lastPoll >= POLL_MS) {
    lastPoll = now;
    update_once(false);
  }

  // 長時間運行時也定期檢查 GitHub Releases。
  if (now - lastOta >= OTA_CHECK_MS) {
    lastOta = now;
    ota_update::checkAndUpdate();
  }

  // 週期全刷（抑制殘影）
  if (now - lastFull >= FULL_REFRESH_MS) {
    lastFull = now;
    update_once(true);
  }
}
