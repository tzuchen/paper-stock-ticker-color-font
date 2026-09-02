// shim：GxEPD2 庫的 src/epd/ 子目錄不在 arduino-cli 偵測器的 include path 上，
// 此檔重導出給 sketch 層的尖號 include 使用。
#pragma once
#if __has_include(<epd/GxEPD2_290_T5D.h>)
#include <epd/GxEPD2_290_T5D.h>
#elif __has_include("/home/orin/Arduino/libraries/GxEPD2/src/epd/GxEPD2_290_T5D.h")
#include "/home/orin/Arduino/libraries/GxEPD2/src/epd/GxEPD2_290_T5D.h"
#else
#error "GxEPD2_290_T5D.h not found; install GxEPD2 and make src/epd available to Arduino CLI"
#endif
