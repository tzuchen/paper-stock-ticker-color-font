#pragma once
#include "DEV_Config.h"

#define EPD_2IN9B_V4_WIDTH   128
#define EPD_2IN9B_V4_HEIGHT  296

void EPD_2IN9B_V4_Init(void);
void EPD_2IN9B_V4_Init_Fast(void);
void EPD_2IN9B_V4_Clear(void);
void EPD_2IN9B_V4_Clear_Fast(void);
void EPD_2IN9B_V4_Clear_Black_Fast(void);
void EPD_2IN9B_V4_Clear_Red_Fast(void);
void EPD_2IN9B_V4_Display(const UBYTE *blackimage, const UBYTE *ryimage);
void EPD_2IN9B_V4_Display_Fast(const UBYTE *blackimage, const UBYTE *ryimage);
void EPD_2IN9B_V4_Display_Base(const UBYTE *blackimage, const UBYTE *ryimage);
void EPD_2IN9B_V4_Display_Partial(const UBYTE *Image, UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend);
void EPD_2IN9B_V4_Sleep(void);
void EPD_2IN9B_V4_ReadBusy(void);
