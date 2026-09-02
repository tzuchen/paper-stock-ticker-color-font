#pragma once
#include <Arduino.h>

#include <EEPROM.h>

namespace stock_list {
static const int MAX_TICKERS = 10;
static const int MAX_TICKER_LEN = 11;
static const uint32_t MAGIC = 0x53504b31UL; // "SPK1"

static const char* DEFAULT_TICKERS[] = {"SPY", "AAPL", "TSLA", "AMD", "WMT", "PLTR", "CVS"};
static const int DEFAULT_COUNT = sizeof(DEFAULT_TICKERS) / sizeof(DEFAULT_TICKERS[0]);

struct Stored {
  uint32_t magic;
  uint8_t count;
  char tickers[MAX_TICKERS][MAX_TICKER_LEN + 1];
};

static char customTickers[MAX_TICKERS][MAX_TICKER_LEN + 1];
static int customCount = 0;

inline bool validChar(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '^';
}

inline bool normalizeTicker(String in, char* out, size_t outSize) {
  in.trim();
  if (in.length() == 0 || in.length() >= outSize) return false;

  size_t n = 0;
  for (unsigned int i = 0; i < in.length(); ++i) {
    char c = in[i];
    if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
    if (!validChar(c)) return false;
    out[n++] = c;
  }
  out[n] = '\0';
  return n > 0;
}

inline void applyStored(const Stored& s) {
  if (s.magic != MAGIC || s.count == 0 || s.count > MAX_TICKERS) {
    customCount = 0;
    return;
  }

  char parsed[MAX_TICKERS][MAX_TICKER_LEN + 1] = {};
  for (uint8_t i = 0; i < s.count; ++i) {
    if (s.tickers[i][MAX_TICKER_LEN] != '\0') {
      customCount = 0;
      return;
    }
    if (!normalizeTicker(String(s.tickers[i]), parsed[i], sizeof(parsed[i]))) {
      customCount = 0;
      return;
    }
  }

  for (uint8_t i = 0; i < s.count; ++i) {
    strncpy(customTickers[i], parsed[i], sizeof(customTickers[i]));
    customTickers[i][MAX_TICKER_LEN] = '\0';
  }
  customCount = s.count;
}

static const int EEPROM_SIZE = 512;
static const int EEPROM_OFFSET = 160;

inline void load() {
  Stored s = {};
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(EEPROM_OFFSET, s);
  applyStored(s);
}

inline void saveStored(const Stored& s) {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(EEPROM_OFFSET, s);
  EEPROM.commit();
}

inline int count() {
  return customCount > 0 ? customCount : DEFAULT_COUNT;
}

inline const char* get(int index) {
  const int n = count();
  if (n <= 0) return "SPY";
  int i = index % n;
  if (i < 0) i += n;
  return customCount > 0 ? customTickers[i] : DEFAULT_TICKERS[i];
}

inline String csv() {
  String out;
  const int n = count();
  for (int i = 0; i < n; ++i) {
    if (i) out += ",";
    out += get(i);
  }
  return out;
}

inline bool saveFromCsv(const String& csvText) {
  Stored s = {};
  s.magic = MAGIC;

  int start = 0;
  while (start <= (int)csvText.length()) {
    int comma = csvText.indexOf(',', start);
    if (comma < 0) comma = csvText.length();

    String token = csvText.substring(start, comma);
    char normalized[MAX_TICKER_LEN + 1];
    if (normalizeTicker(token, normalized, sizeof(normalized))) {
      bool duplicate = false;
      for (uint8_t i = 0; i < s.count; ++i) {
        if (strcmp(s.tickers[i], normalized) == 0) {
          duplicate = true;
          break;
        }
      }
      if (!duplicate) {
        if (s.count >= MAX_TICKERS) return false;
        strncpy(s.tickers[s.count], normalized, sizeof(s.tickers[s.count]));
        s.tickers[s.count][MAX_TICKER_LEN] = '\0';
        ++s.count;
      }
    } else if (token.length() > 0) {
      return false;
    }

    start = comma + 1;
    if (comma == (int)csvText.length()) break;
  }

  if (s.count == 0) return false;
  saveStored(s);
  applyStored(s);
  return true;
}

} // namespace stock_list
