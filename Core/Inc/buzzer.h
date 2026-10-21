#ifndef REDBALL_BUZZER_H
#define REDBALL_BUZZER_H

#include <stdint.h>

void Buzzer_Init(void);
void Buzzer_Tone(uint16_t frequency);
void Buzzer_Stop(void);

#endif
