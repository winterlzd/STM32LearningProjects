#include "main.h"
#include "OLED_Font.h"
#include "OLED.h"

/*
 * 引脚配置：
 * SCL -> PB6 / PB8 / PB10
 * SDA -> PB7 / PB9 / PB11
 * 全引脚同步驱动：无论插在 I2C1(PB6/PB7)、I2C1_Remap(PB8/PB9) 还是 I2C2(PB10/PB11)，均可直接点亮！
 */
#define OLED_SCL_PINS       (GPIO_PIN_6 | GPIO_PIN_8 | GPIO_PIN_10)
#define OLED_SDA_PINS       (GPIO_PIN_7 | GPIO_PIN_9 | GPIO_PIN_11)
#define OLED_PORT           GPIOB

#define OLED_W_SCL(x)       HAL_GPIO_WritePin(OLED_PORT, OLED_SCL_PINS, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define OLED_W_SDA(x)       HAL_GPIO_WritePin(OLED_PORT, OLED_SDA_PINS, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)

/* 微秒级 I2C 延时，确保时钟高低电平宽度满足要求 */
static void OLED_I2C_Delay(void)
{
    for (volatile uint32_t i = 0; i < 30; i++);
}

/**
  * @brief  I2C开始
  */
void OLED_I2C_Start(void)
{
    OLED_W_SDA(1);
    OLED_W_SCL(1);
    OLED_I2C_Delay();
    OLED_W_SDA(0);
    OLED_I2C_Delay();
    OLED_W_SCL(0);
    OLED_I2C_Delay();
}

/**
  * @brief  I2C停止
  */
void OLED_I2C_Stop(void)
{
    OLED_W_SDA(0);
    OLED_I2C_Delay();
    OLED_W_SCL(1);
    OLED_I2C_Delay();
    OLED_W_SDA(1);
    OLED_I2C_Delay();
}

/* 引脚初始化 */
void OLED_I2C_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    // 关闭硬件 I2C 外设以释放 PB6/PB7/PB10/PB11 给 GPIO 模拟
    __HAL_RCC_I2C1_CLK_DISABLE();
    __HAL_RCC_I2C2_CLK_DISABLE();
    
    // 先彻底 DeInit 所有可能被 HAL I2C 占用的引脚，清除 AF 模式残留
    HAL_GPIO_DeInit(OLED_PORT, OLED_SCL_PINS | OLED_SDA_PINS);
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = OLED_SCL_PINS | OLED_SDA_PINS;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;   // 推挽输出强驱动
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(OLED_PORT, &GPIO_InitStruct);
    
    // I2C 总线死锁恢复序列 (发送 9 个 SCL 时钟释放可能被锁死的从机)
    OLED_W_SDA(1);
    for (uint8_t i = 0; i < 9; i++)
    {
        OLED_W_SCL(1);
        OLED_I2C_Delay();
        OLED_W_SCL(0);
        OLED_I2C_Delay();
    }
    OLED_I2C_Stop();
    
    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

/**
  * @brief  I2C发送一个字节
  * @param  Byte 要发送的一个字节
  */
void OLED_I2C_SendByte(uint8_t Byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        if (Byte & (0x80 >> i))
        {
            OLED_W_SDA(1);
        }
        else
        {
            OLED_W_SDA(0);
        }
        OLED_I2C_Delay();
        OLED_W_SCL(1);
        OLED_I2C_Delay();
        OLED_W_SCL(0);
        OLED_I2C_Delay();
    }
    // 第9个时钟周期发送应答时钟
    OLED_W_SDA(1);
    OLED_I2C_Delay();
    OLED_W_SCL(1);
    OLED_I2C_Delay();
    OLED_W_SCL(0);
    OLED_I2C_Delay();
}

/**
  * @brief  OLED写命令（同时向 0x78 和 0x7A 发送，自适应不同地址模块）
  * @param  Command 要写入的命令
  */
void OLED_WriteCommand(uint8_t Command)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78);
    OLED_I2C_SendByte(0x00);
    OLED_I2C_SendByte(Command); 
    OLED_I2C_Stop();

    OLED_I2C_Start();
    OLED_I2C_SendByte(0x7A);
    OLED_I2C_SendByte(0x00);
    OLED_I2C_SendByte(Command); 
    OLED_I2C_Stop();
}

/**
  * @brief  OLED写数据（同时向 0x78 和 0x7A 发送，自适应不同地址模块）
  * @param  Data 要写入的数据
  */
void OLED_WriteData(uint8_t Data)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78);
    OLED_I2C_SendByte(0x40);
    OLED_I2C_SendByte(Data);
    OLED_I2C_Stop();

    OLED_I2C_Start();
    OLED_I2C_SendByte(0x7A);
    OLED_I2C_SendByte(0x40);
    OLED_I2C_SendByte(Data);
    OLED_I2C_Stop();
}

/**
  * @brief  OLED设置光标位置
  * @param  Y 以左上角为原点，向下方向的坐标，范围：0~7
  * @param  X 以左上角为原点，向右方向的坐标，范围：0~127
  */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
    OLED_WriteCommand(0xB0 | (Y & 0x07));			//设置Y位置
    OLED_WriteCommand(0x10 | (((X + 2) & 0xF0) >> 4));	//设置X位置高4位 (兼容SSD1306/SH1106偏移)
    OLED_WriteCommand(0x00 | ((X + 2) & 0x0F));			//设置X位置低4位
}

/**
  * @brief  OLED清屏
  */
void OLED_Clear(void)
{  
    uint8_t i, j;
    for (j = 0; j < 8; j++)
    {
        OLED_SetCursor(j, 0);
        for(i = 0; i < 128; i++)
        {
            OLED_WriteData(0x00);
        }
    }
}

/**
  * @brief  OLED显示一个字符
  * @param  Line 行位置，范围：1~4
  * @param  Column 列位置，范围：1~16
  * @param  Char 要显示的一个字符，范围：ASCII可见字符
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{      	
    uint8_t i;
    OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);		//设置光标位置在上半部分
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i]);			//显示上半部分内容
    }
    OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);	//设置光标位置在下半部分
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);		//显示下半部分内容
    }
}

/**
  * @brief  OLED显示字符串
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  String 要显示的字符串，范围：ASCII可见字符
  */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
    {
        OLED_ShowChar(Line, Column + i, String[i]);
    }
}

/**
  * @brief  OLED次方函数
  */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;
    }
    return Result;
}

/**
  * @brief  OLED显示数字（十进制，正数）
  */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)							
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

/**
  * @brief  OLED显示数字（十进制，带符号数）
  */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
    uint8_t i;
    uint32_t Number1;
    if (Number >= 0)
    {
        OLED_ShowChar(Line, Column, '+');
        Number1 = Number;
    }
    else
    {
        OLED_ShowChar(Line, Column, '-');
        Number1 = -Number;
    }
    for (i = 0; i < Length; i++)							
    {
        OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

/**
  * @brief  OLED显示数字（十六进制，正数）
  */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i, SingleNumber;
    for (i = 0; i < Length; i++)							
    {
        SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
        if (SingleNumber < 10)
        {
            OLED_ShowChar(Line, Column + i, SingleNumber + '0');
        }
        else
        {
            OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
        }
    }
}

/**
  * @brief  OLED显示数字（二进制，正数）
  */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)							
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
    }
}

/**
  * @brief  OLED初始化
  */
void OLED_Init(void)
{
    HAL_Delay(200);             // 上电延时，等待供电和内部复位完成
    
    OLED_I2C_Init();            // 端口初始化
    
    OLED_WriteCommand(0xAE);    // 关闭显示
    
    OLED_WriteCommand(0xD5);    // 设置显示时钟分频比/振荡器频率
    OLED_WriteCommand(0x80);
    
    OLED_WriteCommand(0xA8);    // 设置多路复用率
    OLED_WriteCommand(0x3F);
    
    OLED_WriteCommand(0xD3);    // 设置显示偏移
    OLED_WriteCommand(0x00);
    
    OLED_WriteCommand(0x40);    // 设置显示开始行
    
    OLED_WriteCommand(0xA1);    // 设置左右方向，0xA1正常 0xA0左右反置
    
    OLED_WriteCommand(0xC8);    // 设置上下方向，0xC8正常 0xC0上下反置

    OLED_WriteCommand(0xDA);    // 设置COM引脚硬件配置
    OLED_WriteCommand(0x12);
    
    OLED_WriteCommand(0x81);    // 设置对比度控制
    OLED_WriteCommand(0xCF);

    OLED_WriteCommand(0xD9);    // 设置预充电周期
    OLED_WriteCommand(0xF1);

    OLED_WriteCommand(0xDB);    // 设置VCOMH取消选择级别
    OLED_WriteCommand(0x30);

    OLED_WriteCommand(0x20);    // 设置内存寻址模式 (页寻址)
    OLED_WriteCommand(0x02);

    OLED_WriteCommand(0xA4);    // 设置整个显示打开/关闭

    OLED_WriteCommand(0xA6);    // 设置正常/倒转显示

    OLED_WriteCommand(0x8D);    // SSD1306 设置充电泵 (必须开启，否则无高压显示不亮)
    OLED_WriteCommand(0x14);

    OLED_WriteCommand(0xAD);    // SH1106 设置DC-DC充电泵开启
    OLED_WriteCommand(0x8B);

    OLED_WriteCommand(0xAF);    // 开启显示
        
    OLED_Clear();               // OLED清屏
}
