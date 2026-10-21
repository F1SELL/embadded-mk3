/* Red Ball application: CMSIS-RTOS v1 threads with native FreeRTOS IPC.
 * Board support and kernel are from lmtspbru/SDK-1.1M (see reference/).
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "cmsis_os.h"
#include "main.h"
#include "i2c.h"
#include "oled.h"
#include "game.h"
#include "game_render.h"
#include "buzzer.h"
static QueueHandle_t input_queue, frame_queue, sound_queue;
static SemaphoreHandle_t bus_mutex;
static StaticTask_t idle_tcb;
static StackType_t idle_stack[configMINIMAL_STACK_SIZE];
volatile uint32_t redball_keyboard_errors, redball_oled_errors;
volatile uint32_t redball_input_cycles,redball_game_cycles,redball_display_cycles,redball_sound_events;
enum {SOUND_STOP=1,SOUND_JUMP,SOUND_STAR,SOUND_DEAD,SOUND_LEVEL,SOUND_WIN,SOUND_CLICK};
typedef struct {uint16_t frequency,duration_ms;} Note;
void vApplicationGetIdleTaskMemory(StaticTask_t **t,StackType_t **s,uint32_t *n) {*t=&idle_tcb;*s=idle_stack;*n=configMINIMAL_STACK_SIZE;}
void vApplicationStackOverflowHook(TaskHandle_t t,char *name) {(void)t;(void)name;Error_Handler();}
void vApplicationMallocFailedHook(void) {Error_Handler();}
static int reg_write(uint8_t reg,uint8_t value) {
 return HAL_I2C_Mem_Write(&hi2c1,0xE2,reg,I2C_MEMADD_SIZE_8BIT,&value,1,100)==HAL_OK;
}
static int reg_read(uint8_t reg,uint8_t *value) {
 return HAL_I2C_Mem_Read(&hi2c1,0xE3,reg,I2C_MEMADD_SIZE_8BIT,value,1,100)==HAL_OK;
}
static uint8_t scan_keys(void) {
 static const uint8_t row_config[4]={0xFE,0xFD,0xFB,0xF7};
 uint8_t rows[4]={0},v=0x70;
 int ok=reg_write(2,0) && reg_write(1,0);
 for(unsigned r=0;r<4 && ok;++r) {
  ok=reg_write(3,row_config[r]);
  if(ok)ok=reg_read(0,&v);
  rows[r]=(uint8_t)~v & 0x70;
 }
 ok=reg_write(3,0xFF) && ok;
 if(!ok){++redball_keyboard_errors;return 0;}
 return ((rows[1]&0x10)?KEY_LEFT:0)|((rows[1]&0x40)?KEY_RIGHT:0)|
        ((rows[0]&0x20)?KEY_JUMP:0)|((rows[1]&0x20)?KEY_PAUSE:0)|((rows[3]&0x20)?KEY_MENU:0);
}
static void InputTask(void *arg) {
 (void)arg; uint8_t last=0,stable=0;TickType_t wake=xTaskGetTickCount();
 for(;;) {
  xSemaphoreTake(bus_mutex,portMAX_DELAY);uint8_t raw=scan_keys();xSemaphoreGive(bus_mutex);
  ++redball_input_cycles;
  if(raw==last) stable=raw;
  last=raw;
  xQueueOverwrite(input_queue,&stable);
  vTaskDelayUntil(&wake,pdMS_TO_TICKS(10));
 }
}
static void GameTask(void *arg) {
 (void)arg;Game g;game_init(&g);uint8_t keys=0;TickType_t wake=xTaskGetTickCount();
 for(;;) {
  xQueuePeek(input_queue,&keys,0);
  GameMode old_mode=g.mode;uint8_t old_stars=g.stars,old_grounded=g.grounded;
  uint8_t pressed=keys & (uint8_t)~g.previous_keys;
  game_tick(&g,keys);
  ++redball_game_cycles;
  uint8_t sound=0;
  if(pressed&KEY_MENU)sound=SOUND_STOP;
  else if(old_mode!=g.mode && g.mode==GAME_WON)sound=SOUND_WIN;
  else if(old_mode!=g.mode && g.mode==GAME_DEAD)sound=SOUND_DEAD;
  else if(old_mode!=g.mode && g.mode==GAME_NEXT)sound=SOUND_LEVEL;
  else if(old_stars!=g.stars)sound=SOUND_STAR;
  else if(old_mode==GAME_PLAY && old_grounded && (pressed&KEY_JUMP) && g.vy<0)sound=SOUND_JUMP;
  else if(old_mode!=g.mode)sound=SOUND_CLICK;
  if(sound)xQueueOverwrite(sound_queue,&sound);
  xQueueOverwrite(frame_queue,&g);
  vTaskDelayUntil(&wake,pdMS_TO_TICKS(20));
 }
}
static void DisplayTask(void *arg) {
 (void)arg;Game g;
 xSemaphoreTake(bus_mutex,portMAX_DELAY);
 oled_Init();
 xSemaphoreGive(bus_mutex);
 for(;;) {
  if(xQueueReceive(frame_queue,&g,portMAX_DELAY)==pdPASS) {
   game_render(&g); /* Only this task owns the framebuffer. */
   xSemaphoreTake(bus_mutex,portMAX_DELAY);
   if(HAL_I2C_IsDeviceReady(&hi2c1,OLED_I2C_ADDR,1,5)==HAL_OK)oled_UpdateScreen();
   else ++redball_oled_errors;
   xSemaphoreGive(bus_mutex);
   ++redball_display_cycles;
   vTaskDelay(pdMS_TO_TICKS(40));
  }
 }
}
static void play_notes(const Note *notes,unsigned count) {
 for(unsigned i=0;i<count;++i) {
  Buzzer_Tone(notes[i].frequency);
  vTaskDelay(pdMS_TO_TICKS(notes[i].duration_ms));
  Buzzer_Stop();vTaskDelay(pdMS_TO_TICKS(18));
 }
}
static void SoundTask(void *arg) {
 (void)arg;uint8_t event;
 static const Note jump[]={{659,55}};
 static const Note star[]={{784,70},{1047,110}};
 static const Note dead[]={{392,110},{330,140},{262,220}};
 static const Note level[]={{523,75},{659,75},{784,160}};
 static const Note click[]={{523,45}};
 static const Note win[]={{523,90},{659,90},{784,90},{1047,210},{784,90},{1047,90},{1319,300}};
 Buzzer_Init();
 for(;;)if(xQueueReceive(sound_queue,&event,portMAX_DELAY)==pdPASS) {
  ++redball_sound_events;
  if(event==SOUND_STOP){Buzzer_Stop();continue;}
  if(event==SOUND_JUMP)play_notes(jump,sizeof jump/sizeof jump[0]);
  else if(event==SOUND_STAR)play_notes(star,sizeof star/sizeof star[0]);
  else if(event==SOUND_DEAD)play_notes(dead,sizeof dead/sizeof dead[0]);
  else if(event==SOUND_LEVEL)play_notes(level,sizeof level/sizeof level[0]);
  else if(event==SOUND_WIN)play_notes(win,sizeof win/sizeof win[0]);
  else if(event==SOUND_CLICK)play_notes(click,sizeof click/sizeof click[0]);
 }
}
void MX_FREERTOS_Init(void) {
 bus_mutex=xSemaphoreCreateMutex();
 input_queue=xQueueCreate(1,sizeof(uint8_t));frame_queue=xQueueCreate(1,sizeof(Game));sound_queue=xQueueCreate(1,sizeof(uint8_t));
 if(!bus_mutex || !input_queue || !frame_queue || !sound_queue)Error_Handler();
 uint8_t keys=0;xQueueOverwrite(input_queue,&keys);
 /* Use the same native FreeRTOS creation pattern as the working SDK_RedBall. */
 if(xTaskCreate(InputTask,"Input",256,NULL,3,NULL)!=pdPASS)Error_Handler();
 if(xTaskCreate(GameTask,"Game",512,NULL,2,NULL)!=pdPASS)Error_Handler();
 if(xTaskCreate(DisplayTask,"Display",512,NULL,1,NULL)!=pdPASS)Error_Handler();
 if(xTaskCreate(SoundTask,"Sound",256,NULL,1,NULL)!=pdPASS)Error_Handler();
}
