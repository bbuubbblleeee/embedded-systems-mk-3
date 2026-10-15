#include "leds.h"
#include "main.h"

/* См. схему стенда SDK-1.1M (процессорный модуль):
 *   VD6 «РАБОТА» - зеленый светодиод, вывод PD13;
 *   VD7 «РЕЖИМ»  - двухцветный светодиод, включенный между PD14 (YARC)
 *                  и PD15 (YCRA): PD14 = 1, PD15 = 0 - желтый,
 *                  PD14 = 0, PD15 = 1 - красный, одинаковые уровни - погашен.
 * Выводы настраиваются как выходы в MX_GPIO_Init(). */
#define LED_PORT        GPIOD
#define LED_WORK_PIN    GPIO_PIN_13
#define LED_YARC_PIN    GPIO_PIN_14
#define LED_YCRA_PIN    GPIO_PIN_15

void Leds_ToggleWork(void)
{
	HAL_GPIO_TogglePin(LED_PORT, LED_WORK_PIN);
}

void Leds_SetMode(led_mode_t mode)
{
	HAL_GPIO_WritePin(LED_PORT, LED_YARC_PIN, (mode == LED_MODE_YELLOW) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_PORT, LED_YCRA_PIN, (mode == LED_MODE_RED) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
