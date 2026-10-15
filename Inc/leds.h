#ifndef INC_LEDS_H_
#define INC_LEDS_H_

/* Светодиоды на боковой панели стенда SDK-1.1M. */

typedef enum {
	LED_MODE_OFF = 0,
	LED_MODE_YELLOW,
	LED_MODE_RED
} led_mode_t;

/* Переключает зеленый светодиод «РАБОТА». */
void Leds_ToggleWork(void);

/* Задает цвет двухцветного светодиода «РЕЖИМ». */
void Leds_SetMode(led_mode_t mode);

#endif /* INC_LEDS_H_ */
