#include "main.h"
#include "tim.h"
#include "gpio.h"
#include "OLED.h"

/*
  硬件连接：
  PA0       -> 蜂鸣器 (TIM2_CH1)
  PA1 ~ PA7 -> 7个琴键 (对应音符 1-7)
  PB0       -> 切换音域按钮 (循环切换：中->高->低->中)
  PB8       -> OLED SCL
  PB9       -> OLED SDA
*/

uint8_t KEY = 0; // 当前按下的琴键 (1-7)

// 当前八度模式：0=Low(低音), 1=Mid(中音), 2=High(高音)
// 默认设为 1 (中音)
int8_t current_octave = 1; 

// 显示字符
char *NOTE_NAMES[] = {" ", "1 Do ", "2 Re ", "3 Mi ", "4 Fa ", "5 Sol", "6 La ", "7 Si "};
char *OCTAVE_NAMES[] = {"Low ", "Mid ", "High"};

/* 频率预分频值表 (PSC)
   行: 0=Low, 1=Mid, 2=High
   列: 0=空, 1=Do...7=Si
*/
uint16_t PSC_TABLE[3][8] = {
    {0, 304, 272, 242, 228, 204, 180, 160}, // Low
    {0, 152, 136, 121, 114, 102, 90,  80 }, // Mid
    {0, 76,  68,  60,  57,  51,  45,  40 }  // High
};

void SystemClock_Config(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();
    OLED_Init(); 

    // ========================================================
    // 1. 硬件配置
    // ========================================================
    __HAL_RCC_GPIOA_CLK_ENABLE(); 
    __HAL_RCC_GPIOB_CLK_ENABLE(); 

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 配置琴键 (PA1-PA7)
    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|
                          GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN; // 下拉
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 配置切换按钮 (PB0) - 只需要这一个
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN; // 下拉
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // 初始显示
    OLED_ShowString(1, 1, "STM32 Piano");
    OLED_ShowString(2, 1, "Mode: Mid "); 
    OLED_ShowString(3, 1, "WAITING...");

    while (1)
    {
        // ==========================
        // 1. 单键循环切换逻辑 (PB0)
        // ==========================
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_SET)
        {
            HAL_Delay(20); // 先消抖
            if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_SET) // 确认真的按下了
            {
                // 切换逻辑： 1(Mid) -> 2(High) -> 0(Low) -> 1(Mid)
                current_octave++;
                if (current_octave > 2) 
                {
                    current_octave = 0; // 超过高音，回到低音
                }

                // 更新屏幕
                OLED_ShowString(2, 1, "Mode:      "); 
                OLED_ShowString(2, 1, "Mode: ");
                OLED_ShowString(2, 7, OCTAVE_NAMES[current_octave]);
                
                // 等待按键松开（防止一次按下跳好几档）
                // 如果不想等待松手，可以用 HAL_Delay(300); 代替下面这行
                while(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_SET); 
            }
        }

        // ==========================
        // 2. 检测琴键 (PA1-PA7)
        // ==========================
        KEY = 0; 
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET) KEY = 1;
        else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_SET) KEY = 2;
        else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_SET) KEY = 3;
        else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_SET) KEY = 4;
        else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == GPIO_PIN_SET) KEY = 5;
        else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_SET) KEY = 6;
        else if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7) == GPIO_PIN_SET) KEY = 7;

        if (KEY > 0)
        {
            // === 发声模式 ===
            // 恢复 PA0 为定时器
            GPIO_InitStruct.Pin = GPIO_PIN_0;
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP; 
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
            HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

            // 查表设置频率
            __HAL_TIM_SET_PRESCALER(&htim2, PSC_TABLE[current_octave][KEY]);

            // 启动 PWM
            HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
            __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, 50);

            // 更新显示
            OLED_ShowString(3, 1, "Playing:      ");
            OLED_ShowString(3, 10, NOTE_NAMES[KEY]);
            
            HAL_Delay(10);
        }
        else
        {
            // === 静音模式 (低电平触发专用) ===
            HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1); 

            // 强制 PA0 输出高电平
            GPIO_InitStruct.Pin = GPIO_PIN_0;
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; 
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
            HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET); 
            
            OLED_ShowString(3, 1, "WAITING...    ");
        }
    }
}

// 时钟配置 (保持 16MHz 不变)
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}
