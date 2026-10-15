#include "buzzer.h"

static TIM_HandleTypeDef htim1;

static void Buzzer_GPIO_Init(void)
{
	__HAL_RCC_GPIOE_CLK_ENABLE();

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}

void Buzzer_Init(void)
{
	__HAL_RCC_TIM1_CLK_ENABLE();
	Buzzer_GPIO_Init();

	/* Тактовая частота TIM1 (APB2 timer clock) в данном проекте равна 168 МГц.
	 * Делитель 167 дает счетный такт таймера 1 МГц, что удобно для расчета
	 * периода по формуле ARR = 1 000 000 / частота_звука. */
	htim1.Instance = TIM1;
	htim1.Init.Prescaler = 167;
	htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim1.Init.Period = 1000 - 1;
	htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim1.Init.RepetitionCounter = 0;
	htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	HAL_TIM_PWM_Init(&htim1);

	TIM_OC_InitTypeDef sConfigOC = {0};
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 0;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1);
}

void Buzzer_On(uint32_t freq_hz)
{
	if (freq_hz == 0) {
		Buzzer_Off();
		return;
	}

	uint32_t arr = 1000000UL / freq_hz;
	if (arr < 2) {
		arr = 2;
	}

	__HAL_TIM_SET_AUTORELOAD(&htim1, arr - 1);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, arr / 2); /* 50% скважности = максимальная громкость */
	/* При смене ноты на ходу счетчик мог уже уйти за новое значение ARR -
	 * тогда он досчитал бы до 0xFFFF и звук прервался бы на ~65 мс. */
	__HAL_TIM_SET_COUNTER(&htim1, 0);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
}

void Buzzer_Off(void)
{
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
}
