#ifndef INC_BUZZER_H_
#define INC_BUZZER_H_

#include "main.h"

/* Излучатель звука BA1 подключен к PE9 (TIM1_CH1), см. схему стенда SDK-1.1M.
 * Чтобы звук действительно был слышен, на лицевой панели стенда должен быть
 * замкнут 7-й контакт движкового переключателя SA2. */

void Buzzer_Init(void);
void Buzzer_On(uint32_t freq_hz);
void Buzzer_Off(void);

#endif /* INC_BUZZER_H_ */
