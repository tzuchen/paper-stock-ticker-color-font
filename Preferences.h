// ESP8266 用的 Preferences shim。
// 此環境的 ESP8266 core 安裝缺原生 Preferences 庫，這裡用 EEPROM 存一個 int
// 來支援 .ino 用到的 begin() / putInt(key,val) / getInt(key,default)。
// 若未來裝回完整 core，刪掉這個檔案即可（.ino 會改用原生 <Preferences.h>）。
#pragma once
#include <Arduino.h>
#include <EEPROM.h>

class Preferences {
public:
  bool begin(const char* /*namespace*/, bool /*readWrite*/ = true) { return true; }
  void end() {}
  void clear() {}
  void putInt(const char* key, int value) {
    if (key && key[0] == 'c' && key[1] == 'u') {
      EEPROM.put(0, value);
      EEPROM.commit();
    }
  }
  int getInt(const char* /*key*/, int defaultValue) {
    int v = defaultValue;
    EEPROM.get(0, v);
    return v;
  }
};
