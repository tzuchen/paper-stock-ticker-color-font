#pragma once
// ESP8266 黑白 2.9" T5D (Z10) eink 驅動
// 使用 GxEPD2_290_T5D + GxEPD2_BW（Waveshare 官方）
// 296×128，黑白，SPI
//
// Page 模型（一個 frame = 一次全刷）：
//   draw_three_lines  → firstPage + 填白 + 畫三行，page 保持開著
//   draw_time_bar     → 補畫時間條 + nextPage() 送出 + powerOff，page 收掉
//   present()         → 若 page 還開著（只有三行、沒有時間條）就補送；否則 no-op
//   band_to_white     → page 沒開時自己開 page + 填白；page 開著就直接 fillRect

#include <Arduino.h>
#include <SPI.h>
#include "pins.h"

// GxEPD2 庫：此環境 arduino-cli 的 library detector 只掃描 src/*.h，
// 收不到 src/epd/GxEPD2_290_T5D.h，故該頭檔經 sketch 層的 shim 檔
// GxEPD2_290_T5D.h（重導出）引入；GxEPD2_BW.h 在 src/ 下，偵測器收得到。
// 用相對路徑而非尖號：<GxEPD2_290_T5D.h> 不會觸發 sketch 目錄搜尋。
#include "GxEPD2_290_T5D.h"
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

// GxEPD2 例項（全域）；pin 名稱由 pins.h 定義（EPD_CS / EPD_DC / EPD_RST /
// EPD_BUSY，ESP8266 分支）
static GxEPD2_290_T5D epd(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);
// 296px page = 整個 128x296 面板一次放入 buffer；避免 paged API 要求
// 每一頁都重繪，而目前 ticker 的繪製流程是一次完成整張 frame。
static GxEPD2_BW<GxEPD2_290_T5D, 296> gfx(epd);

// page 狀態：firstPage 後 page 是開的，nextPage 送完才關
static bool s_page_open = false;

// 初始化（boot 時全刷白）
inline void epd_bw_init() {
  Serial.println("[EPD] init start");
  gfx.init(115200, true, 2, false);  // Waveshare 板卡 2ms reset pulse
  gfx.setRotation(3);                // 旋轉 90° CCW → 邏輯尺寸 296×128（ESP8266 實機方向）
  Serial.printf("[EPD] after init: W=%d H=%d rot=%d\n", gfx.width(), gfx.height(), gfx.getRotation());
  gfx.firstPage();
  gfx.fillScreen(GxEPD_WHITE);
  Serial.print("[EPD] refresh... ");
  unsigned long t0 = millis();
  while (gfx.nextPage()) { }
  gfx.powerOff();
  s_page_open = false;
  Serial.printf("[EPD] init done in %dms\n", millis() - t0);
  delay(200);
}

// 診斷：用 GxEPD2 原生 API 畫測試圖形（繞過自訂 draw 函式），
// 確認 eink 驅動本身是否正常 + 畫面方向
inline void epd_bw_native_test() {
  // 原生 128×296 buffer，畫四象限：左上黑、右上白、左下白、右下黑。
  // 使用 writeImageForFullRefresh() 同時寫 previous/current buffer，避免
  // 面板在前一個狀態未知時只更新 current buffer 而顯示異常。
  epd.init(115200, true, 2, false);
  // ESP8266 stack 很小；這個 buffer 必須放在靜態記憶體，不能放函式 stack。
  static uint8_t buf[128 * 296 / 8];  // 4736 bytes, 16 bytes/row
  memset(buf, 0xFF, sizeof(buf));
  for (int ny = 0; ny < 148; ny++) {
    for (int nx = 0; nx < 64; nx++) {
      buf[ny * 16 + nx / 8] &= ~(1 << (7 - nx % 8));
    }
  }
  for (int ny = 148; ny < 296; ny++) {
    for (int nx = 64; nx < 128; nx++) {
      buf[ny * 16 + nx / 8] &= ~(1 << (7 - nx % 8));
    }
  }
  epd.writeImageForFullRefresh(buf, 0, 0, 128, 296);
  epd.refresh(false);
  epd.powerOff();
  s_page_open = false;
  delay(200);
  Serial.printf("[EPD] native test done (RAW 4-QUADRANT): TL+BR black\n");
  // 停住：不再進入 boot banner / WiFi / ticker loop，避免 eink 反覆全刷
  while (true) {
    delay(1000);
  }
}

// 關機
inline void epd_bw_deinit() {
  gfx.powerOff();
  s_page_open = false;
}

// 清屏（全刷白）
inline void epd_bw_clear() {
  gfx.init(0, true, 2, false);
  gfx.firstPage();
  gfx.fillScreen(GxEPD_WHITE);
  while (gfx.nextPage()) { }
  gfx.powerOff();
  s_page_open = false;
  delay(100);
}

// 送出目前 frame：若 page 還開著就補 nextPage + powerOff；否則 no-op
inline void epd_bw_present() {
  if (s_page_open) {
    while (gfx.nextPage()) { }
    gfx.powerOff();
    s_page_open = false;
    delay(100);
  }
}

// 置中繪字（簡化版；GxEPD2 無 drawCentreString，用 getTextBounds 手動置中）
inline void epd_bw_draw_centered(const String& s, int x0, int y0, int x1, int y1,
                                 const GFXfont* font, uint16_t textColor, uint16_t bgColor) {
  gfx.setFont(font);
  gfx.setTextColor(textColor, bgColor);
  int16_t bx, by; uint16_t bw, bh;
  gfx.getTextBounds(s.c_str(), 0, 0, &bx, &by, &bw, &bh);
  int16_t cx = x0 + (x1 - x0) / 2 - (int)bw / 2;
  // GFX 的 by 通常是負值；置中時必須把 glyph top/baseline 偏移算回來，
  // 否則第一行會被推到面板上緣外，看不到 ticker（例如 SPY）。
  int16_t cy = y0 + ((y1 - y0) - (int)bh) / 2 - by;
  gfx.setCursor(cx, cy);
  gfx.print(s);
}

// 畫三行（ticker / price / pct）：開 frame，page 保持開著，等時間條或 present() 收掉
inline void epd_bw_draw_three_lines(const char* ticker, float price, float pct, bool /*fullRefresh*/) {
  // init 會重置 GxEPD2 狀態；放在 firstPage 之前（每次都是新 frame 的開頭）
  gfx.init(0, false, 2, false);
  gfx.setRotation(3);  // 確保 rotation 正確（init 會重置）

  Serial.printf("[EPD] draw_three_lines: %s price=%.2f pct=%.2f\n", ticker, price, pct);

  // 版面：296×128（橫向，rotation=3）
  int margin = 8;
  int gap = 4;
  int l1_h = 24;  // ticker
  int l2_h = 48;  // price
  int l3_h = 24;  // pct

  int y = margin;
  int y1_0 = y; y += l1_h + gap;
  int y2_0 = y; y += l2_h + gap;
  int y3_0 = y; y += l3_h;

  gfx.firstPage();
  gfx.fillScreen(GxEPD_WHITE);
  s_page_open = true;

  // 第一行：ticker
  epd_bw_draw_centered(String(ticker), 0, y1_0, 296, y1_0 + l1_h,
                       &FreeMonoBold18pt7b, GxEPD_BLACK, GxEPD_WHITE);

  // 第二行：price
  String sp = (price > 0) ? String(price, 2) : String("--");
  // Adafruit GFX 的 setTextColor(fg, bg) 只處理字元背景像素，
  // 不會把整個價格區塊填滿；先明確填黑，才能讓白色價格反白顯示。
  gfx.fillRect(0, y2_0, 296, l2_h, GxEPD_BLACK);
  epd_bw_draw_centered(sp, 0, y2_0, 296, y2_0 + l2_h,
                       &FreeSansBold24pt7b, GxEPD_WHITE, GxEPD_BLACK);

  // 第三行：pct
  String spct = isnan(pct) ? String("--") : (String(pct>=0?"+":"") + String(pct,2) + "%");
  epd_bw_draw_centered(spct, 0, y3_0, 296, y3_0 + l3_h,
                       &FreeMonoBold12pt7b, GxEPD_BLACK, GxEPD_WHITE);
}

// 畫時間條：補在同一個 frame 的最底，然後把整個 frame 送出去
inline void epd_bw_draw_time_bar(const String& leftTP, const String& midNY, const String& rightCNT) {
  if (!s_page_open) {
    gfx.init(0, false, 2, false);
    gfx.firstPage();
    gfx.fillScreen(GxEPD_WHITE);
    s_page_open = true;
  }

  int time_0 = 128 - 16;
  int seg = 296 / 3;

  epd_bw_draw_centered(leftTP,  0, time_0, seg, 128, &FreeMonoBold9pt7b, GxEPD_BLACK, GxEPD_WHITE);
  epd_bw_draw_centered(midNY,   seg, time_0, 2*seg, 128, &FreeMonoBold9pt7b, GxEPD_BLACK, GxEPD_WHITE);
  epd_bw_draw_centered(rightCNT, 2*seg, time_0, 296, 128, &FreeMonoBold9pt7b, GxEPD_BLACK, GxEPD_WHITE);

  while (gfx.nextPage()) { }
  gfx.powerOff();
  s_page_open = false;
  delay(100);
}

// 把某區域刷白（ESP8266 黑白版：page 沒開時先開 page + 填白，再 fillRect）
inline void epd_bw_band_to_white(int x0, int y0, int x1, int y1) {
  if (!s_page_open) {
    gfx.init(0, false, 2, false);
    gfx.firstPage();
    gfx.fillScreen(GxEPD_WHITE);
    s_page_open = true;
  }
  gfx.fillRect(x0, y0, x1 - x0, y1 - y0, GxEPD_WHITE);
}
