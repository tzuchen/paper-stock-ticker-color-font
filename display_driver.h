#pragma once
#include <Arduino.h>
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include "config.h"

// Adafruit GFX & FreeFonts
#include <Adafruit_GFX.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

// ---------- Waveshare 2.9" tri-color V4 ----------
#define PHY_W EPD_2IN9B_V4_WIDTH   // 128 (未旋轉)
#define PHY_H EPD_2IN9B_V4_HEIGHT  // 296
#define LOG_W (PHY_H)              // 296 (旋轉270後的邏輯寬)
#define LOG_H (PHY_W)              // 128 (旋轉270後的邏輯高)

// 顏色：你的環境已確認 0xFF=白、0x00=黑
static const UWORD BW_WHITE = 0xFF;
static const UWORD BW_BLACK = 0x00;

// 兩層影像緩衝
static UBYTE* BlackImage = nullptr; // 黑層：0=黑, FF=白
static UBYTE* RYImage    = nullptr; // 紅層：0=紅, FF=白
static UWORD  Imagesize  = 0;

// ---- Adafruit_GFX 封裝到 Waveshare Paint ----
// 直接呼叫 Paint_DrawPoint，讓旋轉/位元編碼完全由庫處理，避免「花字」
class FramebufferGFX : public Adafruit_GFX {
  public:
    FramebufferGFX(int16_t w, int16_t h, bool toRedLayer)
      : Adafruit_GFX(w, h), isRed(toRedLayer) {}

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
      if (x < 0 || y < 0 || x >= width() || y >= height()) return;
      // 切到正確影像
      Paint_SelectImage(isRed ? RYImage : BlackImage);
      // 讓 Waveshare Paint 幫我們處理旋轉與位元定位
      UWORD col = (color == BW_BLACK) ? BLACK : WHITE;
      Paint_DrawPoint(x, y, col, DOT_PIXEL_1X1, DOT_STYLE_DFT);
    }
  private:
    bool isRed;
};

static FramebufferGFX* gfxBlack = nullptr;
static FramebufferGFX* gfxRed   = nullptr;

// 版面切割
struct Bands {
  int x0, x1;
  int y1_0, y1_h;     // Ticker
  int y2_0, y2_h;     // Price
  int y3_0, y3_h;     // Percent
  int time_0, time_h; // Time bar
};

inline Bands layoutBands() {
  Bands b;
  b.x0 = 0; b.x1 = LOG_W;
  int y = MARGIN;
  b.y1_0 = y;       b.y1_h = L1_H; y += L1_H + GAP;
  b.y2_0 = y;       b.y2_h = L2_H; y += L2_H + GAP;
  b.y3_0 = y;       b.y3_h = L3_H;
  b.time_0 = LOG_H - TIME_STRIP_H;
  b.time_h = TIME_STRIP_H;
  return b;
}

// 工具：把某 band 兩層刷白
static inline void band_to_white(int x0,int y0,int x1,int y1) {
  Paint_SelectImage(BlackImage);
  Paint_DrawRectangle(x0, y0, x1-1, y1-1, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
  Paint_SelectImage(RYImage);
  Paint_DrawRectangle(x0, y0, x1-1, y1-1, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
}

// 置中繪字：先由 Paint 填底，再由 GFX 畫字（避免每像素慢速填背景）
static inline void gfx_draw_centered(const String& s, int x0,int y0,int x1,int y1,
                                     const GFXfont* font, uint16_t textColor, uint16_t bgColor,
                                     bool toRedLayer=false) {
  // 填背景（選對層）
  Paint_SelectImage(toRedLayer ? RYImage : BlackImage);
  Paint_DrawRectangle(x0, y0, x1-1, y1-1, (bgColor==BW_BLACK? BLACK: WHITE),
                      DOT_PIXEL_1X1, DRAW_FILL_FULL);

  // 在另一層（非目標層）把同區塊刷白，避免三色疊底干擾
  Paint_SelectImage(toRedLayer ? BlackImage : RYImage);
  Paint_DrawRectangle(x0, y0, x1-1, y1-1, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);

  // 設定字型與顏色，計算置中
  FramebufferGFX* gfx = toRedLayer ? gfxRed : gfxBlack;
  gfx->setFont(font);
  // 單色字（不覆寫背景），背景已由 Paint 填過
  gfx->setTextColor(textColor);

  int16_t bx, by; uint16_t bw, bh;
  gfx->getTextBounds(s.c_str(), 0, 0, &bx, &by, &bw, &bh);
  int tx = x0 + ((x1-x0) - (int)bw)/2;
  int ty = y0 + ((y1-y0) + (int)bh)/2; // baseline
  gfx->setCursor(tx, ty);
  gfx->print(s);
}

inline void display_init() {
  if (DEV_Module_Init()!=0) { while (1) { delay(1000); } }
  EPD_2IN9B_V4_Init();
  EPD_2IN9B_V4_Clear();
  DEV_Delay_ms(200);

  Imagesize = ((PHY_W % 8 == 0) ? (PHY_W / 8) : (PHY_W / 8 + 1)) * PHY_H;
  BlackImage = (UBYTE*)malloc(Imagesize);
  RYImage    = (UBYTE*)malloc(Imagesize);
  if (!BlackImage || !RYImage) { while (1) { delay(1000); } }

  // 使用 Rotate=270，Paint 的座標即為 LOG_W x LOG_H（296x128）
  Paint_NewImage(BlackImage, PHY_W, PHY_H, 270, WHITE);
  Paint_NewImage(RYImage,    PHY_W, PHY_H, 270, WHITE);

  // 兩張影像個別設成 2 色並清白
  Paint_SelectImage(BlackImage); Paint_SetScale(2); Paint_Clear(WHITE);
  Paint_SelectImage(RYImage);    Paint_SetScale(2); Paint_Clear(WHITE);

  // 構建兩個 GFX：黑層與紅層，尺寸依邏輯方向
  gfxBlack = new FramebufferGFX(LOG_W, LOG_H, false);
  gfxRed   = new FramebufferGFX(LOG_W, LOG_H, true);
}

inline void display_deinit() {
  EPD_2IN9B_V4_Sleep();
  if (BlackImage) { free(BlackImage); BlackImage=nullptr; }
  if (RYImage)    { free(RYImage);    RYImage=nullptr; }
}

// ---- 主畫面：Ticker（白底黑字） / Price（整行反色） / Percent（白底黑字） ----
inline void display_draw_three_lines(const char* ticker, float price, float pct, bool /*fullRefresh*/) {
  Bands b = layoutBands();

  // 整張清白（兩層）
  Paint_SelectImage(BlackImage); Paint_Clear(WHITE);
  Paint_SelectImage(RYImage);    Paint_Clear(WHITE);

  // 各 band 先保險白底
  band_to_white(b.x0, b.y1_0, b.x1, b.y1_0 + b.y1_h);
  band_to_white(b.x0, b.y2_0, b.x1, b.y2_0 + b.y2_h);
  band_to_white(b.x0, b.y3_0, b.x1, b.y3_0 + b.y3_h);
  band_to_white(b.x0, b.time_0, b.x1, b.time_0 + b.time_h);

  // Line 1: Ticker —— 白底黑字（黑層）
  gfx_draw_centered(String(ticker), b.x0, b.y1_0, b.x1, b.y1_0 + b.y1_h,
                    &FreeMonoBold18pt7b, BW_BLACK, BW_WHITE, /*toRedLayer=*/false);

  // Line 2: Price —— 整行反色（漲=黑底白字；跌=紅底白字）
  {
    String sp = (price > 0) ? String(price, 2) : String("--");
    bool down = (!isnan(pct) && pct < 0);
    if (down) {
      // 紅底白字（紅層）
      gfx_draw_centered(sp, b.x0, b.y2_0, b.x1, b.y2_0 + b.y2_h,
                        &FreeSansBold24pt7b, BW_WHITE, BW_BLACK, /*toRedLayer=*/true);
    } else {
      // 黑底白字（黑層）
      gfx_draw_centered(sp, b.x0, b.y2_0, b.x1, b.y2_0 + b.y2_h,
                        &FreeSansBold24pt7b, BW_WHITE, BW_BLACK, /*toRedLayer=*/false);
    }
  }

  // Line 3: Percent —— 白底黑字（黑層）
  {
    String sp = isnan(pct) ? String("--") : (String(pct>=0?"+":"") + String(pct,2) + "%");
    gfx_draw_centered(sp, b.x0, b.y3_0, b.x1, b.y3_0 + b.y3_h,
                      &FreeMonoBold12pt7b, BW_BLACK, BW_WHITE, /*toRedLayer=*/false);
  }
}

// ---- 時間條（白底黑字，黑層）----
inline void display_draw_time_bar(const String& leftTP,
                                  const String& midNY,
                                  const String& rightCNT) {
  Bands b = layoutBands();
  // 先刷白底
  band_to_white(b.x0, b.time_0, b.x1, b.time_0 + b.time_h);

  // 切三段置中
  int seg = LOG_W / 3;
  gfx_draw_centered(leftTP,   b.x0,        b.time_0, b.x0+seg,     b.time_0+b.time_h,
                    &FreeMonoBold9pt7b, BW_BLACK, BW_WHITE, false);
  gfx_draw_centered(midNY,    b.x0+seg,    b.time_0, b.x0+2*seg,   b.time_0+b.time_h,
                    &FreeMonoBold9pt7b, BW_BLACK, BW_WHITE, false);
  gfx_draw_centered(rightCNT, b.x0+2*seg,  b.time_0, b.x1,         b.time_0+b.time_h,
                    &FreeMonoBold9pt7b, BW_BLACK, BW_WHITE, false);
}

// ---- 送出顯示（整屏）----
inline void display_present() {
  EPD_2IN9B_V4_Init();
  EPD_2IN9B_V4_Display(BlackImage, RYImage);
  DEV_Delay_ms(100);
}
