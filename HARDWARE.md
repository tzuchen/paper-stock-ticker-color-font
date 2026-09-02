# ESP e-paper ticker hardware notes

## 實測板子

- 立創開源 2.9 吋 SD 墨水屏／天氣墨水屏系列
- PCB 外觀版本標示：V2.44
- 主控：ESP-12F（ESP8266）
- USB-UART：CH340
- 實測 USB 串口：`/dev/ttyUSB0`
- Flash：4 MB

## 按鍵配置（以這一片實板實測為準）

由左至右：

1. 左側黑鍵：硬體 RESET，接 ESP8266 `RST`
2. 中間黑鍵：功能鍵，接 `GPIO0`，按下為 `LOW`，目前功能是上一檔股票
3. 右側紅鍵：功能鍵，實測接 `GPIO3/RX`，按下為 `LOW`，目前功能是下一檔股票

按鍵操作：

- 右側紅鍵短按：下一檔股票
- 右側紅鍵長按約 3 秒：進入手機設定 AP，可設定 Wi-Fi 與股票清單
- 左側黑鍵：僅作硬體 RESET，不由韌體判斷長按時間

注意：雖然部分 V2.41/V2.44 網路資料將按鍵 3 標成 `GPIO5`，這片實板實測是 `GPIO3`；後續韌體以實測結果為準。

## GPIO0 與墨水屏共用

這片板子的 `GPIO0` 同時用作按鍵 2 與墨水屏 DC。韌體必須遵守：

- 傳送墨水屏資料時：將 `GPIO0` 設為 `OUTPUT`
- 墨水屏刷新完成後：將 `GPIO0` 設為 `INPUT_PULLUP`，才能讀按鍵
- 上電或 RESET 瞬間不要按住中間黑鍵，否則 ESP8266 可能進入燒錄模式

ESP8266 對應程式定義在 `pins.h`：

```cpp
#define BTN_PREV 0
#define BTN_NEXT 3
```

ESP8266 黑白螢幕使用 GxEPD2，實機方向設定為 `rotation=3`，邏輯尺寸維持 `296x128`。


## 股票清單設定

右側紅鍵長按約 3 秒會進入手機設定 AP：

- AP 名稱：`SPY-Ticker-Setup`
- AP 密碼：無，這是開放式設定熱點
- 設定頁可同時儲存 Wi-Fi 與股票清單
- 股票代號用逗號分隔，最多 10 個，例如 `SPY,AAPL,TSLA,AMD`
- 儲存後裝置會自動重啟並使用新的清單


## ESP32 按鍵配置

Waveshare ESP32 e-Paper Driver Board 的 A/B 鍵是面板相容性切換，不是韌體使用者按鍵。ESP32 韌體使用 BOOT/GPIO0 作為單鍵操作：

- 短按：下一檔股票
- 長按約 3 秒：進入 Wi-Fi configuration mode

GPIO0 是下載/BOOT strap 腳，上電或重置瞬間不要按住。


## ESP32 顯示腳位

ESP32 三色 V4 面板使用 Waveshare driver，顯示腳位定義在 `DEV_Config.h`：

```cpp
#define EPD_SCK_PIN     13
#define EPD_MOSI_PIN    14
#define EPD_CS_PIN      15
#define EPD_DC_PIN      27
#define EPD_RST_PIN     26
#define EPD_BUSY_PIN    25
```

ESP32 必須用明確 SPI 腳位初始化：`SPI.begin(EPD_SCK_PIN, -1, EPD_MOSI_PIN, EPD_CS_PIN)`。只呼叫 `SPI.begin()` 會使用 ESP32 預設 SPI 腳，app 會正常執行但 e-paper 不刷新。

## Boot 畫面

ESP8266 與 ESP32 使用相同 boot status layout：

- `Firmware vX.Y.Z`
- `Build <date> <time>`
- 狀態文字，例如 `Connecting Wi-Fi...`、`Checking update...`、`Wi-Fi Connected!`

三段文字一次畫在同一張 frame，避免 firmware version 與 build date/time 重疊。
