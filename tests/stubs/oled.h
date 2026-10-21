#include "fonts.h"
typedef enum {Black,White} OLED_COLOR;
void oled_Fill(OLED_COLOR color);
void oled_DrawPixel(uint8_t x,uint8_t y,OLED_COLOR color);
void oled_SetCursor(uint8_t x,uint8_t y);
char oled_WriteString(char *str,FontDef font,OLED_COLOR color);
