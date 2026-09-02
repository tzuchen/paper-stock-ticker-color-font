# OTA firmware updates

The firmware checks GitHub Releases after Wi-Fi connects. If the latest `vX.Y.Z` release tag is greater than `FIRMWARE_VERSION` in `version.h`, the device downloads the board-specific binary from the latest release and reboots into it.

## Release flow

1. Update `FIRMWARE_VERSION` in `version.h`, for example `v1.0.3`.
2. Commit and push the change.
3. Create and push a matching tag:

```sh
git tag v1.0.3
git push origin main v1.0.3
```

GitHub Actions will build and attach:

- `paper-stock-ticker-color-font-esp8266.bin`
- `paper-stock-ticker-color-font-esp32.bin`

The device reads:

- version: `https://api.github.com/repos/tzuchen/paper-stock-ticker-color-font/releases/latest`
- ESP8266 firmware: `https://github.com/tzuchen/paper-stock-ticker-color-font/releases/latest/download/paper-stock-ticker-color-font-esp8266.bin`
- ESP32 firmware: `https://github.com/tzuchen/paper-stock-ticker-color-font/releases/latest/download/paper-stock-ticker-color-font-esp32.bin`

## Notes

- Use `vX.Y.Z` tags. The device updates only when the latest release tag is greater than `FIRMWARE_VERSION`.
- GitHub Release assets must be public to devices unless you add an authenticated update server.
- TLS certificate validation is disabled with `WiFiClientSecure::setInsecure()` to keep ESP8266/ESP32 OTA simple. For stricter security, pin a certificate or serve firmware from infrastructure you control.


## Recent release notes

- `v1.0.1`: fixed the ESP8266 display rotation.
- `v1.0.2`: unified the boot status screen across ESP8266 and ESP32.
