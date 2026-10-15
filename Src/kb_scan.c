#include "kb_scan.h"
#include "kb.h"

static char KB_ScanOnce(void)
{
	static const uint8_t rows[4] = { ROW1, ROW2, ROW3, ROW4 };
	static const char keymap[4][3] = {
		{ '1', '2', '3' },
		{ '4', '5', '6' },
		{ '7', '8', '9' },
		{ '*', '0', '#' }
	};

	for (int r = 0; r < 4; r++) {
		uint8_t key = Check_Row(rows[r]);
		if (key == 0x04) {
			return keymap[r][0]; /* левая колонка */
		} else if (key == 0x02) {
			return keymap[r][1]; /* средняя колонка */
		} else if (key == 0x01) {
			return keymap[r][2]; /* правая колонка */
		}
	}
	return 0;
}

char KB_GetKeyEvent(void)
{
	static char lastKey = 0;

	char key = KB_ScanOnce();
	char event = (key != 0 && key != lastKey) ? key : 0;
	lastKey = key;

	return event;
}
