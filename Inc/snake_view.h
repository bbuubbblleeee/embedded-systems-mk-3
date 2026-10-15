#ifndef INC_SNAKE_VIEW_H_
#define INC_SNAKE_VIEW_H_

#include "snake.h"

/* Рисует текущее состояние игры в видеобуфер OLED-дисплея, который
 * находится в ОЗУ микроконтроллера. Функция не обращается к шине I2C:
 * передачу готового кадра на дисплей выполняет oled_UpdateScreen(). */
void SnakeView_Draw(const snake_game_t *game);

#endif /* INC_SNAKE_VIEW_H_ */
