#include "main.h"
#include "tim.h"
#include "gpio.h"
#include "smg.h"
#include "led.h"
#define LO 10 //低音
#define Md 11 //中音
#define Hi 12 //高音
/*

通过改变TIM2的预分频值（Prescaler）调整PWM频率
不同音区+不同按键对应特定预分频值
占空比固定为50%（发声时）
频率计算公式：f = TIM2_CLK / (Prescaler * Period)

显示逻辑
数码管：
无按键时：显示当前音区（L/N/H）
有按键时：显示当前音符（1-7）
LED：按键按下时点亮对应LED

硬件连接
功能	      GPIO	  外设
琴键1-7	     PA1-7	  按键
音区切换	   PA8-10	  按键
数码管段选	 PB0-7	  7段数码管
琴键指示灯	 PB9-15	  LED
蜂鸣器	     TIM2_CH1	PWM输出

*/
// 数码管显示函数声明
void DisplayDigit(uint8_t digit);
// 数码管段码表 (0-9, L, N, H)
uint8_t smg_num[] = {
    0x3f, // 0
    0x06, // 1
    0x5b, // 2
    0x4f, // 3
    0x66, // 4
    0x6d, // 5
    0x7d, // 6
    0x07, // 7
    0x7f, // 8
    0x6f, // 9
    0x38, // L (低音)
    0x37, // N (中音)
    0x76  // H (高音)
};

uint8_t KEY = 0;// 当前按下的琴键 (1-7)
uint8_t KEY1 = 10;// // 当前音区 (默认低音)显示L
void SystemClock_Config(void);

/*******************************
 * 蜂鸣器控制函数
 * 功能：
 * 1. 根据按键状态控制PWM输出
 * 2. 设置不同音区的预分频值
 * 3. 更新数码管显示
 *******************************/
void speaker()
{
	// 控制PWM输出（有按键时50%占空比，无按键时关闭）
    if (KEY)
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, 50);// 开启声音
    else
        __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, 0);// 关闭声音
// 根据音区和按键设置预分频值（改变频率）
    switch (KEY1)
    {
    case LO:// 低音区
        switch (KEY)
        {
        case 1:
            __HAL_TIM_SET_PRESCALER(&htim2, 302);
            break;
/*
 时钟使用8MHz HSE 配置，下面是 302 这个预分频值的计算原理：

1. 时钟配置分析（SystemClock_Config）
RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
RCC_OscInitStruct.HSEState = RCC_HSE_ON;  // 外部 8MHz 晶振
RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;  // 8MHz × 2 = 16MHz

RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;  // 系统时钟 = 16MHz
RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;         // AHB = 16MHz/2 = 8MHz
RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;          // APB1 = 8MHz/2 = 4MHz
2. TIM2 时钟源计算
STM32 定时器时钟规则：

APB1 预分频 = 2 (≠1)

∴ TIM2 时钟 = APB1 时钟 × 2 = 4MHz × 2 = 8MHz

TIM2_CLK = 8,000,000 Hz

3. PWM 频率计算公式
f_pwm = TIM2_CLK / [(PSC + 1) × (ARR + 1)]
4. 目标频率：中央 C (C4)
标准频率：261.63 Hz
代码里低音区 KEY=1 对应此频率

5. 302 的计算过程
在定时器配置中：
htim2.Init.Period = 100;  // ARR = 100
需要计算 PSC 值：
PSC = (TIM2_CLK / (f_target × (ARR + 1))) - 1
    = (8,000,000 / (261.63 × 101)) - 1
    = (8,000,000 / 26,424.63) - 1
    = 302.82 - 1
    = 301.82
		
*/
        case 2:
            __HAL_TIM_SET_PRESCALER(&htim2, 269);
            break;
        case 3:
             __HAL_TIM_SET_PRESCALER(&htim2, 240);
            break;
        case 4:
            __HAL_TIM_SET_PRESCALER(&htim2, 227);
            break;
        case 5:
            __HAL_TIM_SET_PRESCALER(&htim2, 202);
            break;
        case 6:
            __HAL_TIM_SET_PRESCALER(&htim2, 180);
            break;
        case 7:
            __HAL_TIM_SET_PRESCALER(&htim2, 160);
            break;
        }
        break;

    case Md: // 中音区
        switch (KEY)
        {
        case 1:
            __HAL_TIM_SET_PRESCALER(&htim2, 152);
            break;
        case 2:
            __HAL_TIM_SET_PRESCALER(&htim2, 136);
            break;
        case 3:
            __HAL_TIM_SET_PRESCALER(&htim2, 121);
            break;
        case 4:
            __HAL_TIM_SET_PRESCALER(&htim2, 114);
            break;
        case 5:
            __HAL_TIM_SET_PRESCALER(&htim2, 102);
            break;
        case 6:
            __HAL_TIM_SET_PRESCALER(&htim2, 90);
            break;
        case 7:
            __HAL_TIM_SET_PRESCALER(&htim2, 80);
            break;
        }
        break;

    case Hi:// 高音区
        switch (KEY)
        {
        case 1:
            __HAL_TIM_SET_PRESCALER(&htim2, 76);
            break;
        case 2:
            __HAL_TIM_SET_PRESCALER(&htim2, 68);
            break;
        case 3:
            __HAL_TIM_SET_PRESCALER(&htim2, 60);
            break;
        case 4:
            __HAL_TIM_SET_PRESCALER(&htim2, 57);
            break;
        case 5:
            __HAL_TIM_SET_PRESCALER(&htim2, 51);
            break;
        case 6:
            __HAL_TIM_SET_PRESCALER(&htim2, 45);
            break;
        case 7:
            __HAL_TIM_SET_PRESCALER(&htim2, 40);
            break;
        }
        break;
    }
    uint8_t dat[2];
    dat[0] = smg_num[KEY];
    DisplayDigit(dat[0]); 
}
/*******************************
 * 数码管显示函数
 * 功能：通过GPIOB0-7控制数码管各段
 * 参数：digit - 要显示的段码
 *******************************/
void DisplayDigit(uint8_t digit)
{
    // PB0-PB7
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, (digit & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, (digit & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, (digit & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, (digit & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, (digit & 0x10) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, (digit & 0x20) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, (digit & 0x40) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, (digit & 0x80) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

int main(void)
{
  uint8_t dat[2];  
	HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();

    HAL_TIM_Base_Start_IT(&htim2);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, 0);
    dat[0] = smg_num[9];

    while (1)
    {
			// 检测7个琴键的按下状态
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET)
        {
            KEY = 1;
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);// 点亮对应LED
            while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET)
                speaker(); // 持续发声
            KEY = 0;
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET); // 关闭LED
            __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, 0);// 停止发声
        }

        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_SET)
        {
            KEY = 2;
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
            while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_SET)
                speaker();
            KEY = 0;
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
            __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, 0);
        }

        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_SET)
        {
            KEY = 3;
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
            while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_SET)
                speaker();
            KEY = 0;
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
            __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, 0);
        }

        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_SET)
        {
            KEY = 4;
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
            while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_SET)
                speaker();
            KEY = 0;
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
            __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, 0);
        }

        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == GPIO_PIN_SET)
        {
            KEY = 5;
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
            while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == GPIO_PIN_SET)
                speaker();
            KEY = 0;
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
            __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, 0);
        }

        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_SET)
        {
            KEY = 6;
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
            while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_SET)
                speaker();
            KEY = 0;
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
            __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, 0);
        }

        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7) == GPIO_PIN_SET)
        {
            KEY = 7;
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
            while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7) == GPIO_PIN_SET)
            
						speaker();
            KEY = 0;
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
            __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, 0);
        }
				
 // 音区切换按钮检测
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) == GPIO_PIN_SET)
        {
            KEY1 = 10;//显示L
            dat[0] = smg_num[9];
        }
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9) == GPIO_PIN_SET)
        {
            KEY1 = 11;//显示N
            dat[0] = smg_num[9];
        }
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_10) == GPIO_PIN_SET)
        {
            KEY1 = 12;//显示H
            dat[0] = smg_num[9];
        }

        DisplayDigit(smg_num[KEY1 ]);
    }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}


