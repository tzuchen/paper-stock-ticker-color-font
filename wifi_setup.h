#pragma once
#include <WiFi.h>

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
