#pragma once

// 面板尺寸（黑白 2.9" SSD1680 / GxEPD2_290_T94_V2）
#define EPD_W 296
#define EPD_H 128

// WiFi
static const char* WIFI_SSID = "Sputnik";
static const char* WIFI_PSK  = "qwerasdf";

// 更新節奏
static const unsigned long POLL_MS         = 60 * 1000UL;      // 每 60 秒抓價
static const unsigned long FULL_REFRESH_MS = 10 * 60 * 1000UL; // 每 10 分鐘全刷
static const unsigned long TIME_STRIP_MS   = 60 * 1000UL;      // 每 60 秒更新時間條

// 版面（橫向）
static const int MARGIN = 8;   // 上下邊界
static const int GAP    = 4;   // 行距（4 px）
static const int L1_H   = 24;  // 第一行：Ticker
static const int L2_H   = 48;  // 第二行：Price
static const int L3_H   = 24;  // 第三行：Percent

// 右下時間條（不與內容重疊）
static const int TIME_STRIP_H = 16;