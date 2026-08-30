#pragma once
// 薄介面層：ESP32 三色 / ESP8266 黑白
// 函式簽名統一，底下各接 epd_tricolor.h 或 epd_bw.h

#if defined(ESP8266)
  #include "epd_bw.h"
  // alias: epd_bw_* → display_*
  #define display_init()               epd_bw_init()
  #define display_deinit()             epd_bw_deinit()
  #define display_present()            epd_bw_present()
  #define display_draw_three_lines(ticker, price, pct, fullRefresh) \
      epd_bw_draw_three_lines(ticker, price, pct, fullRefresh)
  #define display_draw_time_bar(left, mid, right) \
      epd_bw_draw_time_bar(left, mid, right)
  #define band_to_white(x0, y0, x1, y1) \
      epd_bw_band_to_white(x0, y0, x1, y1)
  #define gfx_draw_centered(s, x0, y0, x1, y1, font, textColor, bgColor) \
      epd_bw_draw_centered(s, x0, y0, x1, y1, font, textColor, bgColor)
  // 常數
  #define LOG_W  296
  #define LOG_H  128
  #define BW_BLACK  GxEPD_BLACK
  #define BW_WHITE  GxEPD_WHITE
#else
  #include "epd_tricolor.h"
#endif
