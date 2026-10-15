#ifndef INC_SOUND_H_
#define INC_SOUND_H_

/* Звуковые эффекты игры поверх драйвера зуммера (buzzer.c). */

typedef enum {
	SOUND_CLICK = 0,   /* отклик на паузу, смену режима, выход в меню */
	SOUND_START,       /* начало игры */
	SOUND_EAT,         /* змейка съела еду */
	SOUND_GAME_OVER,   /* проигрыш */
	SOUND_WIN,         /* победа */
	SOUND_COUNT
} sound_id_t;

/* Проигрывает мелодию целиком. Длительности нот отсчитываются через
 * osDelay(), поэтому функция блокирует только вызвавшую ее задачу
 * и предназначена для вызова из отдельной звуковой задачи FreeRTOS. */
void Sound_Play(sound_id_t id);

#endif /* INC_SOUND_H_ */
