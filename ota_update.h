#pragma once

#include "version.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#else
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#endif
#include <WiFiClientSecure.h>

#ifndef OTA_ENABLED
#define OTA_ENABLED 1
#endif

#ifndef OTA_VERSION_URL
#define OTA_VERSION_URL "https://api.github.com/repos/tzuchen/paper-stock-ticker-color-font/releases/latest"
#endif

#if defined(ESP8266)
#define OTA_BOARD_NAME "esp8266"
#ifndef OTA_BINARY_URL
#define OTA_BINARY_URL "https://github.com/tzuchen/paper-stock-ticker-color-font/releases/latest/download/paper-stock-ticker-color-font-esp8266.bin"
#endif
#else
#define OTA_BOARD_NAME "esp32"
#ifndef OTA_BINARY_URL
#define OTA_BINARY_URL "https://github.com/tzuchen/paper-stock-ticker-color-font/releases/latest/download/paper-stock-ticker-color-font-esp32.bin"
#endif
#endif

#ifndef OTA_USER_AGENT
#define OTA_USER_AGENT "paper-stock-ticker-color-font/" FIRMWARE_VERSION " (" OTA_BOARD_NAME ")"
#endif

namespace ota_update {

inline String parseJsonStringField(const String& body, const char* field) {
  String key = String("\"") + field + "\":";
  int pos = body.indexOf(key);
  if (pos < 0) return String();
  pos += key.length();
  while (pos < (int)body.length() && isspace((unsigned char)body[pos])) ++pos;
  if (pos >= (int)body.length() || body[pos] != '"') return String();
  ++pos;

  String out;
  bool escaped = false;
  for (; pos < (int)body.length(); ++pos) {
    char c = body[pos];
    if (escaped) {
      out += c;
      escaped = false;
      continue;
    }
    if (c == '\\') {
      escaped = true;
      continue;
    }
    if (c == '"') break;
    out += c;
  }
  return out;
}

inline String latestVersion() {
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(15000);

  HTTPClient http;
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  if (!http.begin(client, OTA_VERSION_URL)) {
    Serial.println("[OTA] version check begin failed");
    return String();
  }

  http.addHeader("User-Agent", OTA_USER_AGENT);
  http.addHeader("Accept", "application/vnd.github+json");
  int code = http.GET();
  Serial.printf("[OTA] version check HTTP %d\n", code);
  if (code != HTTP_CODE_OK) {
    http.end();
    return String();
  }

  String body = http.getString();
  http.end();

  String tag = parseJsonStringField(body, "tag_name");
  tag.trim();
  if (tag.length() == 0) Serial.println("[OTA] latest release has no tag_name");
  return tag;
}

inline bool parseSemver(const String& version, int out[3]) {
  String v = version;
  v.trim();
  if (v.startsWith("v") || v.startsWith("V")) v.remove(0, 1);

  int start = 0;
  for (int i = 0; i < 3; ++i) {
    int dot = (i < 2) ? v.indexOf('.', start) : v.length();
    if (dot < 0 || dot == start) return false;
    String part = v.substring(start, dot);
    for (unsigned int j = 0; j < part.length(); ++j) {
      if (!isdigit((unsigned char)part[j])) return false;
    }
    out[i] = part.toInt();
    start = dot + 1;
  }
  return start == (int)v.length() + 1;
}

inline bool isNewerVersion(const String& latest) {
  int remote[3] = {0, 0, 0};
  int current[3] = {0, 0, 0};
  if (!parseSemver(latest, remote) || !parseSemver(FIRMWARE_VERSION, current)) {
    Serial.printf("[OTA] unsupported version format: latest=%s current=%s\n",
                  latest.c_str(), FIRMWARE_VERSION);
    return false;
  }

  for (int i = 0; i < 3; ++i) {
    if (remote[i] > current[i]) return true;
    if (remote[i] < current[i]) return false;
  }
  return false;
}

inline bool checkAndUpdate() {
#if !OTA_ENABLED
  Serial.println("[OTA] disabled");
  return false;
#else
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[OTA] skipped: no Wi-Fi");
    return false;
  }

  String latest = latestVersion();
  if (!isNewerVersion(latest)) {
    Serial.printf("[OTA] current firmware is up to date: %s\n", FIRMWARE_VERSION);
    return false;
  }

  Serial.printf("[OTA] updating %s -> %s from %s\n",
                FIRMWARE_VERSION, latest.c_str(), OTA_BINARY_URL);

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(30000);

#if defined(ESP8266)
  ESPhttpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, OTA_BINARY_URL);
#else
  httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  t_httpUpdate_return ret = httpUpdate.update(client, OTA_BINARY_URL);
#endif

  switch (ret) {
    case HTTP_UPDATE_FAILED:
#if defined(ESP8266)
      Serial.printf("[OTA] update failed (%d): %s\n",
                    ESPhttpUpdate.getLastError(),
                    ESPhttpUpdate.getLastErrorString().c_str());
#else
      Serial.printf("[OTA] update failed (%d): %s\n",
                    httpUpdate.getLastError(),
                    httpUpdate.getLastErrorString().c_str());
#endif
      return false;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("[OTA] no update available");
      return false;
    case HTTP_UPDATE_OK:
      Serial.println("[OTA] update ok; rebooting");
      return true;
  }
  return false;
#endif
}

} // namespace ota_update
