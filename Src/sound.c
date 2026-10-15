#include "sound.h"
#include "buzzer.h"
#include "cmsis_os.h"

typedef struct {
	uint16_t freqHz;      /* 0 - пауза */
	uint16_t durationMs;
} note_t;

typedef struct {
	const note_t *notes;
	uint8_t count;
} melody_t;

#define MELODY(notes) { notes, sizeof(notes) / sizeof(notes[0]) }

static const note_t clickNotes[]    = { {1000, 20} };
static const note_t startNotes[]    = { {523, 80}, {659, 80}, {784, 120} };
static const note_t eatNotes[]      = { {1568, 40} };
static const note_t gameOverNotes[] = { {784, 150}, {587, 150}, {392, 150}, {262, 300} };
static const note_t winNotes[]      = { {523, 100}, {659, 100}, {784, 100}, {1047, 300} };

static const melody_t melodies[SOUND_COUNT] = {
	[SOUND_CLICK]     = MELODY(clickNotes),
	[SOUND_START]     = MELODY(startNotes),
	[SOUND_EAT]       = MELODY(eatNotes),
	[SOUND_GAME_OVER] = MELODY(gameOverNotes),
	[SOUND_WIN]       = MELODY(winNotes),
};

void Sound_Play(sound_id_t id)
{
	if (id >= SOUND_COUNT) {
		return;
	}

	const melody_t *melody = &melodies[id];
	for (uint8_t i = 0; i < melody->count; i++) {
		Buzzer_On(melody->notes[i].freqHz); /* частота 0 выключает звук */
		osDelay(melody->notes[i].durationMs);
	}
	Buzzer_Off();
}
