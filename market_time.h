#pragma once
#include <Arduino.h>
#include <time.h>

// 預設設成美東（含夏令時間）以便交易所計算
inline void markettime_setup_tz() {
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0/2", 1);
  tzset();
}

// 等待 NTP 成功（回傳 true=已同步）
inline bool markettime_wait_synced(unsigned long timeout_ms = 15000) {
  unsigned long t0 = millis();
  time_t now = time(nullptr);
  while ((now < 1600000000) && (millis() - t0 < timeout_ms)) { // 2017-09-13 12:26:40
    delay(200);
    now = time(nullptr);
  }
  return now >= 1600000000;
}

// 將目前時間用指定時區格式成 "HH:MM"
inline String fmt_hhmm_in_tz(const char* tzspec) {
  time_t now = time(nullptr);
  if (now < 1600000000) return String("--:--"); // 未同步

  // 保存現有 TZ，暫時切換到指定 TZ
  const char* oldtz = getenv("TZ");
  if (tzspec && *tzspec) {
    setenv("TZ", tzspec, 1);
    tzset();
  }

  struct tm lt;
  localtime_r(&now, &lt);
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", lt.tm_hour, lt.tm_min);

  // 還原 TZ（回到預設美東，供交易邏輯使用）
  if (tzspec && *tzspec) {
    if (oldtz) setenv("TZ", oldtz, 1);
    else unsetenv("TZ");
    tzset();
  }
  return String(buf);
}

// 回傳距離「開/收盤」的分鐘數；open=true 表示當下是開盤中（美股常規時段）
inline int market_minutes_to_openclose(bool& open) {
  // 要以美東時間判斷
  setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0/2", 1);
  tzset();

  time_t now = time(nullptr);
  if (now < 1600000000) { open=false; return -1; } // 尚未 NTP
  struct tm lt; localtime_r(&now, &lt);

  int w = lt.tm_wday;   // 0=Sun ... 6=Sat
  int H = lt.tm_hour;
  int M = lt.tm_min;
  int cur = H*60 + M;
  int open_min = 9*60 + 30;  // 09:30
  int close_min= 16*60;      // 16:00

  // 週末：統一算到週一 09:30
  if (w == 0 || w == 6) {
    int days_ahead = (w==6)?2:1; // Sat->Mon(+2), Sun->Mon(+1)
    lt.tm_mday += days_ahead;
    lt.tm_hour = 9; lt.tm_min = 30; lt.tm_sec = 0;
    time_t t = mktime(&lt);
    open=false;
    return (int)((t - now)/60);
  }

  if (cur < open_min) {
    // 尚未開盤 → 距離開盤
    lt.tm_hour = 9; lt.tm_min = 30; lt.tm_sec = 0;
    time_t t = mktime(&lt);
    open=false;
    return (int)((t - now)/60);
  } else if (cur >= open_min && cur < close_min) {
    // 交易中 → 距離收盤
    lt.tm_hour = 16; lt.tm_min = 0; lt.tm_sec = 0;
    time_t t = mktime(&lt);
    open=true;
    return (int)((t - now)/60);
  } else {
    // 已收盤 → 到隔日（若週五則到下週一）開盤
    if (w == 5) lt.tm_mday += 3; // Fri -> Mon
    else        lt.tm_mday += 1; // 其他 -> 次日
    lt.tm_hour = 9; lt.tm_min = 30; lt.tm_sec = 0;
    time_t t = mktime(&lt);
    open=false;
    return (int)((t - now)/60);
  }
}

// 分鐘轉 "HH:MM"
inline String market_fmt_hhmm(int minutes) {
  if (minutes < 0) return String("--:--");
  int h = minutes / 60, m = minutes % 60;
  char buf[6]; snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
  return String(buf);
}
