"""Tests call the exact firmware game.c compiled as a host shared library."""
import ctypes as c
from pathlib import Path
import json
class Game(c.Structure):
    _fields_=[('x',c.c_int32),('y',c.c_int32),('vx',c.c_int32),('vy',c.c_int32),('deaths',c.c_uint16),('frame',c.c_uint16),('level',c.c_uint8),('stars',c.c_uint8),('grounded',c.c_uint8),('previous_keys',c.c_uint8),('mode',c.c_int)]
lib=c.CDLL(str(Path(__file__).with_name('game.dylib')))
lib.game_init.argtypes=[c.POINTER(Game)]
lib.game_tick.argtypes=[c.POINTER(Game),c.c_uint8]
def tick(g,k,n=1):
    for _ in range(n):lib.game_tick(c.byref(g),k)
def new():
    g=Game();lib.game_init(c.byref(g));return g

g=new();assert g.mode==0
tick(g,4);tick(g,0,10);assert g.mode==1 and g.grounded
y=g.y;tick(g,4);assert g.y<y and not g.grounded
vy=g.vy;tick(g,4);assert g.vy>vy # holding does not retrigger
# Pause freezes simulation; only a new press resumes.
tick(g,8); frozen=(g.x,g.y,g.vy);tick(g,2,10);assert (g.x,g.y,g.vy)==frozen and g.mode==2
tick(g,8);assert g.mode==1
# Key 0 returns to the title menu from any gameplay state.
tick(g,16);assert g.mode==0 and g.level==0
# Falling and spikes cause death; retry resets the level.
g=new();g.mode=1;g.x=108*256;g.y=77*256;tick(g,0);assert g.mode==3 and g.deaths==1
tick(g,4);assert g.mode==1 and g.x==16*256
hazards=[(145,51),(108,45),(40,51)]
for level in range(3):
    g=new();g.mode=1;g.level=level;g.x=hazards[level][0]*256;g.y=hazards[level][1]*256
    tick(g,0);assert g.mode==3
# Stars and level transitions.
g=new();g.mode=1;g.x=65*256;g.y=29*256;tick(g,0);assert g.stars&1
g.x=370*256;g.y=52*256;tick(g,0);assert g.mode==4
tick(g,4);assert g.level==1 and g.mode==1
g.x=372*256;g.y=52*256;tick(g,0);assert g.mode==4
tick(g,4);assert g.level==2 and g.mode==1
g.x=372*256;g.y=52*256;tick(g,0);assert g.mode==5
tick(g,4);assert g.level==0 and g.mode==1
# Search valid routes, including every star, using the real collision code.
routes=[]
star_positions=[[65,171,291],[122,196,281],[49,109,250]]
for level in range(3):
    g=new();g.mode=1;g.level=level;tick(g,0,10)
    beam=[(g,[])];visited=set();winner=None
    for step in range(220):
        candidates=[]
        for state,path in beam:
            for action in [0,2,6,1,5,4]:
                h=Game.from_buffer_copy(state);tick(h,action,4)
                if h.mode in (4,5):
                    if h.stars==7:winner=path+[action];break
                    continue
                if h.mode!=1:continue
                key=(h.x//256,h.y//256,h.vx//100,h.vy//100,h.grounded,h.stars,h.previous_keys)
                if key in visited:continue
                visited.add(key)
                # Missing stars behind the player are expensive, but backtracking is allowed.
                stars=star_positions[level]
                missing=sum(1 for i,x in enumerate(stars) if not h.stars&(1<<i) and h.x//256>x+10)
                score=h.x/256+60*bin(h.stars).count('1')-180*missing
                candidates.append((score,h,path+[action]))
            if winner:break
        if winner:break
        candidates.sort(key=lambda v:v[0],reverse=True)
        beam=[(g,p) for _,g,p in candidates[:1200]]
        if not beam:break
    assert winner, f'No complete route for level {level+1}'
    routes.append(winner)
    print(f'Level {level+1}: all 3 stars and finish reached in {len(winner)*4} physics ticks')
Path(__file__).with_name('routes.json').write_text(json.dumps(routes))
print('PASS: movement, jump, pause, menu, hazards, retry, stars, transitions, three complete routes')
