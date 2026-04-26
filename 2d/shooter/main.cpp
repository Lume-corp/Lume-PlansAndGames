#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "opengl32.lib")
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>
extern "C" int _fltused = 0;
#pragma function(memset)
extern "C" void* memset(void* dest, int c, size_t count) {
    unsigned char* bytes = (unsigned char*)dest;
    while (count--) *bytes++ = (unsigned char)c;
    return dest;
}
#define PI 3.1415926535f
float f_abs(float x) { return x < 0.0f ? -x : x; }
float f_min(float a, float b) {return a < b ? a : b;}
float f_max(float a, float b) {return a > b ? a : b;}
float f_mod(float x, float y) {
    float res = x - (int)(x / y) * y;
    return res < 0.0f ? res + y : res;
}
float f_sqrt(float x) {
    if (x <= 0.0f) return 0.0f;
    float res = x;
    for (int i = 0; i < 10; ++i) res = res - (res * res - x) / (2.0f * res);
    return res;
}
float f_sin(float x) {
    x = f_mod(x + PI, 2.0f * PI) - PI;
    float x2 = x * x;
    return x * (1.0f - x2 / 6.0f + x2 * x2 / 120.0f - x2 * x2 * x2 / 5040.0f);
}
float f_cos(float x) {return f_sin(x + PI / 2.0f);}
float f_atan2(float y, float x) {
    float abs_y = f_abs(y) + 1e-10f;
    float r, angle;
    if (x >= 0.0f) { r = (x - abs_y) / (x + abs_y); angle = (PI / 4.0f) * (1.0f - r);}
    else { r = (x + abs_y) / (abs_y - x); angle = (3.0f * PI / 4.0f) - (PI / 4.0f) * r;}
    return y < 0.0f ? -angle : angle;
}
unsigned int r_seed = 12345;
int rand_int() { r_seed = r_seed * 1664525 + 1013904223; return (int)(r_seed >> 16) & 0x7FFF;}
int rand_range(int min, int max) { return min + rand_int() % (max - min + 1);}
float rand_float(float min, float max) { return min + (float)rand_int() / 32767.0f * (max - min);}
struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float x, float y) : x(x), y(y) {}
    Vec2 operator+(Vec2 o) const {return Vec2(x + o.x, y + o.y);}
    Vec2 operator-(Vec2 o) const {return Vec2(x - o.x, y - o.y);}
    Vec2 operator*(float s) const {return Vec2(x * s, y * s);}
    Vec2 operator/(float s) const {return Vec2(x / s, y / s);}
    float dot(Vec2 o) const {return x * o.x + y * o.y;}
    float length() const {return f_sqrt(x * x + y * y);}
    Vec2 normalize() const { float l = length(); return l > 0 ? *this / l : Vec2(0, 0);}
    float dist(Vec2 o) const {return (*this - o).length();}
    Vec2 lerp(Vec2 o, float t) const {return *this + (o - *this) * t;}
    Vec2 rotate(float deg) const {
        float r = deg * PI / 180.0f, c = f_cos(r), s = f_sin(r);
        return Vec2(x * c - y * s, x * s + y * c);
    }
};
float dist_point_line(Vec2 p, Vec2 l1, Vec2 l2) {
    Vec2 line_vec = l2 - l1;
    Vec2 p_vec = p - l1;
    float len_sq = line_vec.dot(line_vec);
    if (len_sq == 0.0f) return p.dist(l1);
    float t = f_max(0.0f, f_min(1.0f, p_vec.dot(line_vec) / len_sq));
    return p.dist(l1 + line_vec * t);
}
int str_len(const char* s) { int l = 0; while (s[l]) l++; return l;}
void str_cpy(char* d, const char* s) { while (*s) *d++ = *s++; *d = 0;}
void str_cat(char* d, const char* s) { int l = str_len(d); while (*s) d[l++] = *s++; d[l] = 0;}
void itoa(int v, char* b) {
    if (v == 0) { b[0] = '0'; b[1] = 0; return;}
    char t[32]; int i = 0, j = 0;
    bool neg = v < 0; if (neg) v = -v;
    while (v > 0) { t[i++] = (v % 10) + '0'; v /= 10;}
    if (neg) t[i++] = '-';
    while (i > 0) b[j++] = t[--i]; b[j] = 0;
}
void ftoa(float v, char* b) {
    itoa((int)v, b); str_cat(b, ".");
    int frac = (int)(f_abs(v - (int)v) * 10.0f);
    char fbuf[8]; itoa(frac, fbuf); str_cat(b, fbuf);
}
int WIDTH = 800, HEIGHT = 600;
Vec2 mouse_pos;
bool lmb_down = false, rmb_down = false;
struct Col { float r, g, b;};
Col C_BLACK={0.08f,0.08f,0.08f}, C_WHITE={1,1,1}, C_RED={0.86f,0.2f,0.2f}, C_GREEN={0.2f,0.86f,0.2f},C_BLUE={0.2f,0.6f,1.0f}, C_CYAN={0,1,1}, C_YELLOW={1,0.86f,0.2f}, C_PURPLE={0.6f,0.2f,0.8f},C_MAGENTA={1,0,1}, C_ORANGE={1,0.47f,0}, C_GRAY={0.6f,0.6f,0.6f}, C_BLOOD={0.7f,0,0};
void SetCol(Col c, float a=1.0f) { glColor4f(c.r, c.g, c.b, a); }
void DrawCircle(Vec2 p, float r, Col c, bool wire = false, float w = 1.0f) {
    glLineWidth(w); SetCol(c); glBegin(wire ? GL_LINE_LOOP : GL_TRIANGLE_FAN);
    if (!wire) glVertex2f(p.x, p.y);
    for (int i=0; i<=32; i++) { float a = i*2.0f*PI/32.0f; glVertex2f(p.x+f_cos(a)*r, p.y+f_sin(a)*r);}
    glEnd();
}
void DrawEllipse(Vec2 p, float rx, float ry, Col c, bool wire = false, float w = 1.0f) {
    glLineWidth(w); SetCol(c); glBegin(wire ? GL_LINE_LOOP : GL_TRIANGLE_FAN);
    if (!wire) glVertex2f(p.x, p.y);
    for (int i = 0; i <= 32; i++) { float a = i * 2.0f * PI / 32.0f; glVertex2f(p.x + f_cos(a) * rx, p.y + f_sin(a) * ry);}
    glEnd();
}
void DrawRect(Vec2 p, float w, float h, Col c, bool wire = false) {
    SetCol(c); glBegin(wire ? GL_LINE_LOOP : GL_QUADS);
    glVertex2f(p.x, p.y); glVertex2f(p.x+w, p.y); glVertex2f(p.x+w, p.y+h); glVertex2f(p.x, p.y+h);
    glEnd();
}
void DrawLine(Vec2 p1, Vec2 p2, Col c, float w = 1.0f) {
    glLineWidth(w); SetCol(c); glBegin(GL_LINES); glVertex2f(p1.x, p1.y); glVertex2f(p2.x, p2.y); glEnd();
}
void DrawTextStr(float x, float y, const char* s, Col c) {
    SetCol(c); glRasterPos2f(x, y + 16); glCallLists(str_len(s), GL_UNSIGNED_BYTE, s);
}

struct Player {
    Vec2 pos, vel, dash_dir;
    float speed = 5.5f, radius = 15.0f;
    int hp = 100, max_hp = 100, weapon = 1;
    int shoot_cd = 0, railgun_cd = 0;
    int nail_ammo = 100, nail_max = 100, nail_regen = 0;
    int coins = 4, coin_timer = 0;
    int dash_timer = 0, dash_cd = 0;
} P;
#define MAX_E 2000
template<typename T> T* Spawn(T* arr) {
    for (int i=0; i<MAX_E; i++) if (!arr[i].active) { memset(&arr[i], 0, sizeof(T)); arr[i].active=true; return &arr[i];}
    return nullptr;
}
struct FText { bool active; Vec2 pos; char t[64]; Col c; int timer; float spd;};
struct Coin { bool active; Vec2 pos, vel; int age; float radius;};
struct Bullet { 
    bool active; Vec2 pos, vel; int type, timer, age, dmg; float radius;
    Vec2 hist[10]; int h_idx, h_cnt;
};
struct Magnet { bool active; Vec2 pos; int timer; float radius, orbit;};
struct Explode { bool active; Vec2 pos; float r, max_r; int dmg; bool is_enemy, boss_hit;};
struct Blood { bool active; Vec2 pos; int timer;};
struct Beam { bool active; Vec2 s, e; int timer; Col c; float w;};
struct SBeam { bool active; Vec2 pos; float radius; int timer; bool dmg_done, is_boss, vertical;};
struct RBeam { bool active; Vec2 o; float ang, hw, spin_spd; int timer, lock, fire; bool p_beam;};
struct Enemy {
    bool active; Vec2 pos; int type, hp, max_hp, shoot_cd, burst_cnt, burst_cd;
    float radius, speed, ang, spin_spd;
    int spin_tmr, lock_tmr; Col c; bool attacking;
};
struct BossData {
    bool active; Vec2 pos, vel, move_target; int hp, max_hp; float radius, pulse;
    int phase_tmr, attack_tmr, attack_phase, move_tmr;
    int bq_cnt; SBeam bq[10]; Vec2 bq_pos; int fan_side;
} B;
FText ftexts[MAX_E]; Coin coins[MAX_E]; Bullet bullets[MAX_E]; Bullet ebullets[MAX_E];
Magnet mags[MAX_E]; Explode expls[MAX_E]; Blood bloods[MAX_E]; Enemy enemies[MAX_E];
Beam beams[MAX_E]; SBeam sbeams[MAX_E]; RBeam rbeams[MAX_E];
int wave=1, enemies_to_spawn=10, spawn_timer=0, wave_delay=180, score=0;
bool boss_spawned = false;
void AddText(Vec2 p, const char* s, Col c, float spd=1.0f) {
    FText* t = Spawn(ftexts); if (t) { t->pos=p; str_cpy(t->t, s); t->c=c; t->timer=60; t->spd=spd;}
}
void TriggerCoinChain(Coin* start_coin, float base_dmg, int w_type) {
    Vec2 cur_pos = start_coin->pos; start_coin->active = false;
    int hits = 1;
    while (true) {
        Vec2 t_pos; void* t_ent = nullptr; int t_type = 0; 
        float min_d = 99999.0f;
        for (int i=0; i<MAX_E; i++) if (coins[i].active) {
            float d = cur_pos.dist(coins[i].pos); if(d<min_d) { min_d=d; t_pos=coins[i].pos; t_ent=&coins[i]; t_type=1;}
        }
        if(!t_type) {
            for(int i=0; i<MAX_E; i++) if(bullets[i].active && bullets[i].type==2) { 
                float d = cur_pos.dist(bullets[i].pos); if(d<min_d) { min_d=d; t_pos=bullets[i].pos; t_ent=&bullets[i]; t_type=2;}
            }
        }
        if(!t_type && B.active) { float d = cur_pos.dist(B.pos); min_d=d; t_pos=B.pos; t_ent=&B; t_type=3;}
        if(!t_type) {
            for(int i=0; i<MAX_E; i++) if(enemies[i].active) {
                float d = cur_pos.dist(enemies[i].pos); if(d<min_d) { min_d=d; t_pos=enemies[i].pos; t_ent=&enemies[i]; t_type=4;}
            }
        }
        if(!t_type) { t_pos = cur_pos + Vec2(1,0).rotate(rand_float(0,360))*1500.0f; t_type=5;}
        
        Col c = (w_type==1) ? C_YELLOW : C_CYAN; float w = (w_type==1)?4.0f:8.0f;
        if(t_type == 1) {
            Beam* bm = Spawn(beams); if(bm){ bm->s=cur_pos; bm->e=t_pos; bm->timer=15; bm->c=c; bm->w=w;}
            cur_pos = t_pos; ((Coin*)t_ent)->active = false; hits++;
            char buf[32]; str_cpy(buf, "x"); float m=1.0f+0.1f*hits; ftoa(m, buf+1); str_cat(buf, " MULTI!");
            AddText(cur_pos, buf, C_YELLOW, 2.0f);
        } else if(t_type == 2) {
            Beam* bm = Spawn(beams); if(bm){ bm->s=cur_pos; bm->e=t_pos; bm->timer=15; bm->c=c; bm->w=w;}
            ((Bullet*)t_ent)->active = false;
            Explode* ex = Spawn(expls); 
            if(ex){ ex->pos=t_pos; ex->max_r=(w_type==4)?300.f:150.f; ex->r=5; ex->dmg=(w_type==4)?200:80;}
            AddText(t_pos, "CORE SNIPE!", C_RED, 3.0f); break;
        } else {
            Vec2 e_pos = cur_pos + (t_pos - cur_pos).normalize() * 3000.0f;
            Beam* bm = Spawn(beams); if(bm){ bm->s=cur_pos; bm->e=e_pos; bm->timer=15; bm->c=c; bm->w=w;}
            float f_dmg = base_dmg * (1.0f + 0.1f*hits); bool hit_any=false;
            for(int i=0; i<MAX_E; i++) if(enemies[i].active && dist_point_line(enemies[i].pos, cur_pos, e_pos)<20.0f) {
                enemies[i].hp -= (int)f_dmg; hit_any=true;
            }
            if(B.active && dist_point_line(B.pos, cur_pos, e_pos) < B.radius){
                B.hp -= (int)f_dmg; hit_any=true; 
                char dmgBuf[32]; itoa((int)f_dmg, dmgBuf); str_cat(dmgBuf, " PIERCE!");
                AddText(B.pos + Vec2(rand_float(-20,20), -30), dmgBuf, C_YELLOW);
            }
            if(hit_any && t_type==4) {
                char dmgBuf[32]; itoa((int)f_dmg, dmgBuf); str_cat(dmgBuf, " PIERCE!");
                AddText(t_pos, dmgBuf, C_YELLOW, 3.0f);
            }
            break;
        }
    }
}
void UpdatePlayer() {
    Vec2 md(0,0);
    if(GetAsyncKeyState('W')) md.y-=1; if(GetAsyncKeyState('S')) md.y+=1;
    if(GetAsyncKeyState('A')) md.x-=1; if(GetAsyncKeyState('D')) md.x+=1;
    if(md.length() > 0) md = md.normalize();
    if(GetAsyncKeyState('1')) P.weapon=1; if(GetAsyncKeyState('2')) P.weapon=2;
    if(GetAsyncKeyState('3')) P.weapon=3; if(GetAsyncKeyState('4')) P.weapon=4;
    if(GetAsyncKeyState(VK_SPACE) && P.dash_cd<=0 && md.length()>0) {
        P.dash_timer=10; P.dash_cd=60; P.dash_dir=md;
    }
    if(P.dash_timer > 0) { P.vel = P.dash_dir*20.0f; P.dash_timer--;}
    else P.vel = md * P.speed;
    P.pos = P.pos + P.vel;
    P.pos.x = f_max(P.radius, f_min(WIDTH - P.radius, P.pos.x));
    P.pos.y = f_max(P.radius, f_min(HEIGHT - P.radius, P.pos.y));
    if(P.dash_cd>0) P.dash_cd--; if(P.shoot_cd>0) P.shoot_cd--; if(P.railgun_cd>0) P.railgun_cd--;
    if(P.nail_ammo < P.nail_max) { if(--P.nail_regen<=0){ P.nail_ammo++; P.nail_regen=30;}}
    if(P.coins < 4) { if(--P.coin_timer<=0){ P.coins++; P.coin_timer=180;}}
    Vec2 aim = (mouse_pos - P.pos).normalize();
    bool lmb = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
    bool rmb = GetAsyncKeyState(VK_RBUTTON) & 0x8000;
    if (rmb && !rmb_down) {
        if(P.weapon==1 && P.coins>0) { Coin* c=Spawn(coins); if(c){c->pos=P.pos; c->vel=aim*14.0f; c->radius=8; P.coins--;}}
        else if(P.weapon==2) {
            bool core_found = false;
            for(int i=0; i<MAX_E; i++) if(bullets[i].active && bullets[i].type==2) {
                bullets[i].active=false; Explode* ex=Spawn(expls);
                if(ex){ ex->pos=bullets[i].pos; ex->max_r=150; ex->r=5; ex->dmg=60;} core_found=true;
            }
            if(!core_found && P.shoot_cd<=0) {
                Bullet* b=Spawn(bullets); if(b){b->pos=P.pos; b->vel=aim*18.0f; b->type=2; b->radius=12; b->timer=120;}
                P.shoot_cd=40;
            }
        }
        else if(P.weapon==3) { Magnet* m=Spawn(mags); if(m){m->pos=mouse_pos; m->timer=300; m->radius=250; m->orbit=90;}}
    } rmb_down = rmb;
    if(lmb && !lmb_down) {
        if(P.weapon==4 && P.railgun_cd<=0) {
            Vec2 e = P.pos + aim*3000.0f;
            Coin* cc=nullptr; Magnet* cm=nullptr; float dc=9999, dm=9999;
            for(int i=0; i<MAX_E; i++) {
                if(coins[i].active && dist_point_line(coins[i].pos, P.pos, e)<20) { float d=P.pos.dist(coins[i].pos); if(d<dc){dc=d; cc=&coins[i];}}
                if(mags[i].active && dist_point_line(mags[i].pos, P.pos, e)<20) { float d=P.pos.dist(mags[i].pos); if(d<dm){dm=d; cm=&mags[i];}}
            }
            if(cc && (!cm || dc<dm)) {
                e = cc->pos; Beam* b=Spawn(beams); if(b){b->s=P.pos; b->e=e; b->c=C_BLUE; b->w=10; b->timer=15;}
                TriggerCoinChain(cc, 100, 4);
            } else if (cm) {
                e = cm->pos; Beam* b=Spawn(beams); if(b){b->s=P.pos; b->e=e; b->c=C_BLUE; b->w=10; b->timer=15;}
                for(int i=0; i<MAX_E; i++) if(enemies[i].active && dist_point_line(enemies[i].pos, P.pos, e)<20) enemies[i].hp-=100;
                for(int i=0; i<MAX_E; i++) if(mags[i].active && &mags[i]!=cm) {
                    Beam* b2=Spawn(beams); if(b2){b2->s=cm->pos; b2->e=mags[i].pos; b2->c=C_BLUE; b2->w=10; b2->timer=15;}
                    for(int j=0; j<MAX_E; j++) if(enemies[j].active && dist_point_line(enemies[j].pos, cm->pos, mags[i].pos)<20) enemies[j].hp-=100;
                }
                AddText(cm->pos, "MAGNETIC WEB!", C_CYAN, 3);
            } else {
                Beam* b=Spawn(beams); if(b){b->s=P.pos; b->e=e; b->c=C_BLUE; b->w=10; b->timer=15;}
                for(int i=0; i<MAX_E; i++) if(enemies[i].active && dist_point_line(enemies[i].pos, P.pos, e)<20) enemies[i].hp-=100;
                for(int i=0; i<MAX_E; i++) if(bullets[i].active && bullets[i].type==2 && dist_point_line(bullets[i].pos, P.pos, e)<50) {
                    bullets[i].active=false; Explode* ex=Spawn(expls); if(ex){ex->pos=bullets[i].pos; ex->max_r=300; ex->r=5; ex->dmg=200;}
                }
            }
            P.railgun_cd=600; P.shoot_cd=30;
        }
    } lmb_down = lmb;
    if(lmb) {
        if(P.weapon==1 && P.shoot_cd<=0) {
            Bullet* b=Spawn(bullets); if(b){b->pos=P.pos; b->vel=aim*20.0f; b->radius=6; b->dmg=15; b->type=1; b->timer=150;}
            P.shoot_cd=30;
        } else if(P.weapon==2 && P.shoot_cd<=0) {
            for(int i=0; i<7; i++) {
                Bullet* b=Spawn(bullets);
                if(b){b->pos=P.pos; b->vel=aim.rotate(rand_float(-15,15))*rand_float(9,15); b->radius=6; b->dmg=8; b->type=3; b->timer=150;}
            } P.shoot_cd=60;
        } else if(P.weapon==3 && P.shoot_cd<=0 && P.nail_ammo>0) {
            Bullet* b=Spawn(bullets); if(b){b->pos=P.pos; b->vel=aim.rotate(rand_float(-10,10))*20.0f; b->radius=6; b->dmg=6; b->type=4; b->timer=150;}
            P.nail_ammo--; P.shoot_cd=6;
        }
    }
}
void SpawnBoss() {
    B.active=true; B.pos=Vec2(WIDTH/2.0f, 120.0f); B.hp=B.max_hp=300; B.radius=40; 
    B.attack_phase=0; B.move_tmr=60; B.attack_tmr=100; boss_spawned=true; enemies_to_spawn=0;
    AddText(Vec2(WIDTH/2.0f-50, HEIGHT/2.0f), "*** BOSS ***", C_MAGENTA, 4);
}
void UpdateGame() {
    UpdatePlayer();
    if(wave_delay > 0) wave_delay--;
    else {
        if(wave==10 && !boss_spawned && !B.active) SpawnBoss();
        else if(!B.active) {
            if(enemies_to_spawn>0) {
                if(--spawn_timer<=0) {
                    Enemy* e=Spawn(enemies);
                    if(e) {
                        e->pos=Vec2(rand_float(50,WIDTH-50), -50);
                        int t = 0;
                        int roll = rand_int() % 100;
                        if (wave >= 16) {
                            if(roll < 30) t = 0; else if(roll < 55) t = 2; else if(roll < 75) t = 6; else if(roll < 93) t = 5; else t = 7;
                        } else if (wave >= 15) {
                            if(roll < 30) t = 0; else if(roll < 65) t = 5; else if(roll < 90) t = 6; else t = 7;
                        } else if (wave == 14) {
                            t = 6;
                        } else if (wave == 13) {
                            t = 2;
                        } else if (wave == 12) {
                            if(roll < 40) t = 0; else if(roll < 75) t = 5; else t = 6;
                        } else if (wave == 11) {
                            if(roll < 50) t = 0; else t = 5;
                        } else {
                            int mage_weight = (int)f_min(40.0f, 5.0f + wave * 3.0f);
                            int rem = 100 - mage_weight;
                            int melee_w = (rem * 50) / 80;
                            if(roll < melee_w) t = 0;
                            else if(roll < rem) t = 1;
                            else {
                                int m_roll = rand_int() % 3;
                                if(m_roll == 0) t = 2; else if(m_roll == 1) t = 3; else t = 4;
                            }
                        }
                        if(t==0) { e->type=0; e->hp=45; e->speed=3.5f; e->radius=15; e->c=C_RED; }
                        else if(t==1) {e->type=1; e->hp=30; e->speed=2.0f; e->radius=15; e->c=C_ORANGE; e->shoot_cd=rand_range(40, 100);}
                        else if(t==2) {e->type=2; e->hp=25; e->speed=1.5f; e->radius=15; e->c=C_PURPLE; e->shoot_cd=rand_range(40, 100);}
                        else if(t==3) {e->type=3; e->hp=25; e->speed=1.0f; e->radius=15; e->c=C_CYAN; e->shoot_cd=rand_range(40, 100);}
                        else if(t==4) {e->type=4; e->hp=25; e->speed=1.5f; e->radius=15; e->c=C_MAGENTA; e->shoot_cd=rand_range(40, 100);}
                        else if(t==5) {e->type=5; e->hp=50; e->speed=1.2f; e->radius=18; e->c=C_ORANGE; e->shoot_cd=rand_range(80, 160);}
                        else if(t==6) {e->type=6; e->hp=40; e->speed=1.8f; e->radius=16; e->c=C_GREEN; e->burst_cd=0;}
                        else if(t==7) {e->type=7; e->hp=60; e->speed=1.0f; e->radius=20; e->c=C_MAGENTA; e->shoot_cd=rand_range(120, 200);}
                    }
                    enemies_to_spawn--; spawn_timer = f_max(15.0f, 60 - wave*4);
                }
            } else {
                bool alive=false; for(int i=0; i<MAX_E; i++) if(enemies[i].active) alive=true;
                if(!alive) { wave++; enemies_to_spawn=10+wave*4; wave_delay=180; P.hp=f_min(P.hp+20, P.max_hp); AddText(P.pos, "+20 HP", C_GREEN);}
            }
        }
    }
    if(B.active) {
        B.pulse += 0.08f;
        if(--B.move_tmr<=0) { B.move_target=Vec2(rand_float(80,WIDTH-80), rand_float(80,HEIGHT/3)); B.move_tmr=rand_range(90,180);}
        Vec2 to_tgt = B.move_target - B.pos;
        if(to_tgt.length()>3) B.vel = to_tgt.normalize()*2.5f; else B.vel=Vec2(0,0);
        B.pos = B.pos + B.vel; B.pos.x=f_max(B.radius, f_min(WIDTH-B.radius, B.pos.x)); B.pos.y=f_max(B.radius, f_min(HEIGHT/2.0f, B.pos.y));
        
        if(--B.attack_tmr<=0) {
            int ph = B.attack_phase % 3;
            if(ph==0) {
                for(int i=0; i<6; i++) {
                    Bullet* b=Spawn(ebullets); if(b) {
                        b->pos=B.pos; b->vel=(P.pos-B.pos).normalize().rotate(i*60.0f)*4.0f; 
                        b->radius=6; b->dmg=12; b->type=2; b->timer=300; 
                    }
                } AddText(B.pos, "SEARCH!", C_PURPLE, 2); B.attack_tmr=120;
            } else if(ph==1) {
                for(int i=0; i<4; i++) {
                    SBeam* sb=Spawn(sbeams); if(sb) { sb->pos=P.pos; sb->is_boss=true; sb->vertical=(i%2==0); sb->timer=-i*60; sb->radius=60;}
                } AddText(B.pos, "SKY WRATH!", C_CYAN, 2); B.attack_tmr=420;
            } else {
                B.fan_side = (rand_int() % 2 == 0) ? 1 : -1; Vec2 orig(B.fan_side==1 ? WIDTH-30 : 30, HEIGHT/2);
                Vec2 bdir(-B.fan_side, 0); float offs[] = {-40, 0, 40};
                for(int i=0; i<3; i++) {
                    for(int j=0; j<8; j++) {
                        Bullet* b=Spawn(ebullets); if(b){
                            Vec2 d = bdir.rotate(offs[i] + (j-3.5f)*7.0f);
                            if(i==0) {
                                float aa = f_atan2((P.pos-orig).y, (P.pos-orig).x)*180/PI;
                                float ca = f_atan2(d.y, d.x)*180/PI;
                                float diff = f_mod(aa - ca + 180.0f, 360.0f) - 180.0f;
                                d = d.rotate(f_max(-20.0f, f_min(20.0f, diff)));
                            }
                            b->pos=orig; b->vel=d*7.0f; b->radius=6; b->dmg=10; b->type=1; b->timer=200+i*18;
                        }
                    }
                } AddText(orig, B.fan_side==1?"RIGHT FAN!":"LEFT FAN!", C_ORANGE, 2); B.attack_tmr=150;
            }
            B.attack_phase++;
        }
        if(B.hp<=0) { B.active=false; score+=500; Blood* bd=Spawn(bloods); if(bd)bd->pos=B.pos;
            Explode* ex=Spawn(expls); if(ex){ex->pos=B.pos; ex->max_r=200; ex->r=5; ex->dmg=0;}
            AddText(B.pos, "BOSS SLAIN! +500", C_YELLOW, 4); wave++; boss_spawned=false; enemies_to_spawn=10+wave*4; wave_delay=200;
        }
    }
    for(int i=0; i<MAX_E; i++) {
        if(!enemies[i].active) continue;
        Enemy& e = enemies[i];
        if(e.hp<=0) { e.active=false; score+=10; Blood* b=Spawn(bloods); if(b){b->pos=e.pos; b->timer=600;} continue;}
        float dist = e.pos.dist(P.pos); Vec2 dir = dist>0 ? (P.pos-e.pos).normalize() : Vec2(0,1);
        
        if(e.type==0) { e.pos = e.pos + dir*e.speed; }
        else if(e.type==1) {
            if(dist>300) e.pos = e.pos + dir*e.speed;
            if(--e.shoot_cd<=0) { Bullet* b=Spawn(ebullets); if(b){b->pos=e.pos; b->vel=dir*7.0f; b->radius=6; b->type=1; b->dmg=10; b->timer=150;} e.shoot_cd=70;}
        }
        else if(e.type==2) {
            if(dist>400) e.pos = e.pos + dir*e.speed;
            if(--e.shoot_cd<=0) { Bullet* b=Spawn(ebullets); if(b){b->pos=e.pos; b->vel=dir*5.0f; b->radius=6; b->type=2; b->dmg=15; b->timer=240;} e.shoot_cd=120;}
        }
        else if(e.type==3) {
            if(dist>350) e.pos = e.pos + dir*e.speed;
            if(--e.shoot_cd<=0) { SBeam* b=Spawn(sbeams); if(b){b->pos=P.pos; b->radius=60; b->timer=0; b->is_boss=false;} e.shoot_cd=200;}
        }
        else if(e.type==4) {
            if(dist>450) e.pos = e.pos + dir*e.speed;
            if(--e.shoot_cd<=0) { Bullet* b=Spawn(ebullets); if(b){b->pos=e.pos; b->vel=dir*6.0f; b->radius=6; b->type=3; b->dmg=20; b->timer=150;} e.shoot_cd=140;}
        }
        else if(e.type==5) {
            if(!e.attacking) {
                if(dist>350) e.pos = e.pos + dir*e.speed;
                if(--e.shoot_cd<=0) { e.attacking=true; e.ang=rand_float(0, PI*2); e.spin_spd=((rand_int()%2==0)?1:-1)*0.08f; e.spin_tmr=90; e.lock_tmr=0;}
            } else {
                if(e.spin_tmr>0) {
                    float ta = f_atan2((P.pos-e.pos).y, (P.pos-e.pos).x);
                    float diff = f_mod(ta - e.ang + PI, PI*2) - PI;
                    e.ang += diff*0.03f + e.spin_spd; e.spin_tmr--; e.spin_spd *= 0.97f;
                } else {
                    if(e.lock_tmr==0) e.lock_tmr=30;
                    if(--e.lock_tmr<=0) {
                        RBeam* rb=Spawn(rbeams); if(rb){rb->o=e.pos; rb->ang=e.ang; rb->hw=55; rb->timer=0;}
                        e.attacking=false; e.shoot_cd=rand_range(150,220);
                    }
                }
            }
        }
        else if(e.type==6) {
            if(dist>280) e.pos = e.pos + dir*e.speed;
            if(e.burst_cd>0) e.burst_cd--;
            else if(e.burst_cnt>0) {
                if(--e.shoot_cd<=0) {
                    Bullet* b=Spawn(ebullets); if(b){b->pos=e.pos; b->vel=dir*8.0f; b->radius=6; b->dmg=12; b->type=1; b->timer=150;}
                    e.burst_cnt--; e.shoot_cd=14; if(e.burst_cnt==0) e.burst_cd=120;
                }
            } else { e.burst_cnt=3; e.shoot_cd=1;}
        }
        else if(e.type==7) {
            if(dist>400) e.pos = e.pos + dir*e.speed;
            if(--e.shoot_cd<=0) { RBeam* rb=Spawn(rbeams); if(rb){rb->o=P.pos; rb->ang=rand_float(0,PI*2); rb->spin_spd=((rand_int()%2==0)?1:-1)*0.07f; rb->p_beam=true; rb->hw=55;} e.shoot_cd=rand_range(180,260);}
        }
        if(e.type==0 && dist < e.radius + P.radius) { P.hp-=15; e.active=false;}
    }
    for(int i=0; i<MAX_E; i++) {
        if(bullets[i].active) {
            if (bullets[i].type == 2) {
                bullets[i].vel = bullets[i].vel * 0.95f; 
            }
            bullets[i].pos = bullets[i].pos + bullets[i].vel; 
            bullets[i].age++;
            
            if(--bullets[i].timer<=0) {
                if(bullets[i].type==2) { Explode* ex=Spawn(expls); if(ex){ex->pos=bullets[i].pos; ex->max_r=150; ex->r=5; ex->dmg=60;}}
                bullets[i].active=false; continue;
            }
            bool hit=false;
            if(bullets[i].type==1) {
                for(int j=0; j<MAX_E; j++) if(coins[j].active && bullets[i].pos.dist(coins[j].pos)<bullets[i].radius+coins[j].radius+5) {
                    bullets[i].active=false; TriggerCoinChain(&coins[j], (float)bullets[i].dmg, 1); hit=true; break;
                }
            }
            if(hit) continue;
            for(int j=0; j<MAX_E; j++) if(enemies[j].active && bullets[i].pos.dist(enemies[j].pos) < bullets[i].radius+enemies[j].radius) {
                if(bullets[i].type==2) { Explode* ex=Spawn(expls); if(ex){ex->pos=bullets[i].pos; ex->max_r=150; ex->r=5; ex->dmg=60;}}
                else enemies[j].hp -= bullets[i].dmg;
                bullets[i].active=false; break;
            }
            if(bullets[i].active && B.active && bullets[i].pos.dist(B.pos)<bullets[i].radius+B.radius) {
                if(bullets[i].type==2) { Explode* ex=Spawn(expls); if(ex){ex->pos=bullets[i].pos; ex->max_r=150; ex->r=5; ex->dmg=60;}}
                B.hp -= bullets[i].dmg; bullets[i].active=false;
                char dmgBuf[16]; itoa(bullets[i].dmg, dmgBuf); char out[32]; str_cpy(out, "-"); str_cat(out, dmgBuf);
                AddText(B.pos + Vec2(rand_float(-20,20), -30), out, {1.0f,0.8f,1.0f});
            }
        }
        if(ebullets[i].active) {
            if(ebullets[i].type==2) {
                ebullets[i].hist[ebullets[i].h_idx] = ebullets[i].pos;
                ebullets[i].h_idx = (ebullets[i].h_idx+1)%10; if(ebullets[i].h_cnt<10) ebullets[i].h_cnt++;
                Vec2 tdir = P.pos - ebullets[i].pos;
                if(tdir.length()>0) ebullets[i].vel = ebullets[i].vel.lerp(tdir.normalize()*f_min(ebullets[i].vel.length()+0.1f, 8.5f), 0.15f);
            }
            ebullets[i].pos = ebullets[i].pos + ebullets[i].vel;
            if(--ebullets[i].timer<=0) {
                if(ebullets[i].type==3) { Explode* ex=Spawn(expls); if(ex){ex->pos=ebullets[i].pos; ex->max_r=70; ex->r=5; ex->dmg=20; ex->is_enemy=true;}}
                ebullets[i].active=false; continue;
            }
            if(ebullets[i].pos.dist(P.pos)<ebullets[i].radius+P.radius) {
                if(ebullets[i].type==3) { Explode* ex=Spawn(expls); if(ex){ex->pos=ebullets[i].pos; ex->max_r=70; ex->r=5; ex->dmg=20; ex->is_enemy=true;}}
                else P.hp -= ebullets[i].dmg;
                ebullets[i].active=false;
            }
        }
    }
    for(int i=0; i<MAX_E; i++) {
        if(coins[i].active) {
            coins[i].vel = coins[i].vel*0.93f; coins[i].pos = coins[i].pos+coins[i].vel; coins[i].age++;
            if(coins[i].pos.x<0 || coins[i].pos.x>WIDTH || coins[i].pos.y<0 || coins[i].pos.y>HEIGHT) coins[i].active=false;
        }
        if(mags[i].active) {
            if(--mags[i].timer<=0) mags[i].active=false;
            else {
                for(int j=0; j<MAX_E; j++) if(bullets[j].active && bullets[j].type==4) {
                    Vec2 to_c = mags[i].pos - bullets[j].pos; float d = to_c.length();
                    if(d>0 && d<mags[i].radius) {
                        Vec2 rad = to_c.normalize(); Vec2 tan(-rad.y, rad.x);
                        float pull = f_max(-15.0f, f_min(15.0f, (d - mags[i].orbit)*0.25f));
                        bullets[j].vel = (rad*pull + tan*20.0f).normalize()*20.0f;
                    }
                }
            }
        }
        if(bloods[i].active) {
            if(--bloods[i].timer<=0) bloods[i].active=false;
            if(bloods[i].pos.dist(P.pos)<50) { P.hp=f_min(P.hp+5, P.max_hp); bloods[i].active=false; AddText(bloods[i].pos, "+5 HP", C_GREEN);}
        }
        if(expls[i].active) {
            expls[i].r += 18.0f; if(expls[i].r >= expls[i].max_r) expls[i].active=false;
            if(expls[i].is_enemy) {
                if(expls[i].pos.dist(P.pos) < expls[i].r+P.radius && expls[i].dmg>0) { P.hp-=expls[i].dmg; expls[i].dmg=0; }
            } else {
                for(int j=0; j<MAX_E; j++) if(enemies[j].active && expls[i].pos.dist(enemies[j].pos) < expls[i].r+enemies[j].radius) { 
                    enemies[j].hp-=expls[i].dmg;
                }
                if(B.active && !expls[i].boss_hit && expls[i].pos.dist(B.pos)<expls[i].r+B.radius) { 
                    int dmg = f_max(1.0f, expls[i].dmg/5.0f);
                    B.hp-=dmg; expls[i].boss_hit=true;
                    char dmgBuf[16]; itoa(dmg, dmgBuf); char out[32]; str_cpy(out, "-"); str_cat(out, dmgBuf);
                    AddText(B.pos + Vec2(rand_float(-20,20), -30), out, {1.0f,0.8f,1.0f});
                }
            }
        }
        if(ftexts[i].active) { ftexts[i].pos.y-=ftexts[i].spd; if(--ftexts[i].timer<=0) ftexts[i].active=false;}
        if(beams[i].active) { if(--beams[i].timer<=0) beams[i].active=false;}
        
        if(sbeams[i].active) {
            if(sbeams[i].timer>=0) {
                sbeams[i].timer++;
                if(sbeams[i].timer<90) {
                    if(!sbeams[i].is_boss) { Vec2 dir=P.pos-sbeams[i].pos; if(dir.length()>0) sbeams[i].pos=sbeams[i].pos+dir.normalize()*5.0f;}
                    else { float& c=sbeams[i].vertical?sbeams[i].pos.x:sbeams[i].pos.y; float t=sbeams[i].vertical?P.pos.x:P.pos.y;
                           float d=t-c; c+= (d>0?1:-1)*f_min(f_abs(d), 5.0f); }
                } else if(sbeams[i].timer==150 && !sbeams[i].dmg_done) {
                    if(!sbeams[i].is_boss && sbeams[i].pos.dist(P.pos)<sbeams[i].radius) P.hp-=30;
                    if(sbeams[i].is_boss) { if(sbeams[i].vertical && f_abs(P.pos.x-sbeams[i].pos.x)<sbeams[i].radius) P.hp-=30;
                                            if(!sbeams[i].vertical && f_abs(P.pos.y-sbeams[i].pos.y)<sbeams[i].radius) P.hp-=30;}
                    sbeams[i].dmg_done=true;
                } else if(sbeams[i].timer>170) sbeams[i].active=false;
            } else sbeams[i].timer++;
        }
        if(rbeams[i].active) {
            if(rbeams[i].p_beam && rbeams[i].timer<130) {
                if(rbeams[i].timer<100) {
                    float ta = f_atan2((P.pos-rbeams[i].o).y, (P.pos-rbeams[i].o).x);
                    rbeams[i].ang += (f_mod(ta-rbeams[i].ang+PI, PI*2)-PI)*0.025f + rbeams[i].spin_spd;
                    rbeams[i].spin_spd *= 0.97f;
                }
            }
            rbeams[i].timer++;
            if((!rbeams[i].p_beam && rbeams[i].timer==20) || (rbeams[i].p_beam && rbeams[i].timer==130)) {
                float L=f_max(WIDTH,HEIGHT)*2.0f; Vec2 d(f_cos(rbeams[i].ang), f_sin(rbeams[i].ang));
                if(dist_point_line(P.pos, rbeams[i].o-d*L, rbeams[i].o+d*L) < rbeams[i].hw) P.hp-=30;
            }
            if((!rbeams[i].p_beam && rbeams[i].timer>35) || (rbeams[i].p_beam && rbeams[i].timer>160)) rbeams[i].active=false;
        }
    }
    if(P.hp<=0) ExitProcess(0);
}
void DrawGame() {
    glClearColor(C_BLACK.r, C_BLACK.g, C_BLACK.b, 1.0f); glClear(GL_COLOR_BUFFER_BIT);
    
    for(int i=0; i<MAX_E; i++) {
        if(bloods[i].active) DrawCircle(bloods[i].pos, (bloods[i].timer/600.f)*10.f, C_BLOOD);
        if(mags[i].active) {DrawCircle(mags[i].pos, 10, C_BLUE); DrawCircle(mags[i].pos, mags[i].radius, C_CYAN, true); DrawCircle(mags[i].pos, mags[i].orbit, C_BLUE, true, 2);}
        if (coins[i].active) {
            float w = f_max(1.0f, f_abs(f_cos(coins[i].age * 0.2f)) * coins[i].radius);
            DrawEllipse(coins[i].pos, w, coins[i].radius, C_YELLOW);
            DrawEllipse(coins[i].pos, w, coins[i].radius, C_ORANGE, true, 1.0f);
        }
        if(sbeams[i].active && sbeams[i].timer>=0) {
            if(sbeams[i].timer<90) {
                if(!sbeams[i].is_boss) DrawCircle(sbeams[i].pos, sbeams[i].radius, C_CYAN, true, 2);
                else { float hw=sbeams[i].radius; if(sbeams[i].vertical) DrawRect(Vec2(sbeams[i].pos.x-hw, 0), hw*2, HEIGHT, C_RED, true);
                       else DrawRect(Vec2(0, sbeams[i].pos.y-hw), WIDTH, hw*2, C_RED, true);}
            } else if(sbeams[i].timer<150) {
                if((sbeams[i].timer/5)%2==0) {
                    if(!sbeams[i].is_boss) DrawCircle(sbeams[i].pos, sbeams[i].radius, C_CYAN, true, 4);
                    else {float hw=sbeams[i].radius; if(sbeams[i].vertical) DrawRect(Vec2(sbeams[i].pos.x-hw, 0), hw*2, HEIGHT, C_CYAN, true);
                           else DrawRect(Vec2(0, sbeams[i].pos.y-hw), WIDTH, hw*2, C_CYAN, true);}
                }
            } else {
                if(!sbeams[i].is_boss) {DrawCircle(sbeams[i].pos, sbeams[i].radius, C_WHITE); DrawRect(Vec2(sbeams[i].pos.x-sbeams[i].radius, 0), sbeams[i].radius*2, HEIGHT, C_CYAN); DrawRect(Vec2(sbeams[i].pos.x-sbeams[i].radius/2, 0), sbeams[i].radius, HEIGHT, C_WHITE);}
                else { float hw=sbeams[i].radius; 
                    if(sbeams[i].vertical) {DrawCircle(Vec2(sbeams[i].pos.x, HEIGHT/2), hw, C_WHITE); DrawRect(Vec2(sbeams[i].pos.x-hw, 0), hw*2, HEIGHT, C_CYAN); DrawRect(Vec2(sbeams[i].pos.x-hw/2, 0), hw, HEIGHT, C_WHITE);}
                    else {DrawCircle(Vec2(WIDTH/2, sbeams[i].pos.y), hw, C_WHITE); DrawRect(Vec2(0, sbeams[i].pos.y-hw), WIDTH, hw*2, C_CYAN); DrawRect(Vec2(0, sbeams[i].pos.y-hw/2), WIDTH, hw, C_WHITE);}
                }
            }
        }
        if(rbeams[i].active) {
            float L=f_max(WIDTH,HEIGHT)*2; Vec2 d(f_cos(rbeams[i].ang), f_sin(rbeams[i].ang));
            Vec2 e1=rbeams[i].o+d*L, e2=rbeams[i].o-d*L;
            int tmr = rbeams[i].timer; bool pb = rbeams[i].p_beam;
            if((!pb && tmr<=0) || (pb && tmr<100)) DrawLine(e2, e1, C_RED, 2);
            else if((!pb && tmr<20) || (pb && tmr<130)) { if((tmr/4)%2==0) DrawLine(e2, e1, C_CYAN, 3); }
            else { DrawLine(e2, e1, C_CYAN, rbeams[i].hw*2); DrawLine(e2, e1, C_WHITE, rbeams[i].hw); }
        }
    }
    
    for(int i=0; i<MAX_E; i++) {
        if(enemies[i].active) {
            if (enemies[i].type == 7) {
                for (int k = 0; k < 4; k++) {
                    float a = (k * 90.0f + 45.0f) * PI / 180.0f;
                    Vec2 tip = enemies[i].pos + Vec2(f_cos(a), f_sin(a)) * enemies[i].radius * 1.4f;
                    DrawLine(enemies[i].pos, tip, enemies[i].c, 3.0f);
                }
            }
            DrawCircle(enemies[i].pos, enemies[i].radius, enemies[i].c);
            if (enemies[i].type == 5 || enemies[i].type == 7) {
                DrawCircle(enemies[i].pos, enemies[i].radius, {1.0f, 0.8f, 0.4f}, true, 2.0f);
            }
            if (enemies[i].type == 5 && enemies[i].attacking) {
                float L = f_max(WIDTH, HEIGHT) * 2.0f;
                Vec2 d(f_cos(enemies[i].ang), f_sin(enemies[i].ang));
                Vec2 e1 = enemies[i].pos + d * L;
                Vec2 e2 = enemies[i].pos - d * L;
                if (enemies[i].spin_tmr > 0) {
                    DrawLine(e2, e1, C_RED, 2.0f);
                } else {
                    if ((enemies[i].lock_tmr / 4) % 2 == 0) DrawLine(e2, e1, C_CYAN, 3.0f);
                }
            }
        }
        if(bullets[i].active) {
            if(bullets[i].type==2) { DrawCircle(bullets[i].pos, bullets[i].radius+f_sin(bullets[i].age*0.5f)*3.f, C_RED); DrawCircle(bullets[i].pos, bullets[i].radius-4.f, C_WHITE);}
            else { Col c=C_WHITE; if(bullets[i].type==1) c=C_YELLOW; if(bullets[i].type==3) c=C_ORANGE; if(bullets[i].type==4) c=C_GRAY; DrawCircle(bullets[i].pos, bullets[i].radius, c);}
        }
        if(ebullets[i].active) {
            Col c=C_WHITE; if(ebullets[i].type==1) c=C_ORANGE; if(ebullets[i].type==2) c=C_PURPLE; if(ebullets[i].type==3) c=C_MAGENTA;
            if(ebullets[i].type==2 && ebullets[i].h_cnt>1) {
                glBegin(GL_LINE_STRIP); SetCol(c); 
                for(int k=0; k<ebullets[i].h_cnt; k++) {
                    int idx = (ebullets[i].h_idx - ebullets[i].h_cnt + 10 + k) % 10;
                    glVertex2f(ebullets[i].hist[idx].x, ebullets[i].hist[idx].y);
                }
                glEnd();
            }
            DrawCircle(ebullets[i].pos, ebullets[i].radius, c);
        }
    }
    if(B.active) {
        float hr=B.radius*1.6f+f_sin(B.pulse)*8.f; DrawCircle(B.pos, hr, {0.7f,0.0f,0.8f}, false, 0.2f);
        DrawCircle(B.pos, B.radius, {0.7f,0.0f,0.8f}); DrawCircle(B.pos, B.radius, {1.0f,0.6f,1.0f}, true, 3);
        DrawRect(Vec2(50, HEIGHT-40), WIDTH-100, 20, {0.2f,0,0.2f}); DrawRect(Vec2(50, HEIGHT-40), (WIDTH-100)*(B.hp/(float)B.max_hp), 20, {0.8f,0,0.9f});
        char buf[64]; str_cpy(buf, "BOSS "); char num[16]; itoa(B.hp, num); str_cat(buf, num); str_cat(buf, "/300"); DrawTextStr(WIDTH/2-40, HEIGHT-62, buf, {1,0.8f,1});
    }
    DrawCircle(P.pos, P.radius, P.dash_timer>0 ? C_BLUE : C_GREEN);
    Vec2 aim_dir = (mouse_pos - P.pos).normalize();
    DrawLine(P.pos, P.pos + aim_dir * 25.0f, C_WHITE, 3);
    for(int i=0; i<MAX_E; i++) {
        if(beams[i].active) { DrawLine(beams[i].s, beams[i].e, beams[i].c, beams[i].w); DrawLine(beams[i].s, beams[i].e, C_WHITE, beams[i].w/2.0f);}
        if(expls[i].active) { DrawCircle(expls[i].pos, expls[i].r, expls[i].is_enemy?C_MAGENTA:C_ORANGE, true, 4);}
        if(ftexts[i].active) { DrawTextStr(ftexts[i].pos.x, ftexts[i].pos.y, ftexts[i].t, ftexts[i].c);}
    }
    char buf[64], n[16];
    str_cpy(buf, "HP: "); itoa(P.hp, n); str_cat(buf, n); str_cat(buf, "/"); itoa(P.max_hp, n); str_cat(buf, n); DrawTextStr(10, 10, buf, C_RED);
    str_cpy(buf, "WAVE: "); itoa(wave, n); str_cat(buf, n); DrawTextStr(WIDTH/2-40, 10, buf, C_MAGENTA);
    str_cpy(buf, "SCORE: "); itoa(score, n); str_cat(buf, n); DrawTextStr(WIDTH-150, 10, buf, C_YELLOW);
    const char* w_names[] = {"", "PISTOL", "SHOTGUN", "NAILGUN", "RAILGUN"};
    str_cpy(buf, "WEAPON [1-4]: "); str_cat(buf, w_names[P.weapon]); DrawTextStr(10, 40, buf, C_WHITE);
    if (P.weapon == 1) { 
        str_cpy(buf, "COINS: "); 
        for (int k = 0; k < P.coins; k++) str_cat(buf, "O ");
        for (int k = 0; k < 4 - P.coins; k++) str_cat(buf, "X ");
        DrawTextStr(10, 70, buf, P.coins > 0 ? C_YELLOW : C_RED); 
    }
    else if(P.weapon==3) { str_cpy(buf, "NAILS: "); itoa(P.nail_ammo, n); str_cat(buf, n); DrawTextStr(10, 70, buf, P.nail_ammo>20?C_GREEN:C_RED);}
    else if(P.weapon==4) {if(P.railgun_cd>0) { str_cpy(buf, "CHARGE: "); itoa(P.railgun_cd/60+1, n); str_cat(buf, n); str_cat(buf, "s"); DrawTextStr(10, 70, buf, C_RED);}
                          else DrawTextStr(10, 70, "RAILGUN READY", C_CYAN);}
    if(wave_delay>0 && !boss_spawned) { str_cpy(buf, "WAVE "); itoa(wave, n); str_cat(buf, n); DrawTextStr(WIDTH/2-40, HEIGHT/4, buf, C_WHITE);}
    DrawCircle(mouse_pos, 6, C_WHITE, true, 2.0f);
    DrawLine(mouse_pos - Vec2(10, 0), mouse_pos + Vec2(10, 0), C_WHITE, 2.0f);
    DrawLine(mouse_pos - Vec2(0, 10), mouse_pos + Vec2(0, 10), C_WHITE, 2.0f);
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if(msg==WM_DESTROY || (msg==WM_KEYDOWN && wp==VK_ESCAPE)) ExitProcess(0);
    if(msg==WM_MOUSEMOVE) { POINT p; GetCursorPos(&p); ScreenToClient(hwnd, &p); mouse_pos = Vec2((float)p.x, (float)p.y);}
    return DefWindowProc(hwnd, msg, wp, lp);
}
extern "C" void APIENTRY WinMainCRTStartup() {
    WIDTH = GetSystemMetrics(SM_CXSCREEN); HEIGHT = GetSystemMetrics(SM_CYSCREEN);
    WNDCLASS wc = { 0 }; wc.lpfnWndProc = WndProc; wc.hInstance = GetModuleHandle(0); wc.lpszClassName = "US_NO_CRT";
    RegisterClass(&wc);
    HWND hwnd = CreateWindowEx(0, wc.lpszClassName, "Ultra Shooter 2D", WS_POPUP | WS_VISIBLE, 0, 0, WIDTH, HEIGHT, 0, 0, wc.hInstance, 0);
    ShowCursor(FALSE); 
    HDC hdc = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA, 32, 0,0,0,0,0,0,0,0,0,0,0,0,0,24,8,0,PFD_MAIN_PLANE,0,0,0,0};
    SetPixelFormat(hdc, ChoosePixelFormat(hdc, &pfd), &pfd);
    wglMakeCurrent(hdc, wglCreateContext(hdc));
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, WIDTH, HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity(); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    HFONT hFont = CreateFont(24, 0, 0, 0, FW_BOLD, 0, 0, 0, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Arial");
    SelectObject(hdc, hFont); wglUseFontBitmaps(hdc, 0, 255, 1000); glListBase(1000);
    P.pos = Vec2(WIDTH / 2.0f, HEIGHT - 100.0f);
    LARGE_INTEGER freq_li, c_li;
    QueryPerformanceFrequency(&freq_li);
    QueryPerformanceCounter(&c_li);
    unsigned int freq = freq_li.LowPart;
    unsigned int ticks_per_frame = freq / 60;
    unsigned int last_tick = c_li.LowPart;
    MSG msg;
    while (true) {
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg);}
        QueryPerformanceCounter(&c_li);
        unsigned int now = c_li.LowPart;
        unsigned int diff = now - last_tick;
        if (diff >= ticks_per_frame) {
            if (diff > ticks_per_frame * 4) last_tick = now;
            else last_tick += ticks_per_frame;
            UpdateGame();
            DrawGame();
            SwapBuffers(hdc);
        }
        else {
            Sleep(0);
        }
    }
}