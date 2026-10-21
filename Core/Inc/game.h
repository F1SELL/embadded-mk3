#ifndef REDBALL_GAME_H
#define REDBALL_GAME_H
#include <stdint.h>
#define KEY_LEFT 1u
#define KEY_RIGHT 2u
#define KEY_JUMP 4u
#define KEY_PAUSE 8u
#define KEY_MENU 16u
#define GAME_RADIUS 4
#define GAME_WORLD_WIDTH 384
#define GAME_STAR_COUNT 3
#define GAME_LEVEL_COUNT 3
typedef struct { int16_t x,y,w,h; } GameRect;
typedef struct { int16_t x,y; } GamePoint;
typedef struct {
 const GameRect *platforms, *spikes;
 uint8_t platform_count, spike_count;
 GamePoint stars[GAME_STAR_COUNT];
 int16_t finish_x;
} GameLevel;
typedef enum { GAME_TITLE, GAME_PLAY, GAME_PAUSED, GAME_DEAD, GAME_NEXT, GAME_WON } GameMode;
typedef struct {
 int32_t x,y,vx,vy; /* Q8 fixed point; one physics tick is 20 ms. */
 uint16_t deaths;
 uint16_t frame;
 uint8_t level, stars, grounded, previous_keys;
 GameMode mode;
} Game;
extern const GameLevel game_levels[GAME_LEVEL_COUNT];
void game_init(Game *g);
void game_tick(Game *g, uint8_t keys);
#endif
