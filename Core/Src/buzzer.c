#include "buzzer.h"
#include "tim.h"

#define BUZZER_PERIOD 99u
#define BUZZER_VOLUME 34u
static uint8_t buzzer_ready;

void Buzzer_Init(void) {
 __HAL_TIM_SET_AUTORELOAD(&htim2,BUZZER_PERIOD);
 __HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_1,0);
 /* Sound is optional: a PWM fault must never stop the RTOS and keyboard. */
 buzzer_ready=(HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_1)==HAL_OK);
}
void Buzzer_Tone(uint16_t frequency) {
 if(!buzzer_ready)return;
 if(frequency==0){Buzzer_Stop();return;}
 uint32_t timer_clock=HAL_RCC_GetPCLK1Freq();
 RCC_ClkInitTypeDef clocks;uint32_t latency;
 HAL_RCC_GetClockConfig(&clocks,&latency);
 if(clocks.APB1CLKDivider!=RCC_HCLK_DIV1)timer_clock*=2u;
 uint32_t divisor=timer_clock/((BUZZER_PERIOD+1u)*(uint32_t)frequency);
 if(divisor==0)divisor=1;
 __HAL_TIM_SET_PRESCALER(&htim2,divisor-1u);
 __HAL_TIM_SET_AUTORELOAD(&htim2,BUZZER_PERIOD);
 __HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_1,BUZZER_VOLUME);
 __HAL_TIM_SET_COUNTER(&htim2,0);
 htim2.Instance->EGR=TIM_EGR_UG;
}
void Buzzer_Stop(void) {
 if(!buzzer_ready)return;
 __HAL_TIM_SET_COMPARE(&htim2,TIM_CHANNEL_1,0);
}
