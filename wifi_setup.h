#pragma once
#if defined(ESP8266)
// ESP8266 core 的 WiFi.h 其實是 ESP8266WiFi 庫的 alias（同 guard WiFi_h），
// 且此 core 安裝中 src/ 沒有 WiFi.h 實體檔，故直接包含 ESP8266WiFi.h。
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

inline void wifi_connect(const char* ssid, const char* password) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(300);
    Serial.print(".");
  }
  Serial.printf("\nWiFi %s\n", (WiFi.status()==WL_CONNECTED)?"connected":"failed");
}
