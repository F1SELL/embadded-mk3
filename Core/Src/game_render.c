#include "game_render.h"
#include "oled.h"
#include <string.h>

static void raw_pixel(int x,int y,OLED_COLOR c) {
 if(x>=0 && x<128 && y>=0 && y<64)oled_DrawPixel((uint8_t)x,(uint8_t)y,c);
}
static void world_pixel(int x,int y,OLED_COLOR c) {
 if(y>=12)raw_pixel(x,y,c);
}
static void text(int x,int y,char *s) {oled_SetCursor((uint8_t)x,(uint8_t)y);oled_WriteString(s,Font_7x10,White);}
static void text_center(int y,char *s) {
 int x=(128-(int)strlen(s)*7)/2;if(x<0)x=0;text(x,y,s);
}
static void line_h(int x1,int x2,int y,OLED_COLOR c) {for(int x=x1;x<=x2;++x)raw_pixel(x,y,c);}
static void line_v(int x,int y1,int y2,OLED_COLOR c) {for(int y=y1;y<=y2;++y)raw_pixel(x,y,c);}
static void rect(int x1,int y1,int x2,int y2,OLED_COLOR c) {
 line_h(x1,x2,y1,c);line_h(x1,x2,y2,c);line_v(x1,y1,y2,c);line_v(x2,y1,y2,c);
}
static void sparkle(int x,int y,int phase) {
 raw_pixel(x,y,White);
 int radius=phase?3:2;
 raw_pixel(x-radius,y,White);raw_pixel(x+radius,y,White);
 raw_pixel(x,y-radius,White);raw_pixel(x,y+radius,White);
 if(phase){raw_pixel(x-1,y-1,White);raw_pixel(x+1,y+1,White);}
}
static void panel(char *a,char *b) {
 for(int y=17;y<=52;++y)for(int x=1;x<127;++x)raw_pixel(x,y,Black);
 rect(1,17,126,52,White);
 text_center(22,a);text_center(38,b);
}
static void draw_background(const Game *g,int camera) {
 if(g->level==0) {
  for(int x=-24-(camera/4)%48;x<152;x+=48) {
   for(int i=0;i<22;++i){world_pixel(x+i,51-i/3,White);world_pixel(x+22+i,44+i/3,White);}
  }
  int cloud=105-(camera/3)%150;
  line_h(cloud,cloud+15,20,White);line_h(cloud+3,cloud+11,18,White);
 } else if(g->level==1) {
  for(int x=-32-(camera/3)%64;x<160;x+=64) {
   for(int i=0;i<32;++i)world_pixel(x+i,48-i/2,White);
   for(int i=0;i<32;++i)world_pixel(x+32+i,32+i/2,White);
  }
  for(int x=5;x<128;x+=19)world_pixel(x,16+((x+camera/5)%9),White);
 } else {
  for(int i=0;i<12;++i) {
   int x=(i*37-camera/5)&127;int y=14+(i*17)%25;
   raw_pixel(x,y,White);if(((g->frame/8+i)&1)==0)raw_pixel(x+1,y,White);
  }
  int cloud=90-(camera/2)%160;
  line_h(cloud,cloud+21,44,White);line_h(cloud+4,cloud+16,42,White);
  line_h(cloud+7,cloud+13,40,White);
 }
}
static void draw_platform(GameRect r,int camera,int level) {
 int left=r.x-camera,right=r.x+r.w-camera;
 for(int x=left;x<right;++x) {
  world_pixel(x,r.y,White);
  if(r.h>3)world_pixel(x,r.y+1,White);
  for(int y=r.y+2;y<r.y+r.h;++y) {
   int pattern=level==2?((x+y)%7==0):level==1?((x-y)%6==0):((x+y)&3)==0;
   if(pattern)world_pixel(x,y,White);
  }
 }
}
static void draw_spikes(GameRect r,int camera) {
 int left=r.x-camera;
 for(int x=0;x<r.w;++x) {
  int tooth=x%6;int height=tooth<3?tooth+1:6-tooth;
  for(int y=0;y<height+1;++y)world_pixel(left+x,r.y+4-y,White);
 }
}
static void draw_star(int x,int y,int camera,int phase) {
 x-=camera;
 raw_pixel(x,y,White);
 for(int d=1;d<=3;++d){raw_pixel(x-d,y,White);raw_pixel(x+d,y,White);raw_pixel(x,y-d,White);raw_pixel(x,y+d,White);}
 raw_pixel(x-1,y-1,White);raw_pixel(x+1,y-1,White);raw_pixel(x-1,y+1,White);raw_pixel(x+1,y+1,White);
 if(phase){raw_pixel(x-4,y,White);raw_pixel(x+4,y,White);}
}
static void draw_flag(int x,int camera,int frame) {
 x-=camera;
 line_v(x,24,56,White);
 for(int y=25;y<34;++y)for(int dx=1;dx<11-(y-25);++dx)
  if(((dx+y+(frame/8))&1)==0)raw_pixel(x+dx,y,White);
 raw_pixel(x-1,56,White);raw_pixel(x+1,56,White);
}
static void draw_ball(const Game *g,int camera) {
 int bx=g->x/256-camera,by=g->y/256;
 for(int y=-4;y<=4;++y)for(int x=-4;x<=4;++x)if(x*x+y*y<=18)world_pixel(bx+x,by+y,White);
 raw_pixel(bx-2,by-3,Black);
 int look=g->vx<0?-1:1;
 raw_pixel(bx+look,by-1,Black);raw_pixel(bx+look*3,by-1,Black);
 raw_pixel(bx+look,by+2,Black);raw_pixel(bx+look*2,by+3,Black);
}
static unsigned star_count(uint8_t mask) {return (mask&1)+((mask>>1)&1)+((mask>>2)&1);}
static void draw_hud(const Game *g) {
 char hud[]="L1 S0 D000 0M";unsigned stars=star_count(g->stars);
 hud[1]='1'+g->level;hud[4]='0'+stars;
 unsigned d=g->deaths%1000;hud[7]='0'+d/100;hud[8]='0'+d/10%10;hud[9]='0'+d%10;
 text(0,0,hud);line_h(0,127,11,White);
}
static void title_screen(const Game *g) {
 (void)g;oled_Fill(Black);rect(0,0,127,63,White);
 sparkle(10,10,1);sparkle(116,51,0);sparkle(107,12,0);
 for(int y=-7;y<=7;++y)for(int x=-7;x<=7;++x)if(x*x+y*y<=45)raw_pixel(19+x,27+y,White);
 raw_pixel(20,25,Black);raw_pixel(24,25,Black);raw_pixel(22,30,Black);
 text(38,5,"RED BALL");text(38,17,"3 LEVELS");
 text(7,36,"2 START 4/6 MOVE");text(7,49,"2 JUMP 5 PAUSE");
}
static void win_screen(const Game *g) {
 oled_Fill(Black);rect(0,0,127,63,White);
 for(int i=0;i<18;++i) {
  int x=(i*29+(g->frame/2))&127;int y=3+(i*19+g->frame)%57;
  if(i&1){raw_pixel(x,y,White);raw_pixel(x+1,y+1,White);}else sparkle(x,y,(g->frame/6+i)&1);
 }
 for(int y=13;y<51;++y)for(int x=9;x<119;++x)raw_pixel(x,y,Black);
 rect(8,12,119,52,White);
 text_center(16,"SUPER PUPER!");text_center(29,"YOU DID IT!");text_center(42,"ALL 3 LEVELS!");
 text_center(53,"2 AGAIN 0 MENU");
}
void game_render(const Game *g) {
 if(g->mode==GAME_TITLE){title_screen(g);return;}
 if(g->mode==GAME_WON){win_screen(g);return;}
 oled_Fill(Black);
 int camera=g->x/256-42;if(camera<0)camera=0;if(camera>GAME_WORLD_WIDTH-128)camera=GAME_WORLD_WIDTH-128;
 const GameLevel *l=&game_levels[g->level];
 draw_background(g,camera);
 for(unsigned i=0;i<l->platform_count;++i)draw_platform(l->platforms[i],camera,g->level);
 for(unsigned i=0;i<l->spike_count;++i)draw_spikes(l->spikes[i],camera);
 for(unsigned i=0;i<GAME_STAR_COUNT;++i)if(!(g->stars&(1u<<i)))draw_star(l->stars[i].x,l->stars[i].y,camera,(g->frame/8+i)&1);
 draw_flag(l->finish_x,camera,g->frame);draw_ball(g,camera);draw_hud(g);
 if(g->mode==GAME_PLAY && g->frame<35) {
  if(g->level==0)panel("LEVEL 1","GREEN HILLS");
  else if(g->level==1)panel("LEVEL 2","DANGER CANYON");
  else panel("LEVEL 3","SKY TOWER");
 }
 if(g->mode==GAME_PAUSED)panel("PAUSED","5 CONT  0 MENU");
 if(g->mode==GAME_DEAD)panel("GAME OVER!","2 RETRY 0 MENU");
 if(g->mode==GAME_NEXT)panel("LEVEL COMPLETE!","2 NEXT  0 MENU");
}
