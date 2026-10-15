#include "snake_view.h"
#include "oled.h"
#include "fonts.h"
#include <stdio.h>
#include <string.h>

/* Разметка экрана 128x64:
 *   строки 0..9   - строка состояния (счет и рекорд, шрифт 7x10);
 *   строки 10..63 - рамка игрового поля и 31x13 клеток по 4x4 пикселя. */
#define CELL_PX      4
#define FIELD_X0     2     /* экранные координаты левого верхнего пикселя клетки (0, 0) */
#define FIELD_Y0     11
#define FRAME_LEFT   (FIELD_X0 - 1)
#define FRAME_TOP    (FIELD_Y0 - 1)
#define FRAME_RIGHT  (FIELD_X0 + SNAKE_FIELD_W * CELL_PX)
#define FRAME_BOTTOM (FIELD_Y0 + SNAKE_FIELD_H * CELL_PX)

/* ------------------------------ Примитивы рисования ------------------------------ */
static void Fill_Rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, OLED_COLOR color)
{
	for (uint8_t j = 0; j < h; j++) {
		for (uint8_t i = 0; i < w; i++) {
			oled_DrawPixel(x + i, y + j, color);
		}
	}
}

/* Прямоугольник с белой рамкой, внутренность которого очищается: на нем
 * выводятся сообщения поверх игрового поля. */
static void Draw_Box(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	Fill_Rect(x, y, w, h, Black);
	oled_DrawSquare(x, x + w - 1, y, y + h - 1, White);
}

static void Draw_Text(uint8_t x, uint8_t y, const char *text, FontDef font)
{
	oled_SetCursor(x, y);
	oled_WriteString((char *)text, font, White);
}

static void Draw_Text_Centered(uint8_t y, const char *text, FontDef font)
{
	uint8_t width = (uint8_t)(strlen(text) * font.FontWidth);
	Draw_Text((uint8_t)((OLED_WIDTH - width) / 2), y, text, font);
}

/* ------------------------------ Элементы игрового поля --------------------------- */
static uint8_t Cell_X(uint8_t x) { return FIELD_X0 + x * CELL_PX; }
static uint8_t Cell_Y(uint8_t y) { return FIELD_Y0 + y * CELL_PX; }

/* Сплошная рамка - стены смертельны, пунктирная - сквозь край поля можно пройти. */
static void Draw_Frame(uint8_t solid)
{
	for (uint8_t x = FRAME_LEFT; x <= FRAME_RIGHT; x++) {
		if (solid || (x % 2 == 0)) {
			oled_DrawPixel(x, FRAME_TOP, White);
			oled_DrawPixel(x, FRAME_BOTTOM, White);
		}
	}
	for (uint8_t y = FRAME_TOP; y <= FRAME_BOTTOM; y++) {
		if (solid || (y % 2 == 0)) {
			oled_DrawPixel(FRAME_LEFT, y, White);
			oled_DrawPixel(FRAME_RIGHT, y, White);
		}
	}
}

/* Еда - «плюс» внутри клетки. */
static void Draw_Food(snake_cell_t c)
{
	uint8_t x = Cell_X(c.x);
	uint8_t y = Cell_Y(c.y);

	oled_DrawPixel(x + 1, y, White);
	Fill_Rect(x, y + 1, 3, 1, White);
	oled_DrawPixel(x + 1, y + 2, White);
}

/* Каждый сегмент - квадрат 3x3. Соседние сегменты соединяются перемычкой
 * в зазоре между клетками, поэтому змейка выглядит сплошной линией, а
 * параллельные участки тела остаются различимы. */
static void Draw_Snake(const snake_game_t *g)
{
	snake_cell_t prev = Snake_GetSegment(g, 0);

	for (uint16_t i = 0; i < g->length; i++) {
		snake_cell_t c = Snake_GetSegment(g, i);
		Fill_Rect(Cell_X(c.x), Cell_Y(c.y), CELL_PX - 1, CELL_PX - 1, White);

		if (i > 0) {
			/* Перемычка рисуется только между клетками-соседями;
			 * при переходе через край поля (режим без стен) ее нет. */
			if (c.y == prev.y && (c.x + 1 == prev.x || prev.x + 1 == c.x)) {
				uint8_t left = (c.x < prev.x) ? c.x : prev.x;
				Fill_Rect(Cell_X(left) + CELL_PX - 1, Cell_Y(c.y), 1, CELL_PX - 1, White);
			} else if (c.x == prev.x && (c.y + 1 == prev.y || prev.y + 1 == c.y)) {
				uint8_t top = (c.y < prev.y) ? c.y : prev.y;
				Fill_Rect(Cell_X(c.x), Cell_Y(top) + CELL_PX - 1, CELL_PX - 1, 1, White);
			}
		}
		prev = c;
	}

	/* Голова отличается от тела «глазом» - погашенным центральным пикселем. */
	snake_cell_t head = Snake_GetSegment(g, 0);
	oled_DrawPixel(Cell_X(head.x) + 1, Cell_Y(head.y) + 1, Black);
}

static void Draw_Status(const snake_game_t *g)
{
	char buf[12];

	snprintf(buf, sizeof(buf), "Score:%u", g->score);
	Draw_Text(0, 0, buf, Font_7x10);

	snprintf(buf, sizeof(buf), "Best:%u", g->best);
	Draw_Text((uint8_t)(OLED_WIDTH - 1 - strlen(buf) * Font_7x10.FontWidth), 0, buf, Font_7x10);
}

/* ------------------------------------ Экраны ------------------------------------- */
static void Draw_Menu(const snake_game_t *g)
{
	char buf[20];

	Draw_Text_Centered(0, "SNAKE", Font_16x26);

	snprintf(buf, sizeof(buf), "Walls: %-3s  (*)", g->wallsOn ? "ON" : "OFF");
	Draw_Text(8, 31, buf, Font_7x10);

	snprintf(buf, sizeof(buf), "Best score: %u", g->best);
	Draw_Text(8, 42, buf, Font_7x10);

	Draw_Text_Centered(53, "Press # to start", Font_7x10);
}

static void Draw_Pause(void)
{
	Draw_Box(18, 18, 92, 38);
	Draw_Text_Centered(21, "PAUSE", Font_11x18);
	Draw_Text_Centered(42, "5-go  *-menu", Font_7x10);
}

static void Draw_Game_Over(const snake_game_t *g)
{
	char buf[20];

	Draw_Box(6, 14, 116, 48);
	Draw_Text_Centered(17, g->won ? "YOU WIN!" : "GAME OVER", Font_11x18);

	if (g->newBest) {
		snprintf(buf, sizeof(buf), "New best: %u!", g->score);
	} else {
		snprintf(buf, sizeof(buf), "Score: %u", g->score);
	}
	Draw_Text_Centered(37, buf, Font_7x10);
	Draw_Text_Centered(48, "#-again *-menu", Font_7x10);
}

void SnakeView_Draw(const snake_game_t *g)
{
	oled_Fill(Black);

	if (g->state == SNAKE_STATE_MENU) {
		Draw_Menu(g);
		return;
	}

	Draw_Status(g);
	Draw_Frame(g->wallsOn);
	Draw_Food(g->food);
	Draw_Snake(g);

	if (g->state == SNAKE_STATE_PAUSED) {
		Draw_Pause();
	} else if (g->state == SNAKE_STATE_OVER) {
		Draw_Game_Over(g);
	}
}
