#pragma once

// 電紙板 SPI pins（給 .ino 用；DEV_Config.h 有自己的 EPD_*_PIN，不要動它）
#if defined(ESP8266)
  // ESP8266：Waveshare 2.9" T5D (Z10) 黑白模組
  // CS=15, DC=0, RST=2, BUSY=4, SCK=14, MOSI=13
  #define EPD_SCK_PIN  14
  #define EPD_MOSI_PIN 13
  #define EPD_CS       15
  #define EPD_RST      2
  #define EPD_DC       0
  #define EPD_BUSY     4
#else
  // ESP32：Waveshare 2.9" 三色 V4（25-pin FFC）
  #define EPD_SCK_PIN  13
  #define EPD_MOSI_PIN 14
  #define EPD_CS       15
  #define EPD_RST      26
  #define EPD_DC       27
  #define EPD_BUSY     25
#endif

// BOOT 鍵
#define BTN_BOOT     0
