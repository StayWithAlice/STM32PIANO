#ifndef _LED_SMG_H
#define _LED_SMG_H
#include "sys.h"

////74HC595操作线
#define LED_DS		PBout(1) //数据线
#define LED_LCLK	PBout(2) //锁存时钟线
#define LED_SCK		PBout(0) //时钟线

void LED_SMG_Init(void);
void LED_Refresh(void);
void LED_Write_Data(uint8_t duan,uint8_t wei);

#endif



