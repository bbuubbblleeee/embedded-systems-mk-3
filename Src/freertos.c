/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "oled.h"
#include "kb_scan.h"
#include "sdk_uart.h"
#include "snake.h"
#include "snake_view.h"
#include "sound.h"
#include "leds.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* Период опроса клавиатуры. Опрос раз в 20 мс заодно подавляет дребезг:
 * контакт успевает успокоиться между двумя соседними опросами. */
#define KB_SCAN_PERIOD_MS   20

/* Период мигания зеленого светодиода «РАБОТА». */
#define LED_PERIOD_MS       250
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
/* Состояние игры - общий ресурс задач Task_Game (изменяет его),
 * Task_Display (рисует по нему кадр) и Task_LED (читает режим игры).
 * Доступ на запись и рисование защищен мьютексом gameMutex. */
static snake_game_t game;
/* USER CODE END Variables */
osThreadId Task_GameHandle;
osThreadId Task_KeyboardHandle;
osThreadId Task_DisplayHandle;
osThreadId Task_SoundHandle;
osThreadId Task_LEDHandle;
osMessageQId keyQueueHandle;
osMessageQId soundQueueHandle;
osMutexId i2cMutexHandle;
osMutexId gameMutexHandle;
osSemaphoreId displaySemHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static snake_cmd_t Key_ToCommand(char key);
static void Request_Redraw(void);
static void Play_Sound(sound_id_t id);
static void Report_Event(snake_event_t event);
/* USER CODE END FunctionPrototypes */

void StartTaskGame(void const * argument);
void StartTaskKeyboard(void const * argument);
void StartTaskDisplay(void const * argument);
void StartTaskSound(void const * argument);
void StartTaskLED(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  /* Игра инициализируется до запуска планировщика, чтобы ни одна задача
   * не увидела ее в неопределенном состоянии. */
  Snake_Init(&game);
  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* definition and creation of i2cMutex */
  osMutexDef(i2cMutex);
  i2cMutexHandle = osMutexCreate(osMutex(i2cMutex));

  /* definition and creation of gameMutex */
  osMutexDef(gameMutex);
  gameMutexHandle = osMutexCreate(osMutex(gameMutex));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of displaySem */
  osSemaphoreDef(displaySem);
  displaySemHandle = osSemaphoreCreate(osSemaphore(displaySem), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* definition and creation of keyQueue */
  osMessageQDef(keyQueue, 8, uint8_t);
  keyQueueHandle = osMessageCreate(osMessageQ(keyQueue), NULL);

  /* definition and creation of soundQueue */
  osMessageQDef(soundQueue, 4, uint8_t);
  soundQueueHandle = osMessageCreate(osMessageQ(soundQueue), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of Task_Game */
  osThreadDef(Task_Game, StartTaskGame, osPriorityHigh, 0, 256);
  Task_GameHandle = osThreadCreate(osThread(Task_Game), NULL);

  /* definition and creation of Task_Keyboard */
  osThreadDef(Task_Keyboard, StartTaskKeyboard, osPriorityAboveNormal, 0, 192);
  Task_KeyboardHandle = osThreadCreate(osThread(Task_Keyboard), NULL);

  /* definition and creation of Task_Display */
  osThreadDef(Task_Display, StartTaskDisplay, osPriorityNormal, 0, 384);
  Task_DisplayHandle = osThreadCreate(osThread(Task_Display), NULL);

  /* definition and creation of Task_Sound */
  osThreadDef(Task_Sound, StartTaskSound, osPriorityAboveNormal, 0, 128);
  Task_SoundHandle = osThreadCreate(osThread(Task_Sound), NULL);

  /* definition and creation of Task_LED */
  osThreadDef(Task_LED, StartTaskLED, osPriorityLow, 0, 128);
  Task_LEDHandle = osThreadCreate(osThread(Task_LED), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* Имена объектов видны в отладчике (FreeRTOS Task List / Queues). */
  vQueueAddToRegistry(keyQueueHandle, "keyQueue");
  vQueueAddToRegistry(soundQueueHandle, "soundQueue");
  vQueueAddToRegistry(i2cMutexHandle, "i2cMutex");
  vQueueAddToRegistry(gameMutexHandle, "gameMutex");
  vQueueAddToRegistry(displaySemHandle, "displaySem");
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartTaskGame */
/**
  * @brief  Игровая логика. Ждет события клавиатуры из очереди keyQueue с
  *         таймаутом до следующего шага змейки: пришла клавиша - выполняет
  *         команду, истек таймаут - делает шаг. О каждом изменении сообщает
  *         задаче дисплея (семафор displaySem) и звуковой задаче (soundQueue).
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartTaskGame */
void StartTaskGame(void const * argument)
{
  /* USER CODE BEGIN StartTaskGame */
  uint32_t nextStepTick = 0;
  osEvent keyEvent;
  snake_event_t event;

  Request_Redraw(); /* показать стартовый экран */

  /* Infinite loop */
  for(;;)
  {
    /* Только эта задача изменяет game, поэтому сама она может читать
     * состояние без мьютекса. */
    uint32_t timeout = osWaitForever;
    if (game.state == SNAKE_STATE_PLAYING) {
      int32_t left = (int32_t)(nextStepTick - osKernelSysTick());
      timeout = (left > 0) ? (uint32_t)left : 0;
    }

    keyEvent = osMessageGet(keyQueueHandle, timeout);

    if (keyEvent.status == osEventMessage) {
      snake_cmd_t cmd = Key_ToCommand((char)keyEvent.value.v);

      osMutexWait(gameMutexHandle, osWaitForever);
      Snake_AddEntropy(&game, osKernelSysTick());
      event = Snake_Command(&game, cmd);
      osMutexRelease(gameMutexHandle);

      if (event == SNAKE_EV_STARTED || event == SNAKE_EV_RESUMED) {
        nextStepTick = osKernelSysTick() + Snake_StepPeriodMs(&game);
      }
    } else if (game.state == SNAKE_STATE_PLAYING) {
      osMutexWait(gameMutexHandle, osWaitForever);
      event = Snake_Step(&game);
      osMutexRelease(gameMutexHandle);

      nextStepTick += Snake_StepPeriodMs(&game);
    } else {
      continue;
    }

    if (event != SNAKE_EV_NONE) {
      Request_Redraw();
      Report_Event(event);
    }
  }
  /* USER CODE END StartTaskGame */
}

/* USER CODE BEGIN Header_StartTaskKeyboard */
/**
  * @brief  Каждые 20 мс опрашивает матричную клавиатуру и кладет коды новых
  *         нажатий в очередь keyQueue. Клавиатура и дисплей подключены к
  *         одной шине I2C1, поэтому опрос выполняется под мьютексом i2cMutex.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartTaskKeyboard */
void StartTaskKeyboard(void const * argument)
{
  /* USER CODE BEGIN StartTaskKeyboard */
  uint32_t wakeTime = osKernelSysTick();
  char key;

  /* Infinite loop */
  for(;;)
  {
    osMutexWait(i2cMutexHandle, osWaitForever);
    key = KB_GetKeyEvent();
    osMutexRelease(i2cMutexHandle);

    if (key) {
      osMessagePut(keyQueueHandle, (uint32_t)key, 0);
    }
    osDelayUntil(&wakeTime, KB_SCAN_PERIOD_MS);
  }
  /* USER CODE END StartTaskKeyboard */
}

/* USER CODE BEGIN Header_StartTaskDisplay */
/**
  * @brief  Ждет семафор displaySem, рисует кадр по состоянию игры и передает
  *         его на OLED-дисплей. Если за время передачи кадра игра изменилась
  *         несколько раз, семафор остается взведенным один раз и следующим
  *         рисуется самое свежее состояние.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartTaskDisplay */
void StartTaskDisplay(void const * argument)
{
  /* USER CODE BEGIN StartTaskDisplay */
  /* Infinite loop */
  for(;;)
  {
    osSemaphoreWait(displaySemHandle, osWaitForever);

    /* Кадр рисуется в буфер в ОЗУ быстро, пока держится мьютекс игры... */
    osMutexWait(gameMutexHandle, osWaitForever);
    SnakeView_Draw(&game);
    osMutexRelease(gameMutexHandle);

    /* ...а долгая передача по I2C идет уже без него, чтобы не задерживать
     * игровую логику. Оба мьютекса никогда не удерживаются одновременно,
     * поэтому взаимоблокировка невозможна. Кадр передается по страницам
     * (8 строк точек), причем только изменившимися, а шина захватывается
     * на одну страницу, чтобы опрос клавиатуры не ждал весь кадр. */
    for (uint8_t page = 0; page < OLED_PAGES; page++) {
      osMutexWait(i2cMutexHandle, osWaitForever);
      oled_UpdatePage(page);
      osMutexRelease(i2cMutexHandle);
    }
  }
  /* USER CODE END StartTaskDisplay */
}

/* USER CODE BEGIN Header_StartTaskSound */
/**
  * @brief  Проигрывает звуковые эффекты из очереди soundQueue. Мелодии
  *         звучат в отдельной задаче, поэтому не задерживают игру.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartTaskSound */
void StartTaskSound(void const * argument)
{
  /* USER CODE BEGIN StartTaskSound */
  osEvent soundEvent;

  /* Infinite loop */
  for(;;)
  {
    soundEvent = osMessageGet(soundQueueHandle, osWaitForever);
    if (soundEvent.status == osEventMessage) {
      Sound_Play((sound_id_t)soundEvent.value.v);
    }
  }
  /* USER CODE END StartTaskSound */
}

/* USER CODE BEGIN Header_StartTaskLED */
/**
  * @brief  Задача с низшим приоритетом: мигает зеленым светодиодом «РАБОТА»
  *         (планировщик работает и процессор не перегружен) и показывает режим
  *         игры светодиодом «РЕЖИМ»: желтый - пауза, красный - игра окончена.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartTaskLED */
void StartTaskLED(void const * argument)
{
  /* USER CODE BEGIN StartTaskLED */
  snake_state_t state;

  /* Infinite loop */
  for(;;)
  {
    Leds_ToggleWork();

    /* Чтение одной переменной выполняется одной инструкцией и атомарно,
     * поэтому для него мьютекс не нужен. */
    state = game.state;
    if (state == SNAKE_STATE_PAUSED) {
      Leds_SetMode(LED_MODE_YELLOW);
    } else if (state == SNAKE_STATE_OVER) {
      Leds_SetMode(LED_MODE_RED);
    } else {
      Leds_SetMode(LED_MODE_OFF);
    }

    osDelay(LED_PERIOD_MS);
  }
  /* USER CODE END StartTaskLED */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/* Раскладка клавиатуры (см. kb_scan.h) и назначение клавиш в игре:
 *   1 [2] 3        2, 4, 6, 8 - движение вверх, влево, вправо, вниз;
 *  [4][5][6]       5 - пауза / продолжить;
 *   7 [8] 9        # - начать игру;
 *  [*] 0 [#]       * - режим стен в меню, выход в меню из паузы и после игры. */
static snake_cmd_t Key_ToCommand(char key)
{
  switch (key) {
  case '2': return SNAKE_CMD_UP;
  case '8': return SNAKE_CMD_DOWN;
  case '4': return SNAKE_CMD_LEFT;
  case '6': return SNAKE_CMD_RIGHT;
  case '5': return SNAKE_CMD_PAUSE;
  case '#': return SNAKE_CMD_START;
  case '*': return SNAKE_CMD_OPTION;
  default:  return SNAKE_CMD_NONE;
  }
}

/* Двоичный семафор не накапливает повторные «выдачи», поэтому лишние
 * запросы перерисовки, пока задача дисплея занята, просто сливаются в один. */
static void Request_Redraw(void)
{
  osSemaphoreRelease(displaySemHandle);
}

/* Очередь звуков короткая: если она заполнена, звук пропускается, а игра
 * не ждет звуковую задачу. */
static void Play_Sound(sound_id_t id)
{
  osMessagePut(soundQueueHandle, (uint32_t)id, 0);
}

/* Звуковая реакция на события игры и диагностические сообщения в UART6. */
static void Report_Event(snake_event_t event)
{
  char msg[48];

  switch (event) {
  case SNAKE_EV_STARTED:
    Play_Sound(SOUND_START);
    snprintf(msg, sizeof(msg), "Game started, walls %s\r\n", game.wallsOn ? "ON" : "OFF");
    UART_Transmit((uint8_t *)msg);
    break;

  case SNAKE_EV_ATE:
    Play_Sound(SOUND_EAT);
    snprintf(msg, sizeof(msg), "Score: %u\r\n", game.score);
    UART_Transmit((uint8_t *)msg);
    break;

  case SNAKE_EV_CRASHED:
  case SNAKE_EV_WON:
    Play_Sound(event == SNAKE_EV_WON ? SOUND_WIN : SOUND_GAME_OVER);
    snprintf(msg, sizeof(msg), "%s Score: %u, best: %u\r\n",
             event == SNAKE_EV_WON ? "You win!" : "Game over.", game.score, game.best);
    UART_Transmit((uint8_t *)msg);
    break;

  case SNAKE_EV_PAUSED:
  case SNAKE_EV_RESUMED:
  case SNAKE_EV_MODE:
  case SNAKE_EV_MENU:
    Play_Sound(SOUND_CLICK);
    break;

  default:
    break;
  }
}
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
