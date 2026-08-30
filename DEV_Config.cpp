#include "DEV_Config.h"
#include <SPI.h>

#ifndef Debug
#define Debug(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#endif

UBYTE DEV_Module_Init(void) {
  pinMode(EPD_SCK_PIN, OUTPUT);
  pinMode(EPD_MOSI_PIN, OUTPUT);
  pinMode(EPD_CS_PIN, OUTPUT);
  pinMode(EPD_DC_PIN, OUTPUT);
  pinMode(EPD_RST_PIN, OUTPUT);
  pinMode(EPD_BUSY_PIN, INPUT);
  digitalWrite(EPD_CS_PIN, HIGH);
  digitalWrite(EPD_RST_PIN, HIGH);
  SPI.begin();
  SPI.setFrequency(1000000);
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  return 0;
}

void DEV_GPIO_Init(void) {
  pinMode(EPD_SCK_PIN, OUTPUT);
  pinMode(EPD_MOSI_PIN, OUTPUT);
  pinMode(EPD_CS_PIN, OUTPUT);
  pinMode(EPD_DC_PIN, OUTPUT);
  pinMode(EPD_RST_PIN, OUTPUT);
  pinMode(EPD_BUSY_PIN, INPUT);
}

void DEV_SPI_Init(void) {
  SPI.begin();
  SPI.setFrequency(1000000);
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
}

void GPIO_Mode(UWORD GPIO_Pin, UWORD Mode) {
  pinMode(GPIO_Pin, Mode);
}

void DEV_SPI_WriteByte(UBYTE data) {
  SPI.transfer(data);
}

void DEV_SPI_SendByte(UBYTE data) {
  SPI.transfer(data);
}

UBYTE DEV_SPI_ReadByte() {
  return SPI.transfer(0x00);
}

void DEV_SPI_Write_nByte(UBYTE *pData, UDOUBLE len) {
  for (UDOUBLE i = 0; i < len; i++) {
    SPI.transfer(pData[i]);
  }
}

void DEV_Module_Exit(void) {
  SPI.endTransaction();
  SPI.end();
}
