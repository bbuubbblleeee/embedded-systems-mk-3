#include "snake.h"
#include <string.h>

/* Скорость игры: шаг начинается с 200 мс и сокращается на 5 мс за каждую
 * съеденную еду, но не быстрее 70 мс на клетку. */
#define PERIOD_START_MS    200
#define PERIOD_STEP_MS     5
#define PERIOD_MIN_MS      70

#define RNG_DEFAULT_SEED   0x2545F491UL

/* Смещение головы за один шаг для каждого направления (индекс - snake_dir_t). */
static const int8_t dirDx[4] = { 0, 1, 0, -1 };
static const int8_t dirDy[4] = { -1, 0, 1, 0 };

static snake_dir_t Opposite(snake_dir_t dir)
{
	return (snake_dir_t)((dir + 2) % 4);
}

/* Генератор псевдослучайных чисел xorshift32. */
static uint32_t Random_Next(snake_game_t *g)
{
	uint32_t x = g->rng;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	g->rng = x;
	return x;
}

static uint16_t Tail_Index(const snake_game_t *g)
{
	return (uint16_t)((g->head + SNAKE_MAX_LEN - (g->length - 1)) % SNAKE_MAX_LEN);
}

/* Ставит еду в случайную свободную клетку: выбирается номер n среди
 * свободных клеток, затем поле обходится до n-й свободной клетки. Так еда
 * никогда не попадает на змейку, даже когда свободных клеток почти не осталось. */
static void Place_Food(snake_game_t *g)
{
	uint16_t freeCells = SNAKE_MAX_LEN - g->length;
	uint16_t n = (uint16_t)(Random_Next(g) % freeCells);

	for (uint8_t y = 0; y < SNAKE_FIELD_H; y++) {
		for (uint8_t x = 0; x < SNAKE_FIELD_W; x++) {
			if (g->grid[y][x]) {
				continue;
			}
			if (n == 0) {
				g->food.x = x;
				g->food.y = y;
				return;
			}
			n--;
		}
	}
}

static void Start_Game(snake_game_t *g)
{
	uint8_t y = SNAKE_FIELD_H / 2;
	uint8_t x0 = SNAKE_FIELD_W / 2 - (SNAKE_START_LEN - 1);

	memset(g->grid, 0, sizeof(g->grid));

	/* Змейка появляется горизонтально в центре поля, хвостом влево. */
	for (uint16_t i = 0; i < SNAKE_START_LEN; i++) {
		g->body[i].x = x0 + i;
		g->body[i].y = y;
		g->grid[y][x0 + i] = 1;
	}
	g->head = SNAKE_START_LEN - 1;
	g->length = SNAKE_START_LEN;
	g->dir = SNAKE_DIR_RIGHT;
	g->turnCount = 0;
	g->score = 0;
	g->won = 0;
	g->newBest = 0;

	Place_Food(g);
	g->state = SNAKE_STATE_PLAYING;
}

static snake_event_t Finish_Game(snake_game_t *g, uint8_t won)
{
	g->state = SNAKE_STATE_OVER;
	g->won = won;
	g->newBest = (g->score > g->best);
	if (g->newBest) {
		g->best = g->score;
	}
	return won ? SNAKE_EV_WON : SNAKE_EV_CRASHED;
}

/* Запоминает поворот до следующего шага. Поворот сравнивается с последним
 * уже введенным направлением, поэтому быстрое «вверх, затем влево» за один
 * шаг отрабатывается за два шага, а разворот на 180 градусов невозможен. */
static void Queue_Turn(snake_game_t *g, snake_dir_t dir)
{
	snake_dir_t last = g->turnCount ? g->turns[g->turnCount - 1] : g->dir;

	if (dir == last || dir == Opposite(last) || g->turnCount >= SNAKE_TURN_BUF) {
		return;
	}
	g->turns[g->turnCount++] = dir;
}

static uint8_t Command_ToDir(snake_cmd_t cmd, snake_dir_t *dir)
{
	switch (cmd) {
	case SNAKE_CMD_UP:    *dir = SNAKE_DIR_UP;    return 1;
	case SNAKE_CMD_DOWN:  *dir = SNAKE_DIR_DOWN;  return 1;
	case SNAKE_CMD_LEFT:  *dir = SNAKE_DIR_LEFT;  return 1;
	case SNAKE_CMD_RIGHT: *dir = SNAKE_DIR_RIGHT; return 1;
	default:              return 0;
	}
}

void Snake_Init(snake_game_t *g)
{
	memset(g, 0, sizeof(*g));
	g->state = SNAKE_STATE_MENU;
	g->wallsOn = 1;
	g->rng = RNG_DEFAULT_SEED;
}

void Snake_AddEntropy(snake_game_t *g, uint32_t value)
{
	g->rng ^= value * 2654435761UL;
	if (g->rng == 0) {
		g->rng = RNG_DEFAULT_SEED; /* xorshift не выходит из нулевого состояния */
	}
	Random_Next(g);
}

snake_event_t Snake_Command(snake_game_t *g, snake_cmd_t cmd)
{
	snake_dir_t dir;

	switch (g->state) {
	case SNAKE_STATE_MENU:
		if (cmd == SNAKE_CMD_START) {
			Start_Game(g);
			return SNAKE_EV_STARTED;
		}
		if (cmd == SNAKE_CMD_OPTION) {
			g->wallsOn = !g->wallsOn;
			return SNAKE_EV_MODE;
		}
		break;

	case SNAKE_STATE_PLAYING:
		if (cmd == SNAKE_CMD_PAUSE) {
			g->state = SNAKE_STATE_PAUSED;
			return SNAKE_EV_PAUSED;
		}
		if (Command_ToDir(cmd, &dir)) {
			Queue_Turn(g, dir); /* поворот станет виден только на следующем шаге */
		}
		break;

	case SNAKE_STATE_PAUSED:
		if (cmd == SNAKE_CMD_PAUSE || cmd == SNAKE_CMD_START) {
			g->state = SNAKE_STATE_PLAYING;
			return SNAKE_EV_RESUMED;
		}
		if (cmd == SNAKE_CMD_OPTION) {
			g->state = SNAKE_STATE_MENU;
			return SNAKE_EV_MENU;
		}
		break;

	case SNAKE_STATE_OVER:
		if (cmd == SNAKE_CMD_START) {
			Start_Game(g);
			return SNAKE_EV_STARTED;
		}
		if (cmd == SNAKE_CMD_OPTION) {
			g->state = SNAKE_STATE_MENU;
			return SNAKE_EV_MENU;
		}
		break;
	}

	return SNAKE_EV_NONE;
}

snake_event_t Snake_Step(snake_game_t *g)
{
	if (g->state != SNAKE_STATE_PLAYING) {
		return SNAKE_EV_NONE;
	}

	if (g->turnCount > 0) {
		g->dir = g->turns[0];
		for (uint8_t i = 1; i < g->turnCount; i++) {
			g->turns[i - 1] = g->turns[i];
		}
		g->turnCount--;
	}

	snake_cell_t head = g->body[g->head];
	int16_t nx = head.x + dirDx[g->dir];
	int16_t ny = head.y + dirDy[g->dir];

	if (nx < 0 || nx >= SNAKE_FIELD_W || ny < 0 || ny >= SNAKE_FIELD_H) {
		if (g->wallsOn) {
			return Finish_Game(g, 0);
		}
		nx = (nx + SNAKE_FIELD_W) % SNAKE_FIELD_W;
		ny = (ny + SNAKE_FIELD_H) % SNAKE_FIELD_H;
	}

	uint8_t ate = (nx == g->food.x && ny == g->food.y);
	snake_cell_t tail = g->body[Tail_Index(g)];
	uint8_t intoTail = (nx == tail.x && ny == tail.y);

	/* Если змейка не растет, ее хвост на этом шаге освобождает клетку,
	 * поэтому голове можно войти в клетку, где сейчас находится хвост. */
	if (g->grid[ny][nx] && !(intoTail && !ate)) {
		return Finish_Game(g, 0);
	}

	if (ate) {
		g->length++;
	} else {
		g->grid[tail.y][tail.x] = 0;
	}
	g->head = (g->head + 1) % SNAKE_MAX_LEN;
	g->body[g->head].x = (uint8_t)nx;
	g->body[g->head].y = (uint8_t)ny;
	g->grid[ny][nx] = 1;

	if (!ate) {
		return SNAKE_EV_MOVED;
	}

	g->score++;
	if (g->length == SNAKE_MAX_LEN) {
		return Finish_Game(g, 1);
	}
	Place_Food(g);
	return SNAKE_EV_ATE;
}

uint32_t Snake_StepPeriodMs(const snake_game_t *g)
{
	uint32_t speedup = (uint32_t)g->score * PERIOD_STEP_MS;

	if (speedup >= PERIOD_START_MS - PERIOD_MIN_MS) {
		return PERIOD_MIN_MS;
	}
	return PERIOD_START_MS - speedup;
}

snake_cell_t Snake_GetSegment(const snake_game_t *g, uint16_t index)
{
	return g->body[(g->head + SNAKE_MAX_LEN - index) % SNAKE_MAX_LEN];
}
