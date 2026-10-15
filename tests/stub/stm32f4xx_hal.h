/* Заглушка HAL для сборки модулей на ПК: перехватывает обмен с OLED по I2C. */
#ifndef STUB_HAL_H
#define STUB_HAL_H
#include <stdint.h>
typedef enum { HAL_OK = 0, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef struct { int dummy; } I2C_HandleTypeDef;
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *h, uint16_t addr, uint16_t mem, uint16_t memsz, uint8_t *data, uint16_t size, uint32_t timeout);
void HAL_Delay(uint32_t ms);
#endif
