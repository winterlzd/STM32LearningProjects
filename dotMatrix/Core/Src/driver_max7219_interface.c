/* Core/Src/MAX7219/max7219_interface.c */
#include "driver_max7219_interface.h"
#include "main.h"                 // CubeMX 自动生成，包含 hspi1 / huart1 句柄
#include <stdarg.h>

extern SPI_HandleTypeDef hspi1;   // 也可以直接 #include "spi.h"

#define MAX7219_CS_PORT  MAX7219_CS_GPIO_Port    // CubeMX 起 User Label 后自动生成
#define MAX7219_CS_PIN   MAX7219_CS_Pin          //   main.h 里会出现这两个宏

static inline void cs_low (void) { HAL_GPIO_WritePin(MAX7219_CS_PORT, MAX7219_CS_PIN, GPIO_PIN_RESET); }
static inline void cs_high(void) { HAL_GPIO_WritePin(MAX7219_CS_PORT, MAX7219_CS_PIN, GPIO_PIN_SET);   }

uint8_t max7219_interface_spi_init(void)
{
    /* CubeMX 已自动初始化 hspi1，这里没额外事做 */
    return 0;
}

uint8_t max7219_interface_spi_deinit(void)
{
    return (HAL_SPI_DeInit(&hspi1) == HAL_OK) ? 0 : 1;
}

/* libdriver 把所有 16-bit 命令都当成"写命令" */
uint8_t max7219_interface_spi_write_cmd(uint8_t *buf, uint16_t len)
{
    cs_low();
    HAL_StatusTypeDef s = HAL_SPI_Transmit(&hspi1, buf, len, HAL_MAX_DELAY);
    cs_high();
    return (s == HAL_OK) ? 0 : 1;
}

/* reg + 1 字节数据，组合成 2 字节帧发出去（MAX7219 协议一次传 16 bit） */
uint8_t max7219_interface_spi_write(uint8_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t tx[2] = { (uint8_t)(reg & 0x0F), buf[0] };
    cs_low();
    HAL_StatusTypeDef s = HAL_SPI_Transmit(&hspi1, tx, 2, HAL_MAX_DELAY);
    cs_high();
    return (s == HAL_OK) ? 0 : 1;
}

void max7219_interface_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

void max7219_interface_debug_print(const char *const fmt, ...)
{
    char str[128];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(str, sizeof(str), fmt, ap);
    va_end(ap);
}