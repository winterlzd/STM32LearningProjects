#ifndef __OLED_H
#define __OLED_H

#include <stdint.h>
#include "main.h"

/* 
 * I2C 引脚模式配置:
 * 默认开启【全引脚同步驱动模式】：
 * 同时驱动 PB6 & PB8 作为 SCL，同时驱动 PB7 & PB9 作为 SDA。
 * 无论硬件接在 PB6/PB7 还是 PB8/PB9，都能直接点亮！
 */
#define OLED_ADDRESS        0x78    // OLED 从机地址 (通常为 0x78)

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

#endif
