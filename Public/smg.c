#include "smg.h"
#include "delay.h"

//74HC595驱动
//数码管显示
//duan:显示的段码
//wei:要显示的数码管编号 0-7(共8个数码管)
void LED_Write_Data(uint8_t duan,uint8_t wei)
{
	uint8_t i;
	for( i=0;i<8;i++)//先送段
	{
		LED_DS=(duan>>i)&0x01;
		LED_SCK=0;
		delay_us(5);
		LED_SCK=1;
	}
    //LED_Wei(wei);//后选中位
}
//74HC595驱动
//数码管刷新显示
void LED_Refresh(void)
{
	LED_LCLK=1;
	delay_us(5);
	LED_LCLK=0;
}

