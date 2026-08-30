// shim：GxEPD2 庫的 src/epd/ 子目錄不在 arduino-cli 偵測器的 include path 上，
// 此檔重導出給 sketch 層的尖號 include 使用。
#pragma once
#include "/home/orin/Arduino/libraries/GxEPD2/src/epd/GxEPD2_290_T5D.h"
