#include "main.h"
#include "tim.h"
#include "gpio.h"
#include "OLED.h"

/*
  硬件连接：
  PA0      -> 蜂鸣器 (TIM2_CH1)
  PA1 ~ PA7 -> 7个按键 (对应音符 1-7)
  PB8      -> OLED SCL
  PB9      -> OLED SDA
*/

uint8_t KEY = 0; // 当前按下的琴键 (1-7), 0表示没按

// 定义音符对应的唱名字符串
char *NOTE_NAMES[] = {
    " ",
    "1 Do ",
    "2 Re ",
    "3 Mi ",
    "4 Fa ",
    "5 Sol",
    "6 La ",
    "7 Si "
};

void SystemClock_Config(void);

// speaker 函数已删除，逻辑移入主循环

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();

    // 1. 初始化 OLED
    OLED_Init(); 
    
    // 2. 修复引脚悬空问题 (开启下拉电阻)
    __HAL_RCC_GPIOA_CLK_ENABLE(); 
    GPIO_InitTypeDef GPIO_FixStruct = {0};
    GPIO_FixStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 |
                         GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_FixStruct.Mode = GPIO_MODE_INPUT;      // 输入模式
    GPIO_FixStruct.Pull = GPIO_PULLDOWN;        // 下拉
    HAL_GPIO_Init(GPIOA, &GPIO_FixStruct);
    
    // 3. 初始屏幕显示
    OLED_ShowString(1, 1, "STM32 Piano");
    OLED_ShowString(3, 1, "WAITING...");

    while (1)
    {
        KEY = 0; // 每次循环先重置按键状态

        // 轮询检测按键 PA1 - PA7
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET) KEY = 1;
        else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_SET) KEY = 2;
        else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_SET) KEY = 3;
        else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_SET) KEY = 4;
        else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == GPIO_PIN_SET) KEY = 5;
        else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_SET) KEY = 6;
        else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7) == GPIO_PIN_SET) KEY = 7;

        if (KEY > 0)
        {
            // 【发声模式】: 必须把 PA0 重新配置为 "复用推挽 (AF_PP)"
            GPIO_InitTypeDef GPIO_InitStruct = {0};
            GPIO_InitStruct.Pin = GPIO_PIN_0;
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP; // 变身为定时器引脚
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
            HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

            // 1. 启动 PWM
            HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
            __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, 50); // 50% 占空比

            // 2. 显示
            OLED_ShowString(3, 1, "Playing:      ");
            OLED_ShowString(3, 10, NOTE_NAMES[KEY]);

            // 3. 设置频率
            switch (KEY)
            {
                case 1: __HAL_TIM_SET_PRESCALER(&htim2, 152); break;
                case 2: __HAL_TIM_SET_PRESCALER(&htim2, 136); break;
                case 3: __HAL_TIM_SET_PRESCALER(&htim2, 121); break;
                case 4: __HAL_TIM_SET_PRESCALER(&htim2, 114); break;
                case 5: __HAL_TIM_SET_PRESCALER(&htim2, 102); break;
                case 6: __HAL_TIM_SET_PRESCALER(&htim2, 90);  break;
                case 7: __HAL_TIM_SET_PRESCALER(&htim2, 80);  break;
            }
            HAL_Delay(10);
        }
        else
        {
            // ========================================================
            // 强制把 PA0 变成 "普通推挽输出 (OUTPUT_PP)"
            // 并且输出 高电平
            // ========================================================
            HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1); // 先停定时器

            GPIO_InitTypeDef GPIO_InitStruct = {0};
            GPIO_InitStruct.Pin = GPIO_PIN_0;
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // 变身为普通GPIO
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
            HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

            // ！！！关键一击：强制输出 1 (3.3V)！！！
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET); 
            
            OLED_ShowString(3, 1, "WAITING...    ");
        }
    }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  // 1. 配置振荡器 (HSE = 8MHz, PLL = 8 * 2 = 16MHz)
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2; // 锁定为 x2 (16MHz)
  
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  // 2. 配置时钟树
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
      // 只要不做错事，这里永远不会被执行
  }
}
