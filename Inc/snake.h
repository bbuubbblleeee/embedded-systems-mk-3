#ifndef INC_SNAKE_H_
#define INC_SNAKE_H_

#include <stdint.h>

/* Модель игры «Змейка».
 *
 * Модуль не обращается ни к аппаратуре, ни к операционной системе: он только
 * хранит состояние игры и изменяет его по командам игрока (Snake_Command) и
 * по шагам игрового времени (Snake_Step). Откуда приходят команды, как часто
 * делаются шаги и как состояние выводится на экран, решают вызывающие задачи.
 * Благодаря этому логику игры можно проверить отдельно от стенда. */

#define SNAKE_FIELD_W      31    /* ширина игрового поля, клеток */
#define SNAKE_FIELD_H      13    /* высота игрового поля, клеток */
#define SNAKE_MAX_LEN      (SNAKE_FIELD_W * SNAKE_FIELD_H)
#define SNAKE_START_LEN    3     /* длина змейки в начале игры */
#define SNAKE_TURN_BUF     2     /* сколько поворотов можно ввести заранее, за один шаг */

typedef enum {
	SNAKE_DIR_UP = 0,
	SNAKE_DIR_RIGHT,
	SNAKE_DIR_DOWN,
	SNAKE_DIR_LEFT
} snake_dir_t;

/* Команды игрока. Соответствие клавишам задается вне модели. */
typedef enum {
	SNAKE_CMD_NONE = 0,
	SNAKE_CMD_UP,
	SNAKE_CMD_DOWN,
	SNAKE_CMD_LEFT,
	SNAKE_CMD_RIGHT,
	SNAKE_CMD_START,     /* начать игру / продолжить после паузы */
	SNAKE_CMD_PAUSE,     /* пауза / продолжить */
	SNAKE_CMD_OPTION     /* в меню - режим стен, в паузе и после игры - выход в меню */
} snake_cmd_t;

typedef enum {
	SNAKE_STATE_MENU = 0,  /* стартовый экран */
	SNAKE_STATE_PLAYING,   /* идет игра */
	SNAKE_STATE_PAUSED,    /* игра приостановлена */
	SNAKE_STATE_OVER       /* игра окончена (проигрыш или заполнено все поле) */
} snake_state_t;

/* Что произошло в результате команды или шага. Любое событие, кроме
 * SNAKE_EV_NONE, означает, что изображение на экране нужно обновить. */
typedef enum {
	SNAKE_EV_NONE = 0,
	SNAKE_EV_MENU,       /* возврат в меню */
	SNAKE_EV_MODE,       /* переключен режим стен */
	SNAKE_EV_STARTED,    /* началась новая игра */
	SNAKE_EV_PAUSED,
	SNAKE_EV_RESUMED,
	SNAKE_EV_MOVED,      /* змейка сдвинулась на клетку */
	SNAKE_EV_ATE,        /* змейка съела еду и выросла */
	SNAKE_EV_CRASHED,    /* столкновение со стеной или с собой - проигрыш */
	SNAKE_EV_WON         /* змейка заняла все поле - победа */
} snake_event_t;

typedef struct {
	uint8_t x;
	uint8_t y;
} snake_cell_t;

typedef struct {
	snake_state_t state;
	uint8_t       wallsOn;       /* 1 - стены смертельны, 0 - змейка проходит сквозь край поля */

	snake_cell_t  body[SNAKE_MAX_LEN];  /* кольцевой буфер сегментов, body[head] - голова */
	uint16_t      head;
	uint16_t      length;
	uint8_t       grid[SNAKE_FIELD_H][SNAKE_FIELD_W]; /* 1 - клетка занята змейкой */

	snake_dir_t   dir;                    /* текущее направление движения */
	snake_dir_t   turns[SNAKE_TURN_BUF];  /* повороты, введенные до следующего шага */
	uint8_t       turnCount;

	snake_cell_t  food;
	uint16_t      score;         /* съедено еды в текущей игре */
	uint16_t      best;          /* лучший результат с момента включения */
	uint8_t       won;           /* 1 - игра закончилась победой */
	uint8_t       newBest;       /* 1 - в последней игре поставлен рекорд */

	uint32_t      rng;           /* состояние генератора псевдослучайных чисел */
} snake_game_t;

void          Snake_Init(snake_game_t *g);
snake_event_t Snake_Command(snake_game_t *g, snake_cmd_t cmd);
snake_event_t Snake_Step(snake_game_t *g);

/* Период шага игры: змейка ускоряется по мере роста счета. */
uint32_t      Snake_StepPeriodMs(const snake_game_t *g);

/* Подмешивает в генератор случайных чисел внешнюю энтропию (например,
 * время нажатия клавиши), чтобы еда появлялась по-разному в каждой игре. */
void          Snake_AddEntropy(snake_game_t *g, uint32_t value);

/* Сегмент змейки с номером index: 0 - голова, length - 1 - хвост. */
snake_cell_t  Snake_GetSegment(const snake_game_t *g, uint16_t index);

#endif /* INC_SNAKE_H_ */
