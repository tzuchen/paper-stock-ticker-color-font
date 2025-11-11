#pragma once
#include <Arduino.h>
#include <time.h>
#include "display_driver.h"
#include "market_time.h"

// ────────────────────────────────────────────────────────────────

// ────────────────────────────────────────────────────────────────
// 股票主內容
// ────────────────────────────────────────────────────────────────
inline void draw_content_frame(const char* ticker, float price, float pct, bool fullRefresh) {
  display_draw_three_lines(ticker, price, pct, fullRefresh);
}
// ────────────────────────────────────────────────────────────────
// 時間條：TP xx:xx   NY xx:xx   TC/TO xx:xx
// ────────────────────────────────────────────────────────────────
inline void draw_time_strip() {
  String tp = fmt_hhmm_in_tz("CST-8");                         
  String ny = fmt_hhmm_in_tz("EST5EDT,M3.2.0/2,M11.1.0/2");     

  bool isOpen = false;
  int mins = market_minutes_to_openclose(isOpen);
  String tc = (mins < 0) ? "--:--" : market_fmt_hhmm(mins);

  // 緊貼標籤 + 固定寬度（7字元：2字母+5字時間）
  char bufL[10], bufM[10], bufR[10];
  snprintf(bufL, sizeof(bufL), "TP%-5s", tp.c_str());   // 例如 "TP09:35"
  snprintf(bufM, sizeof(bufM), "NY%-5s", ny.c_str());   // 例如 "NY21:35"
  snprintf(bufR, sizeof(bufR), "%s%-5s", isOpen ? "TC" : "TO", tc.c_str()); // "TC03:25" or "TO05:15"

  String left  = String(bufL);
  String mid   = String(bufM);
  String right = String(bufR);

  display_draw_time_bar(left, mid, right);
}


