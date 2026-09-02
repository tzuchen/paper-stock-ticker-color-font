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

// 實板按鍵。
#if defined(ESP8266)
  // ESP8266 V2.44：中間黑鍵 GPIO0；右側紅鍵 GPIO3/RX。
  // GPIO0 同時是 BOOT/FLASH strap，開機/重置時不要按住。
  #define BTN_PREV     0
  #define BTN_NEXT     3
#else
  // Waveshare ESP32 e-Paper Driver Board：A/B 是面板相容性切換，
  // 可由韌體讀取的板載按鍵是 BOOT/GPIO0。ESP32 以單鍵操作：短按下一檔，長按配網。
  #define BTN_PREV     -1
  #define BTN_NEXT     0
#endif
