/* Тесты модели игры на ПК (не входят в прошивку).
 * Сборка и запуск из папки tests:
 *   mkdir -p out && cc -std=gnu11 -Istub -I../Inc test_snake.c ../Src/snake.c ../Src/snake_view.c ../Src/oled.c ../Src/fonts.c -o test_snake && ./test_snake
 * Изображения экранов сохраняются в папку out (формат PBM). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "snake.h"
#include "snake_view.h"
#include "oled.h"

/* ---------- эмуляция GDDRAM контроллера SSD1306 ---------- */
I2C_HandleTypeDef hi2c1;
static uint8_t gddram[8][128];
static int curPage = 0;
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *h, uint16_t addr, uint16_t mem, uint16_t memsz, uint8_t *data, uint16_t size, uint32_t timeout)
{
	(void)h; (void)memsz; (void)timeout; assert(addr == 0x78);
	if (mem == 0x00 && size == 1 && data[0] >= 0xB0 && data[0] <= 0xB7) curPage = data[0] - 0xB0;
	if (mem == 0x40) { assert(size == 128); memcpy(gddram[curPage], data, 128); }
	return HAL_OK;
}
void HAL_Delay(uint32_t ms) { (void)ms; }

static void dump(const char *name)
{
	char path[256]; snprintf(path, sizeof path, "out/%s.pbm", name);
	FILE *f = fopen(path, "wb"); if (!f) return; fprintf(f, "P4\n128 64\n");
	for (int y = 0; y < 64; y++) { for (int xb = 0; xb < 16; xb++) { uint8_t b = 0;
		for (int i = 0; i < 8; i++) { int x = xb * 8 + i; if (gddram[y / 8][x] & (1 << (y % 8))) b |= 0x80 >> i; }
		fputc(b, f); } }
	fclose(f);
}
static void render(const snake_game_t *g, const char *name) { SnakeView_Draw(g); oled_UpdateScreen(); dump(name); }

/* ---------- проверка целостности состояния ---------- */
static void check_invariants(const snake_game_t *g)
{
	int cnt = 0; static uint8_t seen[SNAKE_FIELD_H][SNAKE_FIELD_W]; memset(seen, 0, sizeof seen);
	for (int y = 0; y < SNAKE_FIELD_H; y++) for (int x = 0; x < SNAKE_FIELD_W; x++) cnt += g->grid[y][x];
	assert(cnt == g->length);
	for (int i = 0; i < g->length; i++) {
		snake_cell_t c = Snake_GetSegment(g, i);
		assert(c.x < SNAKE_FIELD_W && c.y < SNAKE_FIELD_H);
		assert(g->grid[c.y][c.x] == 1);
		assert(!seen[c.y][c.x]); seen[c.y][c.x] = 1;
		if (i > 0 && g->wallsOn) { snake_cell_t p = Snake_GetSegment(g, i - 1); assert(abs(p.x - c.x) + abs(p.y - c.y) == 1); }
	}
	if (g->state == SNAKE_STATE_PLAYING) assert(g->grid[g->food.y][g->food.x] == 0);
}

static snake_game_t g;

/* бот: поворачивает к еде, избегая немедленной смерти */
static snake_cmd_t bot(const snake_game_t *g)
{
	static const int dx[4] = {0,1,0,-1}, dy[4] = {-1,0,1,0};
	static const snake_cmd_t cmds[4] = {SNAKE_CMD_UP, SNAKE_CMD_RIGHT, SNAKE_CMD_DOWN, SNAKE_CMD_LEFT};
	snake_cell_t h = Snake_GetSegment(g, 0);
	int best = -1, bestd = 1 << 30;
	for (int d = 0; d < 4; d++) {
		if (d == (g->dir + 2) % 4) continue;
		int nx = h.x + dx[d], ny = h.y + dy[d];
		if (!g->wallsOn) { nx = (nx + SNAKE_FIELD_W) % SNAKE_FIELD_W; ny = (ny + SNAKE_FIELD_H) % SNAKE_FIELD_H; }
		if (nx < 0 || ny < 0 || nx >= SNAKE_FIELD_W || ny >= SNAKE_FIELD_H) continue;
		if (g->grid[ny][nx]) continue;
		int dist = abs(nx - g->food.x) + abs(ny - g->food.y);
		if (dist < bestd) { bestd = dist; best = d; }
	}
	return best < 0 ? SNAKE_CMD_NONE : cmds[best];
}

int main(void)
{
	oled_Init();
	Snake_Init(&g);
	assert(g.state == SNAKE_STATE_MENU && g.wallsOn == 1);
	render(&g, "01_menu"); fprintf(stderr, "menu ok\n");

	/* команды в меню */
	assert(Snake_Command(&g, SNAKE_CMD_UP) == SNAKE_EV_NONE);
	assert(Snake_Command(&g, SNAKE_CMD_OPTION) == SNAKE_EV_MODE && g.wallsOn == 0);
	assert(Snake_Command(&g, SNAKE_CMD_OPTION) == SNAKE_EV_MODE && g.wallsOn == 1);
	assert(Snake_Step(&g) == SNAKE_EV_NONE);

	/* старт */
	assert(Snake_Command(&g, SNAKE_CMD_START) == SNAKE_EV_STARTED);
	assert(g.state == SNAKE_STATE_PLAYING && g.length == 3 && g.dir == SNAKE_DIR_RIGHT && g.score == 0);
	check_invariants(&g);
	assert(Snake_StepPeriodMs(&g) == 200);

	/* разворот на 180 запрещен */
	Snake_Command(&g, SNAKE_CMD_LEFT);
	assert(g.turnCount == 0);
	/* два поворота за шаг: вверх, затем влево */
	Snake_Command(&g, SNAKE_CMD_UP); Snake_Command(&g, SNAKE_CMD_LEFT); Snake_Command(&g, SNAKE_CMD_DOWN);
	assert(g.turnCount == 2);
	snake_cell_t h0 = Snake_GetSegment(&g, 0);
	g.food.x = 0; g.food.y = 0; /* убрать еду с пути */
	assert(Snake_Step(&g) == SNAKE_EV_MOVED); assert(g.dir == SNAKE_DIR_UP);
	assert(Snake_GetSegment(&g, 0).x == h0.x && Snake_GetSegment(&g, 0).y == h0.y - 1);
	assert(Snake_Step(&g) == SNAKE_EV_MOVED); assert(g.dir == SNAKE_DIR_LEFT);
	check_invariants(&g);

	/* пауза */
	assert(Snake_Command(&g, SNAKE_CMD_PAUSE) == SNAKE_EV_PAUSED);
	assert(Snake_Step(&g) == SNAKE_EV_NONE);
	assert(Snake_Command(&g, SNAKE_CMD_UP) == SNAKE_EV_NONE && g.turnCount == 0);
	assert(Snake_Command(&g, SNAKE_CMD_PAUSE) == SNAKE_EV_RESUMED);

	/* еда: поставить прямо перед головой */
	snake_cell_t h = Snake_GetSegment(&g, 0);
	g.grid[g.food.y][g.food.x] = 0; g.food.x = h.x - 1; g.food.y = h.y;
	assert(Snake_Step(&g) == SNAKE_EV_ATE);
	assert(g.length == 4 && g.score == 1 && Snake_StepPeriodMs(&g) == 195);
	check_invariants(&g);

	/* удар о левую стену */
	g.food.x = SNAKE_FIELD_W - 1; g.food.y = SNAKE_FIELD_H - 1;
	snake_event_t ev;
	while ((ev = Snake_Step(&g)) == SNAKE_EV_MOVED) { check_invariants(&g); }
	assert(ev == SNAKE_EV_CRASHED && g.state == SNAKE_STATE_OVER);
	assert(Snake_GetSegment(&g, 0).x == 0);
	assert(g.best == 1 && g.newBest == 1);
	render(&g, "05_gameover");
	assert(Snake_Command(&g, SNAKE_CMD_OPTION) == SNAKE_EV_MENU);
	render(&g, "01b_menu_best");

	/* без стен: проход сквозь край */
	Snake_Command(&g, SNAKE_CMD_OPTION);
	assert(Snake_Command(&g, SNAKE_CMD_START) == SNAKE_EV_STARTED && g.wallsOn == 0);
	g.food.x = 0; g.food.y = 0;
	for (int i = 0; i < SNAKE_FIELD_W; i++) { assert(Snake_Step(&g) == SNAKE_EV_MOVED); }
	assert(Snake_GetSegment(&g, 0).x == SNAKE_FIELD_W / 2 && Snake_GetSegment(&g, 0).y == SNAKE_FIELD_H / 2);
	check_invariants(&g);

	/* голова может войти в клетку, которую освобождает хвост (змейка длиной 4 по кругу) */
	Snake_Init(&g); Snake_Command(&g, SNAKE_CMD_START);
	h = Snake_GetSegment(&g, 0); g.food.x = h.x + 1; g.food.y = h.y; assert(Snake_Step(&g) == SNAKE_EV_ATE); /* длина 4 */
	g.food.x = 0; g.food.y = 0; g.grid[0][0] = 0;
	snake_cmd_t circle[] = { SNAKE_CMD_DOWN, SNAKE_CMD_LEFT, SNAKE_CMD_UP, SNAKE_CMD_RIGHT };
	for (int r = 0; r < 12; r++) { Snake_Command(&g, circle[r % 4]); assert(Snake_Step(&g) == SNAKE_EV_MOVED); check_invariants(&g); }
	/* а в собственное тело (не хвост) - нельзя: длина 5 и поворот в шею */
	h = Snake_GetSegment(&g, 0);

	fprintf(stderr, "basic ok\n");
	/* победа: заполнить все поле, кроме одной клетки, змейкой-«змеевиком» */
	Snake_Init(&g); Snake_Command(&g, SNAKE_CMD_START);
	memset(g.grid, 0, sizeof g.grid);
	int n = 0;
	for (int y = 0; y < SNAKE_FIELD_H; y++)
		for (int i = 0; i < SNAKE_FIELD_W; i++) {
			int x = (y % 2 == 0) ? i : SNAKE_FIELD_W - 1 - i;
			if (n == SNAKE_MAX_LEN - 1) break;
			g.body[n].x = x; g.body[n].y = y; g.grid[y][x] = 1; n++;
		}
	g.length = n; g.head = n - 1; g.turnCount = 0;
	g.dir = SNAKE_DIR_LEFT; /* последняя строка (y=12, четная) идет вправо; голова в (29,12) */
	g.dir = SNAKE_DIR_RIGHT; g.food.x = SNAKE_FIELD_W - 1; g.food.y = SNAKE_FIELD_H - 1;
	check_invariants(&g);
	render(&g, "07_almost_full");
	assert(Snake_Step(&g) == SNAKE_EV_WON && g.state == SNAKE_STATE_OVER && g.won == 1 && g.length == SNAKE_MAX_LEN);
	render(&g, "06_win");

	fprintf(stderr, "win ok\n");
	/* длительный прогон ботом: инварианты, разброс еды, рост скорости */
	long totalSteps = 0, games = 0, maxScore = 0;
	Snake_Init(&g);
	for (int game = 0; game < 300; game++) {
		g.wallsOn = game % 2;
		Snake_AddEntropy(&g, game * 7919u);
		assert(Snake_Command(&g, game == 0 ? SNAKE_CMD_START : SNAKE_CMD_START) == SNAKE_EV_STARTED);
		for (long st = 0; st < 20000; st++) {
			snake_cmd_t c = bot(&g); if (c) Snake_Command(&g, c);
			ev = Snake_Step(&g); totalSteps++;
			check_invariants(&g);
			if (ev == SNAKE_EV_CRASHED || ev == SNAKE_EV_WON) break;
			if (game == 7 && g.score == 12 && ev == SNAKE_EV_ATE) { render(&g, "03_playing"); Snake_Command(&g, SNAKE_CMD_PAUSE); render(&g, "04_pause"); Snake_Command(&g, SNAKE_CMD_PAUSE); }
			if (game == 8 && g.score == 25 && ev == SNAKE_EV_ATE) { render(&g, "03b_playing_nowalls"); }
			if (totalSteps > 50000000) abort();
		}
		games++; if (g.score > maxScore) maxScore = g.score;
		assert(Snake_StepPeriodMs(&g) >= 70);
	}
	/* первая картинка игры - сразу после старта */
	Snake_Init(&g); Snake_Command(&g, SNAKE_CMD_START); render(&g, "02_start");
	printf("OK: %ld games, %ld steps, max score %ld\n", games, totalSteps, maxScore);
	return 0;
}
