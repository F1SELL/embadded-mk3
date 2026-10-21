#include "game.h"
#include <string.h>
#define Q 256
static const GameRect platforms1[] = {{0,56,98,8},{118,56,98,8},{236,56,148,8},{53,39,25,3},{158,37,27,3},{275,38,32,3}};
static const GameRect platforms2[] = {{0,56,68,8},{90,50,53,14},{167,56,57,8},{248,48,53,16},{324,56,60,8},{111,31,22,3},{186,35,21,3},{270,29,22,3}};
static const GameRect platforms3[] = {{0,56,58,8},{82,50,42,14},{148,56,47,8},{219,46,47,18},{290,56,94,8},{40,35,18,3},{100,29,20,3},{169,36,18,3},{241,26,18,3}};
static const GameRect spikes1[] = {{140,51,10,5},{252,51,12,5},{328,51,12,5}};
static const GameRect spikes2[] = {{103,45,10,5},{199,51,10,5},{257,43,10,5},{341,51,10,5}};
static const GameRect spikes3[] = {{35,51,10,5},{98,45,10,5},{169,51,10,5},{235,41,10,5},{322,51,12,5}};
const GameLevel game_levels[GAME_LEVEL_COUNT] = {
 {platforms1,spikes1,6,3,{{65,29},{171,27},{291,28}},369},
 {platforms2,spikes2,8,4,{{122,21},{196,25},{281,19}},370},
 {platforms3,spikes3,9,5,{{49,25},{109,19},{250,16}},370}
};
static void spawn(Game *g) {
 g->x=16*Q; g->y=51*Q; g->vx=g->vy=0; g->stars=0; g->grounded=0; g->frame=0; g->mode=GAME_PLAY;
}
void game_init(Game *g) { memset(g,0,sizeof(*g)); spawn(g); g->mode=GAME_TITLE; }
static int overlap(int a,int b,int c,int d) { return a<d && b>c; }
void game_tick(Game *g,uint8_t keys) {
 ++g->frame;
 uint8_t pressed=keys & (uint8_t)~g->previous_keys; g->previous_keys=keys;
 if(pressed & KEY_MENU) { game_init(g); g->previous_keys=keys; return; }
 if(g->mode==GAME_TITLE || g->mode==GAME_DEAD || g->mode==GAME_NEXT || g->mode==GAME_WON) {
  if(pressed & KEY_JUMP) {
   if(g->mode==GAME_NEXT && g->level+1<GAME_LEVEL_COUNT) ++g->level;
   if(g->mode==GAME_WON) { g->level=0; g->deaths=0; }
   spawn(g);
  }
  return;
 }
 if(pressed & KEY_PAUSE) g->mode=g->mode==GAME_PAUSED?GAME_PLAY:GAME_PAUSED;
 if(g->mode!=GAME_PLAY) return;
 const GameLevel *l=&game_levels[g->level];
 int direction=((keys & KEY_RIGHT)!=0)-((keys & KEY_LEFT)!=0);
 if(direction) { g->vx+=direction*100; if(g->vx>512)g->vx=512; if(g->vx< -512)g->vx=-512; }
 else { g->vx=g->vx*3/4; if(g->vx<20 && g->vx> -20)g->vx=0; }
 if((pressed & KEY_JUMP) && g->grounded) { g->vy=-1100; g->grounded=0; }
 g->vy+=64; if(g->vy>1280)g->vy=1280;
 int old_y=g->y;
 g->x+=g->vx;
 for(unsigned i=0;i<l->platform_count;++i) {
  GameRect r=l->platforms[i];
  if(overlap(g->y-4*Q,g->y+4*Q,r.y*Q,(r.y+r.h)*Q) && overlap(g->x-4*Q,g->x+4*Q,r.x*Q,(r.x+r.w)*Q)) {
   if(g->vx>0)g->x=(r.x-4)*Q; else if(g->vx<0)g->x=(r.x+r.w+4)*Q;
   g->vx=0;
  }
 }
 if(g->x<4*Q) { g->x=4*Q; g->vx=0; }
 if(g->x>(GAME_WORLD_WIDTH-4)*Q) {g->x=(GAME_WORLD_WIDTH-4)*Q;g->vx=0;}
 g->y+=g->vy; g->grounded=0;
 for(unsigned i=0;i<l->platform_count;++i) {
  GameRect r=l->platforms[i];
  if(!overlap(g->x-4*Q,g->x+4*Q,r.x*Q,(r.x+r.w)*Q))continue;
  if(g->vy>=0 && old_y+4*Q<=r.y*Q && g->y+4*Q>=r.y*Q) { g->y=(r.y-4)*Q;g->vy=0;g->grounded=1; }
  else if(g->vy<0 && old_y-4*Q>=(r.y+r.h)*Q && g->y-4*Q<=(r.y+r.h)*Q) {g->y=(r.y+r.h+4)*Q;g->vy=0;}
 }
 int x=g->x/Q,y=g->y/Q;
 if(y>76) {g->mode=GAME_DEAD;++g->deaths;return;}
 for(unsigned i=0;i<l->spike_count;++i) {
  GameRect r=l->spikes[i];
  if(overlap(x-3,x+3,r.x,r.x+r.w) && overlap(y-3,y+3,r.y,r.y+r.h)) {g->mode=GAME_DEAD;++g->deaths;return;}
 }
 for(unsigned i=0;i<GAME_STAR_COUNT;++i) {
  int dx=x-l->stars[i].x,dy=y-l->stars[i].y;
  if(dx*dx+dy*dy<=64)g->stars|=(1u<<i);
 }
 if(x>=l->finish_x && g->grounded)g->mode=g->level+1<GAME_LEVEL_COUNT?GAME_NEXT:GAME_WON;
}
