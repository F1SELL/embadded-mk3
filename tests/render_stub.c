#include "oled.h"
#include <string.h>
unsigned char pixels[128*64];
static unsigned cx,cy;
void oled_Fill(OLED_COLOR color){memset(pixels,color?255:0,sizeof pixels);}
void oled_DrawPixel(uint8_t x,uint8_t y,OLED_COLOR color){if(x<128 && y<64)pixels[y*128+x]=color?255:0;}
void oled_SetCursor(uint8_t x,uint8_t y){cx=x;cy=y;}
char oled_WriteString(char *str,FontDef f,OLED_COLOR color){
 while(*str){if(cx+f.FontWidth>=128||cy+f.FontHeight>=64)return *str;
 for(unsigned y=0;y<f.FontHeight;y++)for(unsigned x=0;x<f.FontWidth;x++)
 oled_DrawPixel(cx+x,cy+y,((f.data[(*str-32)*f.FontHeight+y]<<x)&0x8000)?color:!color);
 cx+=f.FontWidth;str++;}return 0;
}
