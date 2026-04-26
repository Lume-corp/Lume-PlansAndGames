#pragma optimize("gsy", on)
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(linker, "/SUBSYSTEM:windows")
#pragma comment(linker, "/MERGE:.rdata=.text")
#pragma comment(linker, "/MERGE:.data=.text")
#pragma comment(linker, "/SECTION:.text,ERW")
#pragma comment(linker, "/ENTRY:WinMainCRTStartup")
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <immintrin.h>
extern "C" int _fltused = 0;
typedef decltype(sizeof(0)) size_t;
#pragma function(memset)
extern "C" void* __cdecl memset(void* dest, int c, size_t count) {
    char* d = (char*)dest; while (count--) *d++ = (char)c; return dest;
}
#pragma function(memcpy)
extern "C" void* __cdecl memcpy(void* dest, const void* src, size_t count) {
    char* d = (char*)dest; const char* s = (const char*)src; while (count--) *d++ = *s++; return dest;
}
static float Fabs(float x) { return x < 0.0f ? -x : x; }
static float Fmax(float a, float b) { return a > b ? a : b; }
static float Fmin(float a, float b) { return a < b ? a : b; }
static float Floor(float x) { int i = (int)x; return (x < i) ? (float)(i - 1) : (float)i; }
static float Sqrt(float x) { return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(x))); }
static float Sin(float x) {
    float pi = 3.141592653589f;
    x = x * 0.15915494309f;
    x = x - (float)(int)x;
    if (x < 0.0f) x += 1.0f;
    x = x * 2.0f * pi;
    if (x > pi) x -= 2.0f * pi;
    if (x > pi / 2.0f) x = pi - x;
    else if (x < -pi / 2.0f) x = -pi - x;
    float x2 = x * x;
    return x * (1.0f + x2 * (-0.1666665668f + x2 * (0.008333025139f + x2 * (-0.0001980741872f + x2 * 0.000002601903f))));
}
static float Cos(float x) { return Sin(x + 1.57079632679f); }
static float Atan2(float y, float x) {
    float ax = Fabs(x), ay = Fabs(y);
    if (ax == 0.0f && ay == 0.0f) return 0.0f;
    float a = (ax < ay) ? ax / ay : ay / ax;
    float s = a * a;
    float r = ((-0.0464964749f * s + 0.15931422f) * s - 0.327622764f) * s * a + a;
    if (ay > ax) r = 1.57079632679f - r;
    if (x < 0.0f) r = 3.14159265358f - r;
    if (y < 0.0f) r = -r;
    return r;
}
static void StrCpy(char* d, const char* s) { while (*s) *d++ = *s++; *d = 0; }
static void StrCat(char* d, const char* s) { while (*d) d++; while (*s) *d++ = *s++; *d = 0; }
static int StrLen(const char* s) { int l = 0; while (s[l]) l++; return l; }
static void StrCatI(char* d, int v) {
    while (*d) d++;
    if (v == 0) { *d++ = '0'; *d = 0; return; }
    if (v < 0) { *d++ = '-'; v = -v; }
    char b[16]; int l = 0;
    while (v) { b[l++] = (v % 10) + '0'; v /= 10; }
    while (l > 0) *d++ = b[--l]; *d = 0;
}
static int Atoi(const char* s) {
    int r = 0; while (*s >= '0' && *s <= '9') { r = r * 10 + (*s - '0'); s++; } return r;
}
static unsigned rs = 1337;
static unsigned XR() { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
static float FR() { return (XR() % 10000) / 10000.0f; }
#define WORLD_SIZE 64
#define CHUNK_H 32
#define GRAVITY 0.015f
#define FRICTION 0.8f
#define JUMP 0.35f
#define PI  3.14159265f
#define MAX_DEMONS 100
#define MAX_PROJS 300
#define MAX_STRIKES 50
#define MAX_PARTS 2000
#define MAX_COINS 20
#define MAX_MAGNETS 10
#define MAX_BEAMS 50
#define MAX_EXPLOSIONS 20
struct vec3 {
    float x, y, z;
    vec3() : x(0), y(0), z(0) {}
    vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    vec3 operator+(const vec3& o) const { return vec3(x + o.x, y + o.y, z + o.z); }
    vec3 operator-(const vec3& o) const { return vec3(x - o.x, y - o.y, z - o.z); }
    vec3 operator*(float s) const { return vec3(x * s, y * s, z * s); }
    float dot(const vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    vec3 cross(const vec3& o) const { return vec3(y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x); }
    float length() const { return Sqrt(x * x + y * y + z * z); }
    vec3 normalize() const { float l = length(); return l > 0 ? vec3(x / l, y / l, z / l) : vec3(0, 0, 0); }
};
float distPointLine(vec3 p, vec3 l1, vec3 l2) {
    vec3 lineVec = l2 - l1;
    float lineLen = lineVec.length();
    if (lineLen == 0) return (p - l1).length();
    float t = Fmax(0.0f, Fmin(1.0f, (p - l1).dot(lineVec) / (lineLen * lineLen)));
    vec3 proj = l1 + lineVec * t;
    return (p - proj).length();
}
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER  0x8892
#define GL_STATIC_DRAW   0x88E4
#endif
typedef ptrdiff_t GLsizeiptr_;
typedef void(APIENTRY* PFN_GEN)(GLsizei, GLuint*);
typedef void(APIENTRY* PFN_BIND)(GLenum, GLuint);
typedef void(APIENTRY* PFN_DATA)(GLenum, GLsizeiptr_, const void*, GLenum);
typedef void(APIENTRY* PFN_DEL)(GLsizei, const GLuint*);
static PFN_GEN  pGenBuf;
static PFN_BIND pBndBuf;
static PFN_DATA pBufDat;
static PFN_DEL  pDelBuf;
static bool     hasVBO = false;
static GLuint   worldVBO = 0;
static int      worldVC = 0;
void initVBO() {
    pGenBuf = (PFN_GEN)wglGetProcAddress("glGenBuffers");
    pBndBuf = (PFN_BIND)wglGetProcAddress("glBindBuffer");
    pBufDat = (PFN_DATA)wglGetProcAddress("glBufferData");
    pDelBuf = (PFN_DEL)wglGetProcAddress("glDeleteBuffers");
    hasVBO = (pGenBuf && pBndBuf && pBufDat && pDelBuf);
}
HWND hWnd; HDC hDC; HGLRC hRC;
int width = 1280, height = 720;
bool running = true, cursorVisible = false;
GLuint fontBase = 0, atlasTex = 0;
enum TexID {
    T_FLOOR = 0, T_WALL, T_LAVA, T_DEMON_RED, T_DEMON_GREEN, T_DEMON_BLUE, T_DEMON_HEAD, T_ANGEL,
    T_PLASMA, T_FIREBALL, T_HOMING, T_LASER, T_MAGIC_CIRCLE, T_ORBITAL_BEAM, T_BLOOD, T_COIN, TEX_COUNT
};
int wave = 0, enemiesAlive = 0, waveTimer = 100;
bool cheatMode = false;
char cheatInput[32];
unsigned char map[WORLD_SIZE][CHUNK_H][WORLD_SIZE];
struct Player {
    float x, y, z, vx, vy, vz, yaw, pitch;
    int hp, jumpCount, dashCooldown, shootCooldown;
    int parryTimer, parryCooldown;
    float gunRecoil, damageFlash, parryFlash;
    bool onGround;
    int weapon, coins, maxCoins, coinTimer;
    int nailAmmo, nailMax, nailRegenTimer, railgunCooldown;
} p;
struct Demon { float x, y, z, vx, vy, vz; int type, hp, shootCooldown, attackStep; bool active, onGround; };
struct Projectile { float x, y, z, vx, vy, vz; bool isEnemy, active; int type, damage, timer; };
struct OrbitalStrike { float x, z; int state, timer; bool active; };
struct Particle { float x, y, z, vx, vy, vz, life; int tex; bool active; };
struct Coin { float x, y, z, vx, vy, vz; bool active; };
struct Magnet { float x, y, z; int timer; bool active; };
struct Beam { vec3 start, end; int timer; float r, g, b; bool active; };
struct Explosion { float x, y, z, radius, maxRadius; bool active; };
static Demon demons[MAX_DEMONS];
static Projectile projs[MAX_PROJS];
static OrbitalStrike strikes[MAX_STRIKES];
static Particle parts[MAX_PARTS];
static Coin coins_list[MAX_COINS];
static Magnet magnets[MAX_MAGNETS];
static Beam beams[MAX_BEAMS];
static Explosion explosions[MAX_EXPLOSIONS];
struct Vertex { float x, y, z, u, v; };
static Vertex worldMesh[65536];
void setupFont() {
    HFONT f = CreateFontA(-24, 0, 0, 0, FW_BOLD, 0, 0, 0, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Arial");
    SelectObject(hDC, f); fontBase = glGenLists(96); wglUseFontBitmaps(hDC, 32, 96, fontBase);
}
void dTxt(int x, int y, const char* t) {
    glRasterPos2i(x, y); glPushAttrib(GL_LIST_BIT); glListBase(fontBase - 32);
    glCallLists((GLsizei)StrLen(t), GL_UNSIGNED_BYTE, t); glPopAttrib();
}
void buildAtlas() {
    static unsigned char a[256][256][4] = {};
    for (int i = 0; i < TEX_COUNT; i++) {
        int px = (i % 16) * 16, py = (i / 16) * 16;
        for (int y = 0; y < 16; y++)for (int x = 0; x < 16; x++) {
            float n = (FR() - 0.5f) * 40; int r = 0, g = 0, b = 0, al = 255;
            switch (i) {
            case T_FLOOR: r = g = b = (int)(50 + n); break;
            case T_WALL: r = (int)(100 + n); g = (int)(40 + n); b = (int)(20 + n); break;
            case T_LAVA: r = (int)(200 + n); g = (int)(50 + n); b = 0; break;
            case T_DEMON_RED: r = (int)(200 + n); g = 20; b = 20; if (y > 10 && x > 4 && x < 12) { r = 255; g = 255; b = 0; } break;
            case T_DEMON_GREEN: r = 20; g = (int)(180 + n); b = 20; if (y > 10 && x > 4 && x < 12) { r = 255; g = 0; b = 0; } break;
            case T_DEMON_BLUE: r = 50; g = (int)(100 + n); b = (int)(255 + n); if (y > 6 && x > 4 && x < 12) { r = 255; g = 255; b = 255; } break;
            case T_DEMON_HEAD: r = (int)(150 + n); g = (int)(80 + n); b = (int)(30 + n); if (y > 4 && y < 12 && x > 4 && x < 12) { r = 255; g = 0; b = 0; } break;
            case T_ANGEL: r = 50; g = 150; b = 255; if ((x > 6 && x < 9) || (y > 6 && y < 9)) { r = 255; g = 255; b = 0; } break;
            case T_PLASMA: r = 0; g = 200; b = 255; break;
            case T_FIREBALL: r = 255; g = 100; b = 0; break;
            case T_HOMING: r = 255; g = 0; b = 255; break;
            case T_LASER: r = 255; g = 255; b = 200; break;
            case T_MAGIC_CIRCLE: {float d = Sqrt((float)((x - 8) * (x - 8) + (y - 8) * (y - 8))); r = g = b = 255; al = (d > 5 && d < 8) ? 255 : 0; } break;
            case T_ORBITAL_BEAM: r = 200; g = 255; b = 255; al = 180; break;
            case T_BLOOD: r = (int)(150 + n); g = 0; b = 0; break;
            case T_COIN: r = 255; g = 200; b = 0; if (x > 3 && x < 13 && y > 3 && y < 13) { r = 255; g = 255; b = 100; } break;
            }
            auto clamp = [](int v) {return v < 0 ? 0 : v>255 ? 255 : v; };
            a[py + y][px + x][0] = (unsigned char)clamp(r); a[py + y][px + x][1] = (unsigned char)clamp(g);
            a[py + y][px + x][2] = (unsigned char)clamp(b); a[py + y][px + x][3] = (unsigned char)al;
        }
    }
    glGenTextures(1, &atlasTex); glBindTexture(GL_TEXTURE_2D, atlasTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, a);
}
inline bool isSolid(int b) { return b == 1 || b == 2; }
static int topYCache[WORLD_SIZE][WORLD_SIZE];
void buildTopYCache() {
    for (int x = 0; x < WORLD_SIZE; x++) for (int z = 0; z < WORLD_SIZE; z++) {
        topYCache[x][z] = 0; for (int y = CHUNK_H - 1; y >= 0; y--) if (isSolid(map[x][y][z])) { topYCache[x][z] = y; break; }
    }
}
inline int getTopY(int x, int z) { if (x < 0 || x >= WORLD_SIZE || z < 0 || z >= WORLD_SIZE)return 0; return topYCache[x][z]; }
inline bool isBoxSolid(float x0, float y0, float z0, float x1, float y1, float z1) {
    int ix0 = (int)Floor(x0), iy0 = (int)Floor(y0), iz0 = (int)Floor(z0); int ix1 = (int)Floor(x1), iy1 = (int)Floor(y1), iz1 = (int)Floor(z1);
    for (int x = ix0; x <= ix1; x++) {
        if (x < 0 || x >= WORLD_SIZE)continue;
        for (int y = iy0; y <= iy1; y++) {
            if (y < 0 || y >= CHUNK_H)continue;
            for (int z = iz0; z <= iz1; z++) { if (z < 0 || z >= WORLD_SIZE)continue; if (isSolid(map[x][y][z]))return true; }
        }
    } return false;
}
static const float FV[6][4][3] = { {{0,1,0},{0,1,1},{1,1,1},{1,1,0}},{{0,0,0},{1,0,0},{1,0,1},{0,0,1}}, {{0,0,1},{1,0,1},{1,1,1},{0,1,1}},{{1,0,0},{0,0,0},{0,1,0},{1,1,0}}, {{0,0,0},{0,0,1},{0,1,1},{0,1,0}},{{1,0,1},{1,0,0},{1,1,0},{1,1,1}} };
static const float FUV[4][2] = { {0,1},{0,0},{1,0},{1,1} };
static const int FN[6][3] = { {0,1,0},{0,-1,0},{0,0,1},{0,0,-1},{-1,0,0},{1,0,0}};
void generateArena() {
    memset(map, 0, sizeof(map)); worldVC = 0;
    for (int x = 0; x < WORLD_SIZE; x++)for (int z = 0; z < WORLD_SIZE; z++) {
        map[x][0][z] = (FR() > 0.95f) ? 2 : 1;
        if (x == 0 || x == WORLD_SIZE - 1 || z == 0 || z == WORLD_SIZE - 1) for (int y = 1; y < 15; y++)map[x][y][z] = 1;
        if (x > 5 && x < WORLD_SIZE - 5 && z > 5 && z < WORLD_SIZE - 5) {
            if (x % 15 == 0 && z % 15 == 0)for (int y = 1; y < 10; y++)map[x][y][z] = 1;
            if (x % 20 > 15 && z % 20 > 15)map[x][6][z] = 1;
        }
    }
    for (int x = 0; x < WORLD_SIZE; x++)for (int y = 0; y < CHUNK_H; y++)for (int z = 0; z < WORLD_SIZE; z++) {
        int b = map[x][y][z]; if (!b)continue;
        int tex = (b == 2) ? T_LAVA : ((y == 0) ? T_FLOOR : T_WALL); float u0 = (tex % 16) / 16.0f, v0 = (tex / 16) / 16.0f;
        for (int f = 0; f < 6; f++) {
            int nx = x + FN[f][0], ny = y + FN[f][1], nz = z + FN[f][2];
            if (nx >= 0 && nx < WORLD_SIZE && ny >= 0 && ny < CHUNK_H && nz >= 0 && nz < WORLD_SIZE && isSolid(map[nx][ny][nz]))continue;
            for (int i = 0; i < 4; i++) {
                if (worldVC < 65536) worldMesh[worldVC++] = { (float)x + FV[f][i][0],(float)y + FV[f][i][1],(float)z + FV[f][i][2], u0 + FUV[i][0] / 16.0f, v0 + FUV[i][1] / 16.0f};
            }
        }
    }
    if (hasVBO && worldVC > 0) {
        if (!worldVBO)pGenBuf(1, &worldVBO); pBndBuf(GL_ARRAY_BUFFER, worldVBO);
        pBufDat(GL_ARRAY_BUFFER, (GLsizeiptr_)(worldVC * sizeof(Vertex)), worldMesh, GL_STATIC_DRAW);
        pBndBuf(GL_ARRAY_BUFFER, 0); 
    }
    buildTopYCache();
    memset(&p, 0, sizeof(p));
    p.x = WORLD_SIZE / 2.0f; p.z = WORLD_SIZE / 2.0f; p.y = 5;
    p.hp = 100; p.weapon = 1; p.coins = 4; p.maxCoins = 4; p.coinTimer = 180;
    p.nailAmmo = 100; p.nailMax = 100; p.nailRegenTimer = 15;
    wave = 0; enemiesAlive = 0; waveTimer = 100;

    memset(demons, 0, sizeof(demons)); memset(projs, 0, sizeof(projs));
    memset(strikes, 0, sizeof(strikes)); memset(parts, 0, sizeof(parts));
    memset(coins_list, 0, sizeof(coins_list)); memset(magnets, 0, sizeof(magnets));
    memset(beams, 0, sizeof(beams)); memset(explosions, 0, sizeof(explosions));
}
void spawnParticles(float x, float y, float z, int tex, int n) {
    for (int i = 0; i < MAX_PARTS && n > 0; i++) if (!parts[i].active) {
        parts[i] = { x,y,z, (FR() - 0.5f) * 0.4f,FR() * 0.4f,(FR() - 0.5f) * 0.4f, 1.0f, tex, true };
        n--;
    }
}
void spawnWave() {
    wave++;
    int cnt[5] = { wave + 1, (wave >= 3) ? wave - 1 : 0, (wave >= 5) ? (wave - 3) / 2 : 0, (wave >= 8) ? (wave - 6) / 2 : 0, (wave >= 11) ? (wave - 9) / 2 : 0 };
    int types[5] = { 0,1,2,3,4 }; int hps[5] = { 40,30,60,120,200 }; float ys[5] = { 2,2,10,12,18 };
    for (int t = 0; t < 5; t++)for (int j = 0; j < cnt[t]; j++) {
        for (int i = 0; i < MAX_DEMONS; i++) if (!demons[i].active) {
            demons[i].x = 5 + (float)(XR() % (WORLD_SIZE - 10)); demons[i].z = 5 + (float)(XR() % (WORLD_SIZE - 10));
            demons[i].y = ys[t]; demons[i].type = types[t]; demons[i].hp = hps[t];
            demons[i].shootCooldown = XR() % 60; demons[i].active = true;
            enemiesAlive++; break;
        }
    }
}
vec3 raycastWall(vec3 start, vec3 dir, float maxDist) {
    vec3 curr = start;
    for (float d = 0; d < maxDist; d += 0.5f) {
        curr = start + dir * d;
        if (isBoxSolid(curr.x - 0.1f, curr.y - 0.1f, curr.z - 0.1f, curr.x + 0.1f, curr.y + 0.1f, curr.z + 0.1f)) break;
    } return curr;
}
void explodeCore(vec3 pos, float radius, int damage) {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) if (!explosions[i].active) { explosions[i] = { pos.x, pos.y, pos.z, 0.5f, radius, true }; break;}
    spawnParticles(pos.x, pos.y, pos.z, T_FIREBALL, 50);
    for (int i = 0; i < MAX_DEMONS; i++) {
        auto& d = demons[i]; if (!d.active) continue;
        if ((vec3(d.x, d.y + 1.0f, d.z) - pos).length() < radius) {
            d.hp -= damage;
            if (d.hp <= 0) { d.active = false; enemiesAlive--; p.hp = (int)Fmin(100, (float)p.hp + 5); }
            spawnParticles(d.x, d.y + 1.0f, d.z, T_BLOOD, 20);
        }
    }
}
void triggerCoinChain(Coin& startCoin, int baseDamage, int weaponType) {
    vec3 curr(startCoin.x, startCoin.y, startCoin.z);
    startCoin.active = false; int hits = 1;
    while (true) {
        vec3 targetPos; float minDist = 9999.0f; Coin* tCoin = nullptr;
        for (int i = 0; i < MAX_COINS; i++) {
            auto& c = coins_list[i]; if (!c.active) continue; float d = (vec3(c.x, c.y, c.z) - curr).length();
            if (d < minDist) { minDist = d; tCoin = &c; }
        }
        if (tCoin) {
            targetPos = vec3(tCoin->x, tCoin->y, tCoin->z); tCoin->active = false; hits++;
            for (int i = 0; i < MAX_BEAMS; i++) if (!beams[i].active) { beams[i] = { curr, targetPos, 15, 1.0f, 1.0f, 0.0f, true }; break;}
            curr = targetPos; continue;
        }
        Projectile* tCore = nullptr; minDist = 9999.0f;
        for (int i = 0; i < MAX_PROJS; i++) {
            auto& pr = projs[i]; if (pr.active && pr.type == 6) {
                float d = (vec3(pr.x, pr.y, pr.z) - curr).length();
                if (d < minDist) { minDist = d; tCore = &pr; }
            }
        }
        if (tCore) {
            targetPos = vec3(tCore->x, tCore->y, tCore->z); tCore->active = false;
            for (int i = 0; i < MAX_BEAMS; i++) if (!beams[i].active) { beams[i] = { curr, targetPos, 15, 1.0f, 0.0f, 0.0f, true }; break;}
            explodeCore(targetPos, weaponType == 4 ? 12.0f : 8.0f, weaponType == 4 ? 200 : 80); break;
        }
        Demon* tDemon = nullptr; minDist = 9999.0f;
        for (int i = 0; i < MAX_DEMONS; i++) {
            auto& d = demons[i]; if (!d.active) continue; float d_dist = (vec3(d.x, d.y + 1.0f, d.z) - curr).length();
            if (d_dist < minDist) { minDist = d_dist; tDemon = &d; }
        }
        vec3 dir(0, -1, 0); if (tDemon) dir = (vec3(tDemon->x, tDemon->y + 1.0f, tDemon->z) - curr).normalize();
        vec3 endHit = raycastWall(curr, dir, 50.0f);
        for (int i = 0; i < MAX_BEAMS; i++) if (!beams[i].active) { beams[i] = { curr, endHit, 15, 1.0f, 1.0f, 0.0f, true }; break;}
        float finalDmg = baseDamage * (1.0f + 0.1f * hits);
        for (int i = 0; i < MAX_DEMONS; i++) {
            auto& d = demons[i]; if (!d.active) continue;
            if (distPointLine(vec3(d.x, d.y + 1.0f, d.z), curr, endHit) < 1.5f) {
                d.hp -= (int)finalDmg; spawnParticles(d.x, d.y + 1.0f, d.z, T_BLOOD, 20);
                if (d.hp <= 0) { d.active = false; enemiesAlive--; p.hp = (int)Fmin(100, (float)p.hp + 5);}
            }
        } break;
    }
}
void handleWeapons() {
    if (GetAsyncKeyState('1') & 0x8000) p.weapon = 1; if (GetAsyncKeyState('2') & 0x8000) p.weapon = 2;
    if (GetAsyncKeyState('3') & 0x8000) p.weapon = 3; if (GetAsyncKeyState('4') & 0x8000) p.weapon = 4;
    vec3 pPos(p.x, p.y + 1.5f, p.z);
    vec3 aimDir(Sin(p.yaw) * Cos(p.pitch), -Sin(p.pitch), -Cos(p.yaw) * Cos(p.pitch));
    static bool lmbWas = false, rmbWas = false;
    bool lmbNow = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0; bool rmbNow = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

    if (lmbNow && !lmbWas && p.weapon == 4 && p.railgunCooldown <= 0) {
        vec3 beamEnd = raycastWall(pPos, aimDir, 100.0f);
        for (int i = 0; i < MAX_COINS; i++) {
            auto& c = coins_list[i]; if (c.active && distPointLine(vec3(c.x, c.y, c.z), pPos, beamEnd) < 1.5f) {
                beamEnd = vec3(c.x, c.y, c.z); triggerCoinChain(c, 100, 4); break;
            }
        }
        for (int i = 0; i < MAX_PROJS; i++) {
            auto& pr = projs[i]; if (pr.active && pr.type == 6) {
                if (distPointLine(vec3(pr.x, pr.y, pr.z), pPos, beamEnd) < 2.0f) {
                    pr.active = false; beamEnd = vec3(pr.x, pr.y, pr.z); explodeCore(beamEnd, 12.0f, 250); break;
                }
            }
        }
        for (int i = 0; i < MAX_DEMONS; i++) {
            auto& d = demons[i]; if (!d.active) continue;
            if (distPointLine(vec3(d.x, d.y + 1.0f, d.z), pPos, beamEnd) < 1.5f) {
                d.hp -= 100; spawnParticles(d.x, d.y + 1.0f, d.z, T_BLOOD, 20);
                if (d.hp <= 0) { d.active = false; enemiesAlive--; }
            }
        }
        for (int i = 0; i < MAX_BEAMS; i++) if (!beams[i].active) { beams[i] = { pPos, beamEnd, 20, 0.0f, 1.0f, 1.0f, true }; break; }
        p.railgunCooldown = 300; p.gunRecoil = 1.0f; p.damageFlash = 0.2f;
    }
    else if (lmbNow && p.weapon == 1 && p.shootCooldown <= 0) {
        vec3 beamEnd = raycastWall(pPos, aimDir, 100.0f); bool hitCoin = false;
        for (int i = 0; i < MAX_COINS; i++) {
            auto& c = coins_list[i]; if (c.active && distPointLine(vec3(c.x, c.y, c.z), pPos, beamEnd) < 2.0f) {
                beamEnd = vec3(c.x, c.y, c.z); hitCoin = true; triggerCoinChain(c, 30, 1); break;
            }
        }
        if (!hitCoin) {
            for (int i = 0; i < MAX_DEMONS; i++) {
                auto& d = demons[i]; if (!d.active) continue;
                if (distPointLine(vec3(d.x, d.y + 1.0f, d.z), pPos, beamEnd) < 1.0f) {
                    d.hp -= 20; spawnParticles(d.x, d.y + 1.0f, d.z, T_BLOOD, 10);
                    if (d.hp <= 0) { d.active = false; enemiesAlive--; p.hp = (int)Fmin(100, (float)p.hp + 2);}
                }
            }
        }
        for (int i = 0; i < MAX_BEAMS; i++) if (!beams[i].active) { beams[i] = { pPos, beamEnd, 6, 1.0f, 1.0f, 0.0f, true }; break;}
        p.shootCooldown = 15; p.gunRecoil = 0.5f;
    }
    else if (lmbNow && p.shootCooldown <= 0) {
        if (p.weapon == 2) {
            for (int j = 0; j < 7; j++) {
                vec3 sp = aimDir + vec3((FR() - 0.5f) * 0.2f, (FR() - 0.5f) * 0.2f, (FR() - 0.5f) * 0.2f);
                sp = sp.normalize(); float m = 1.0f + FR() * 0.5f;
                for (int i = 0; i < MAX_PROJS; i++) if (!projs[i].active) {projs[i] = {pPos.x, pPos.y, pPos.z, sp.x * m, sp.y * m, sp.z * m, false, true, 5, 8, 200}; break;}
            }
            p.shootCooldown = 40; p.gunRecoil = 1.0f;
        }
        else if (p.weapon == 3 && p.nailAmmo > 0) {
            vec3 sp = aimDir + vec3((FR() - 0.5f) * 0.05f, (FR() - 0.5f) * 0.05f, (FR() - 0.5f) * 0.05f);
            for (int i = 0; i < MAX_PROJS; i++) if (!projs[i].active) {projs[i] = {pPos.x, pPos.y, pPos.z, sp.x * 1.5f, sp.y * 1.5f, sp.z * 1.5f, false, true, 7, 6, 300}; break;}
            p.nailAmmo--; p.shootCooldown = 5; p.gunRecoil = 0.2f;
        }
    }

    if (rmbNow && !rmbWas) {
        if (p.weapon == 1 && p.coins > 0) {
            for (int i = 0; i < MAX_COINS; i++) if (!coins_list[i].active) { coins_list[i] = { pPos.x, pPos.y, pPos.z, aimDir.x * 0.4f, aimDir.y * 0.4f + 0.5f, aimDir.z * 0.4f, true}; break;}
            p.coins--;
        }
        else if (p.weapon == 2 && p.shootCooldown <= 0) {
            for (int i = 0; i < MAX_PROJS; i++) if (!projs[i].active) {projs[i] = {pPos.x, pPos.y, pPos.z, aimDir.x * 0.8f, aimDir.y * 0.8f + 0.2f, aimDir.z * 0.8f, false, true, 6, 0, 200}; break;}
            p.shootCooldown = 60; p.gunRecoil = 1.0f;
        }
        else if (p.weapon == 3) {
            vec3 hit = raycastWall(pPos, aimDir, 50.0f);
            for (int i = 0; i < MAX_MAGNETS; i++) if (!magnets[i].active) {magnets[i] = { hit.x - aimDir.x * 0.5f, hit.y + 1.0f, hit.z - aimDir.z * 0.5f, 400, true}; break;}
        }
    }
    lmbWas = lmbNow; rmbWas = rmbNow;
    if (p.coins < p.maxCoins) {if (--p.coinTimer <= 0) { p.coins++; p.coinTimer = 180;}}
    if (p.nailAmmo < p.nailMax) {if (--p.nailRegenTimer <= 0) { p.nailAmmo++; p.nailRegenTimer = 15;}}
    if (p.railgunCooldown > 0) p.railgunCooldown--;
}
void physics() {
    if (cheatMode) return;
    if (enemiesAlive <= 0) {if (waveTimer > 0)waveTimer--; else { waveTimer = 100; spawnWave();}}
    handleWeapons();
    float spd = 0.05f, fx = 0, fz = 0;
    if (GetAsyncKeyState('W') & 0x8000) {fx += Sin(p.yaw) * Cos(p.pitch); fz -= Cos(p.yaw) * Cos(p.pitch);}
    if (GetAsyncKeyState('S') & 0x8000) {fx -= Sin(p.yaw) * Cos(p.pitch); fz += Cos(p.yaw) * Cos(p.pitch);}
    if (GetAsyncKeyState('A') & 0x8000) {fx -= Cos(p.yaw); fz -= Sin(p.yaw);}
    if (GetAsyncKeyState('D') & 0x8000) {fx += Cos(p.yaw); fz += Sin(p.yaw);}
    float len = Sqrt(fx * fx + fz * fz);
    if (len > 0) {fx = (fx / len) * spd; fz = (fz / len) * spd;}
    p.vx += fx; p.vz += fz; p.vx *= FRICTION; p.vz *= FRICTION;
    if (p.dashCooldown > 0)p.dashCooldown--;
    if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) && p.dashCooldown == 0 && len > 0) {p.vx += (fx / spd) * 1.5f; p.vz += (fz / spd) * 1.5f; p.dashCooldown = 60;}
    if (p.parryCooldown > 0)p.parryCooldown--; if (p.parryTimer > 0)p.parryTimer--;
    if ((GetAsyncKeyState('F') & 0x8000) && p.parryCooldown == 0) { p.parryTimer = 15; p.parryCooldown = 80; }
    p.x += p.vx; if (isBoxSolid(p.x - 0.3f, p.y, p.z - 0.3f, p.x + 0.3f, p.y + 1.7f, p.z + 0.3f)) {p.x -= p.vx; p.vx = 0;}
    p.z += p.vz; if (isBoxSolid(p.x - 0.3f, p.y, p.z - 0.3f, p.x + 0.3f, p.y + 1.7f, p.z + 0.3f)) {p.z -= p.vz; p.vz = 0;}
    p.vy -= GRAVITY; p.y += p.vy;
    if (isBoxSolid(p.x - 0.3f, p.y, p.z - 0.3f, p.x + 0.3f, p.y + 1.7f, p.z + 0.3f)) { if (p.vy < 0) {p.onGround = true; p.jumpCount = 0; } p.y -= p.vy; p.vy = 0;} else p.onGround = false;
    static bool spaceWas = false; bool spaceNow = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    if (spaceNow && !spaceWas && p.jumpCount < 2) {p.vy = JUMP; p.jumpCount++; p.onGround = false;} spaceWas = spaceNow;
    if (p.y < -10 || p.hp <= 0) { generateArena(); return; }
    if (p.shootCooldown > 0)p.shootCooldown--; if (p.gunRecoil > 0)p.gunRecoil -= 0.1f;
    if (p.damageFlash > 0)p.damageFlash -= 0.05f; if (p.parryFlash > 0)p.parryFlash -= 0.05f;
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!explosions[i].active) continue;
        explosions[i].radius += 1.5f; if (explosions[i].radius >= explosions[i].maxRadius) explosions[i].active = false;
    }
    for (int i = 0; i < MAX_COINS; i++) {
        auto& c = coins_list[i]; if (!c.active) continue;
        c.vy -= GRAVITY; c.x += c.vx; c.y += c.vy; c.z += c.vz; c.vx *= 0.98f; c.vz *= 0.98f;
        if (isBoxSolid(c.x - 0.1f, c.y - 0.1f, c.z - 0.1f, c.x + 0.1f, c.y + 0.1f, c.z + 0.1f)) c.active = false;
    }
    for (int i = 0; i < MAX_MAGNETS; i++) {
        auto& m = magnets[i]; if (!m.active) continue;
        if (--m.timer <= 0) { m.active = false; continue;}
        vec3 mPos(m.x, m.y, m.z);
        for (int j = 0; j < MAX_PROJS; j++){
            auto& pr = projs[j]; if (!pr.active || pr.type != 7) continue;
            vec3 pPos(pr.x, pr.y, pr.z); float distToMag = (mPos - pPos).length();
            if (distToMag < 20.0f) {
                pr.vy = (m.y - pr.y) * 0.2f;
                vec3 toCenterXZ(m.x - pr.x, 0, m.z - pr.z); float distXZ = toCenterXZ.length();
                if (distXZ > 0.1f) {
                    vec3 radial = toCenterXZ.normalize();
                    vec3 tangent(-radial.z, 0, radial.x);
                    float pull = (distXZ - 2.5f) * 0.4f;
                    vec3 newVel = radial * pull + tangent * 1.5f;
                    pr.vx = newVel.x; pr.vz = newVel.z;
                    if (pr.timer < 30) pr.timer = 30;
                }
            }
        }
    }
    for (int i = 0; i < MAX_DEMONS; i++) {
        auto& d = demons[i]; if (!d.active)continue;
        float dx = p.x - d.x, dz = p.z - d.z, dist = Sqrt(dx * dx + dz * dz), ang = Atan2(dx, dz);
        if (d.type == 2) {
            d.vx = Sin(ang) * 0.03f; d.vz = Cos(ang) * 0.03f; d.vy = ((p.y + 4) - d.y) * 0.02f;
            d.x += d.vx; d.y += d.vy; d.z += d.vz;
            if (d.shootCooldown > 0)d.shootCooldown--;
            if (dist < 40 && d.shootCooldown == 0) {
                for (int s = 0; s < 5; s++) {
                    for (int j = 0; j < MAX_PROJS; j++) if (!projs[j].active) { projs[j] = {d.x,d.y,d.z,Sin(ang + (s - 2) * 0.2f) * 0.3f,0.2f,Cos(ang + (s - 2) * 0.2f) * 0.3f,true,true,2,5,400 }; break;}
                }
                d.shootCooldown = 150;
            }
        }
        else if (d.type == 3) {
            d.vx = Sin(ang) * 0.02f; d.vz = Cos(ang) * 0.02f; d.vy = ((p.y + 5) - d.y) * 0.02f; d.x += d.vx; d.y += d.vy; d.z += d.vz;
            if (d.shootCooldown > 0)d.shootCooldown--;
            if (dist < 35 && d.shootCooldown == 0) {
                float pD = Sqrt(dx * dx + (p.y + 1 - d.y) * (p.y + 1 - d.y) + dz * dz);
                if (d.attackStep < 5) {
                    for (int j = 0; j < MAX_PROJS; j++) if (!projs[j].active) { projs[j] = {d.x,d.y,d.z,(dx / pD) * 0.6f,((p.y + 1 - d.y) / pD) * 0.6f,(dz / pD) * 0.6f,true,true,1,8,400 }; break;}
                    d.attackStep++; d.shootCooldown = 15;
                }
                else {
                    for (int j = 0; j < MAX_PROJS; j++) if (!projs[j].active) { projs[j] = {d.x,d.y,d.z,(dx / pD) * 1.5f,((p.y + 1 - d.y) / pD) * 1.5f,(dz / pD) * 1.5f,true,true,3,20,400 }; break;}
                    d.attackStep = 0; d.shootCooldown = 120;
                }
            }
        }
        else if (d.type == 4) {
            d.vy = ((p.y + 10) - d.y) * 0.02f; if (dist < 15) { d.vx = -Sin(ang) * 0.03f; d.vz = -Cos(ang) * 0.03f; } else if (dist > 25) {d.vx = Sin(ang) * 0.03f; d.vz = Cos(ang) * 0.03f;} else d.vx = d.vz = 0;
            d.x += d.vx; d.y += d.vy; d.z += d.vz;
            if (d.shootCooldown > 0)d.shootCooldown--; if (d.shootCooldown == 0) {
                for (int j = 0; j < MAX_STRIKES; j++) if (!strikes[j].active) { strikes[j] = { p.x,p.z,0,90,true }; break; }
                d.shootCooldown = 300;
            }
        }
        else {
            d.vy -= GRAVITY; d.y += d.vy;
            if (isBoxSolid(d.x - 0.4f, d.y, d.z - 0.4f, d.x + 0.4f, d.y + 1.8f, d.z + 0.4f)) { d.y -= d.vy; d.vy = 0; d.onGround = true; } else d.onGround = false;
            if (dist < 30) {
                float eSpd = (d.type == 1) ? 0.12f : 0.06f; d.vx = Sin(ang) * eSpd; d.vz = Cos(ang) * eSpd; d.x += d.vx;
                if (isBoxSolid(d.x - 0.4f, d.y + 0.1f, d.z - 0.4f, d.x + 0.4f, d.y + 1.8f, d.z + 0.4f)) { d.x -= d.vx; if (d.onGround)d.vy = JUMP; } d.z += d.vz;
                if (isBoxSolid(d.x - 0.4f, d.y + 0.1f, d.z - 0.4f, d.x + 0.4f, d.y + 1.8f, d.z + 0.4f)) { d.z -= d.vz; if (d.onGround)d.vy = JUMP; }
                if (d.type == 1) {
                    if (d.shootCooldown > 0)d.shootCooldown--;
                    if (dist < 1.5f && d.shootCooldown == 0) { if (p.parryTimer > 0) { d.hp = 0; p.parryFlash = 1; p.hp = (int)Fmin(100, (float)p.hp + 20); spawnParticles(d.x, d.y, d.z, T_BLOOD, 30); } else { p.hp -= 10; p.damageFlash = 1; d.shootCooldown = 40; } }
                }
                else {
                    if (d.shootCooldown > 0)d.shootCooldown--;
                    if (dist < 20 && d.shootCooldown == 0) {
                        float dy2 = (p.y + 1) - (d.y + 1), pD = Sqrt(dx * dx + dy2 * dy2 + dz * dz);
                        for (int j = 0; j < MAX_PROJS; j++) if (!projs[j].active) { projs[j] = { d.x,d.y + 1,d.z,(dx / pD) * 0.6f,(dy2 / pD) * 0.6f,(dz / pD) * 0.6f,true,true,1,8,400 }; break; }
                        d.shootCooldown = 80;
                    }
                }
            }
        }
        if (d.hp <= 0) { d.active = false; enemiesAlive--; p.hp = (int)Fmin(100, (float)p.hp + 5); }
    }

    for (int i = 0; i < MAX_PROJS; i++) {
        auto& pr = projs[i]; if (!pr.active)continue;
        if (pr.type == 2 && pr.isEnemy) {
            float tx2 = p.x - pr.x, ty2 = (p.y + 1) - pr.y, tz2 = p.z - pr.z, tL = Sqrt(tx2 * tx2 + ty2 * ty2 + tz2 * tz2);
            if (tL > 0) { tx2 /= tL; ty2 /= tL; tz2 /= tL; }
            pr.vx += tx2 * 0.015f; pr.vy += ty2 * 0.015f; pr.vz += tz2 * 0.015f;
            float pL = Sqrt(pr.vx * pr.vx + pr.vy * pr.vy + pr.vz * pr.vz);
            if (pL > 0) { pr.vx = (pr.vx / pL) * 0.3f; pr.vy = (pr.vy / pL) * 0.3f; pr.vz = (pr.vz / pL) * 0.3f; }
        }
        if (pr.type == 6) { pr.vy -= GRAVITY; pr.vx *= 0.99f; pr.vz *= 0.99f; }
        pr.x += pr.vx; pr.y += pr.vy; pr.z += pr.vz; pr.timer--;
        if (isBoxSolid(pr.x - 0.1f, pr.y - 0.1f, pr.z - 0.1f, pr.x + 0.1f, pr.y + 0.1f, pr.z + 0.1f) || pr.timer <= 0) {
            pr.active = false;
            if (pr.type == 6) explodeCore(vec3(pr.x, pr.y, pr.z), 10.0f, 150);
            else spawnParticles(pr.x, pr.y, pr.z, T_PLASMA, 10);
            continue;
        }
        if (pr.isEnemy) {
            if (Fabs(pr.x - p.x) < 0.5f && pr.y > p.y && pr.y < p.y + 1.8f && Fabs(pr.z - p.z) < 0.5f) {
                if (p.parryTimer > 0) { p.parryFlash = 1; p.hp = (int)Fmin(100, (float)p.hp + 15); pr.isEnemy = false; pr.damage = 100; pr.vx = Sin(p.yaw) * Cos(p.pitch) * 3; pr.vy = -Sin(p.pitch) * 3; pr.vz = -Cos(p.yaw) * Cos(p.pitch) * 3; }
                else { p.hp -= pr.damage; p.damageFlash = 1; pr.active = false; }
            }
        }
        else {
            for (int j = 0; j < MAX_DEMONS; j++) {
                auto& d = demons[j]; if (!d.active)continue;
                if (Fabs(pr.x - d.x) < 0.8f && pr.y > d.y && pr.y < d.y + 1.8f && Fabs(pr.z - d.z) < 0.8f) {
                    pr.active = false;
                    if (pr.type == 6) { explodeCore(vec3(pr.x, pr.y, pr.z), 10.0f, 150); }
                    else {
                        d.hp -= pr.damage; spawnParticles(pr.x, pr.y, pr.z, T_BLOOD, 20);
                        if (d.hp <= 0) { d.active = false; enemiesAlive--; p.hp = (int)Fmin(100, (float)p.hp + 5); }
                    }
                    break;
                }
            }
        }
    }
    for (int i = 0; i < MAX_STRIKES; i++) {
        auto& s = strikes[i]; if (!s.active)continue;
        if (s.state == 0) { s.x += (p.x - s.x) * 0.1f; s.z += (p.z - s.z) * 0.1f; if (--s.timer <= 0) { s.state = 1; s.timer = 60; } }
        else if (s.state == 1) { if (--s.timer <= 0) { s.state = 2; s.timer = 20; } }
        else { float d2 = Sqrt((p.x - s.x) * (p.x - s.x) + (p.z - s.z) * (p.z - s.z)); if (d2 < 2.5f) { p.hp -= 5; p.damageFlash = 1; } if (--s.timer <= 0)s.active = false; }
    }
    for (int i = 0; i < MAX_PARTS; i++) {
        auto& pt = parts[i]; if (!pt.active) continue;
        pt.vy -= GRAVITY * 0.5f; pt.x += pt.vx; pt.y += pt.vy; pt.z += pt.vz; pt.life -= 0.05f;
        if (pt.life <= 0) pt.active = false;
    }
}
void drawSprite(float x, float y, float z, float sz, int tex) {
    float u0 = (tex % 16) / 16.0f, v0 = (tex / 16) / 16.0f, u1 = u0 + 1.0f / 16, v1 = v0 + 1.0f / 16;
    glPushMatrix(); glTranslatef(x, y, z); glRotatef(-p.yaw * 180 / PI, 0, 1, 0); glRotatef(-p.pitch * 180 / PI, 1, 0, 0);
    glBegin(GL_QUADS); glTexCoord2f(u0, v1); glVertex3f(-sz, -sz, 0); glTexCoord2f(u1, v1); glVertex3f(sz, -sz, 0); glTexCoord2f(u1, v0); glVertex3f(sz, sz, 0); glTexCoord2f(u0, v0); glVertex3f(-sz, sz, 0); glEnd(); glPopMatrix();
}
void drawFlatSprite(float x, float y, float z, float sz, int tex, float alpha) {
    float u0 = (tex % 16) / 16.0f, v0 = (tex / 16) / 16.0f, u1 = u0 + 1.0f / 16, v1 = v0 + 1.0f / 16;
    glColor4f(1, 1, 1, alpha); glBegin(GL_QUADS); glTexCoord2f(u0, v1); glVertex3f(x - sz, y, z - sz); glTexCoord2f(u1, v1); glVertex3f(x + sz, y, z - sz); glTexCoord2f(u1, v0); glVertex3f(x + sz, y, z + sz); glTexCoord2f(u0, v0); glVertex3f(x - sz, y, z + sz); glEnd();
}
void drawCylinder(float x, float z, float r, int tex) {
    float u0 = (tex % 16) / 16.0f, v0 = (tex / 16) / 16.0f, u1 = u0 + 1.0f / 16, v1 = v0 + 1.0f / 16; glColor4f(1, 1, 1, 0.5f); glBegin(GL_QUADS);
    glTexCoord2f(u0, v1); glVertex3f(x - r, 0, z); glTexCoord2f(u1, v1); glVertex3f(x + r, 0, z); glTexCoord2f(u1, v0); glVertex3f(x + r, 50, z); glTexCoord2f(u0, v0); glVertex3f(x - r, 50, z);
    glTexCoord2f(u0, v1); glVertex3f(x, 0, z - r); glTexCoord2f(u1, v1); glVertex3f(x, 0, z + r); glTexCoord2f(u1, v0); glVertex3f(x, 50, z + r); glTexCoord2f(u0, v0); glVertex3f(x, 50, z - r); glEnd();
}
void dRect(int x, int y, int w, int h, float r, float g, float b, float a = 1) {
    glColor4f(r, g, b, a); glBegin(GL_QUADS); glVertex2i(x, y); glVertex2i(x + w, y); glVertex2i(x + w, y + h); glVertex2i(x, y + h); glEnd();
}
void drawExplosion(float x, float y, float z, float r) {
    glDisable(GL_TEXTURE_2D); glColor4f(1.0f, 0.4f, 0.0f, 0.6f); glPushMatrix(); glTranslatef(x, y, z);
    glBegin(GL_QUADS);
    glVertex3f(-r, 0, -r); glVertex3f(r, 0, -r); glVertex3f(r, 0, r); glVertex3f(-r, 0, r);
    glVertex3f(-r, -r, 0); glVertex3f(r, -r, 0); glVertex3f(r, r, 0); glVertex3f(-r, r, 0);
    glVertex3f(0, -r, -r); glVertex3f(0, r, -r); glVertex3f(0, r, r); glVertex3f(0, -r, r);
    glEnd(); glPopMatrix(); glEnable(GL_TEXTURE_2D); glColor4f(1, 1, 1, 1);
}
void render() {
    glClearColor(0.1f, 0.05f, 0.05f, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST); glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, atlasTex);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glEnable(GL_ALPHA_TEST); glAlphaFunc(GL_GREATER, 0.1f);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluPerspective(90, (float)width / height, 0.1f, 200.0f);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity(); float cd = Sin(p.yaw) * Cos(p.pitch), cy = -Sin(p.pitch), cz = -Cos(p.yaw) * Cos(p.pitch); gluLookAt(p.x, p.y + 1.6f, p.z, p.x + cd, p.y + 1.6f + cy, p.z + cz, 0, 1, 0);
    glColor4f(1, 1, 1, 1);
    if (hasVBO && worldVBO && worldVC > 0) {
        glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_TEXTURE_COORD_ARRAY); pBndBuf(GL_ARRAY_BUFFER, worldVBO);
        glVertexPointer(3, GL_FLOAT, sizeof(Vertex), (void*)0); glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), (void*)(3 * sizeof(float)));
        glDrawArrays(GL_QUADS, 0, worldVC); pBndBuf(GL_ARRAY_BUFFER, 0); glDisableClientState(GL_VERTEX_ARRAY); glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    else if (worldVC > 0) { glBegin(GL_QUADS); for (int i = 0; i < worldVC; i++) { glTexCoord2f(worldMesh[i].u, worldMesh[i].v); glVertex3f(worldMesh[i].x, worldMesh[i].y, worldMesh[i].z); } glEnd(); }
    glDisable(GL_CULL_FACE);
    for (int i = 0; i < MAX_STRIKES; i++) { auto& s = strikes[i]; if (!s.active)continue; float sy = (float)getTopY((int)s.x, (int)s.z) + 1.1f; if (s.state == 0 || s.state == 1) { float al = (s.state == 1 && s.timer % 10 > 5) ? 0.2f : 1.0f; drawFlatSprite(s.x, sy, s.z, 2.5f, T_MAGIC_CIRCLE, al); } else drawCylinder(s.x, s.z, 2.5f, T_ORBITAL_BEAM); }
    for (int i = 0; i < MAX_DEMONS; i++) { auto& d = demons[i]; if (!d.active)continue; glPushMatrix(); glTranslatef(d.x - 0.4f, d.y, d.z - 0.4f); float sY = (d.type >= 2 && d.type <= 4) ? 0.8f : 1.8f; glScalef(0.8f, sY, 0.8f); int tex = T_DEMON_RED + (d.type < 5 ? d.type : 0); float u0 = (tex % 16) / 16.0f, v0 = (tex / 16) / 16.0f; glBegin(GL_QUADS); for (int f = 0; f < 6; f++)for (int k = 0; k < 4; k++) { glTexCoord2f(u0 + FUV[k][0] / 16.0f, v0 + FUV[k][1] / 16.0f); glVertex3f(FV[f][k][0], FV[f][k][1], FV[f][k][2]); } glEnd(); glPopMatrix(); }
    for (int i = 0; i < MAX_PROJS; i++) { auto& pr = projs[i]; if (!pr.active)continue; int pt = pr.isEnemy ? (pr.type == 0 ? T_PLASMA : pr.type == 1 ? T_FIREBALL : pr.type == 2 ? T_HOMING : T_LASER) : (pr.type == 6 ? T_FIREBALL : T_PLASMA); drawSprite(pr.x, pr.y, pr.z, pr.type == 6 ? 0.6f : 0.2f, pt); }
    for (int i = 0; i < MAX_COINS; i++) if (coins_list[i].active) drawSprite(coins_list[i].x, coins_list[i].y, coins_list[i].z, 0.2f, T_COIN);
    for (int i = 0; i < MAX_MAGNETS; i++) if (magnets[i].active) drawSprite(magnets[i].x, magnets[i].y, magnets[i].z, 0.4f, T_PLASMA);
    for (int i = 0; i < MAX_PARTS; i++) if (parts[i].active) drawSprite(parts[i].x, parts[i].y, parts[i].z, 0.1f * parts[i].life, parts[i].tex);
    for (int i = 0; i < MAX_EXPLOSIONS; i++) if (explosions[i].active) drawExplosion(explosions[i].x, explosions[i].y, explosions[i].z, explosions[i].radius);
    glDisable(GL_TEXTURE_2D); glLineWidth(5.0f); glBegin(GL_LINES);
    for (int i = 0; i < MAX_BEAMS; i++) { auto& b = beams[i]; if (!b.active)continue; glColor4f(b.r, b.g, b.b, (float)b.timer / 20.0f); glVertex3f(b.start.x, b.start.y, b.start.z); glVertex3f(b.end.x, b.end.y, b.end.z); if (--b.timer <= 0) b.active = false; }
    glEnd(); glEnable(GL_TEXTURE_2D); glColor3f(1, 1, 1);
    glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); glLoadIdentity(); glTranslatef(0.4f, -0.4f - p.gunRecoil * 0.2f, -1 + p.gunRecoil * 0.3f); glRotatef(10, 0, 1, 0); float gu0 = (T_WALL % 16) / 16.0f, gv0 = (T_WALL / 16) / 16.0f, gu1 = gu0 + 1.0f / 16, gv1 = gv0 + 1.0f / 16; int coreCol = (p.weapon == 1) ? T_COIN : (p.weapon == 4) ? T_LASER : T_PLASMA; float pu0 = (coreCol % 16) / 16.0f, pv0 = (coreCol / 16) / 16.0f, pu1 = pu0 + 1.0f / 16, pv1 = pv0 + 1.0f / 16; glColor3f(0.5f, 0.5f, 0.5f); glBegin(GL_QUADS); glTexCoord2f(gu0, gv1); glVertex3f(-0.1f, -0.1f, -0.5f); glTexCoord2f(gu1, gv1); glVertex3f(0.1f, -0.1f, -0.5f); glTexCoord2f(gu1, gv0); glVertex3f(0.1f, 0.1f, -0.5f); glTexCoord2f(gu0, gv0); glVertex3f(-0.1f, 0.1f, -0.5f); glTexCoord2f(gu0, gv1); glVertex3f(-0.1f, 0.1f, -0.5f); glTexCoord2f(gu1, gv1); glVertex3f(0.1f, 0.1f, -0.5f); glTexCoord2f(gu1, gv0); glVertex3f(0.15f, 0.15f, 0.5f); glTexCoord2f(gu0, gv0); glVertex3f(-0.15f, 0.15f, 0.5f); glTexCoord2f(gu0, gv1); glVertex3f(-0.1f, -0.1f, -0.5f); glTexCoord2f(gu1, gv1); glVertex3f(0.1f, -0.1f, -0.5f); glTexCoord2f(gu1, gv0); glVertex3f(0.15f, -0.15f, 0.5f); glTexCoord2f(gu0, gv0); glVertex3f(-0.15f, -0.15f, 0.5f); glTexCoord2f(gu0, gv1); glVertex3f(-0.1f, -0.1f, -0.5f); glTexCoord2f(gu1, gv1); glVertex3f(-0.1f, 0.1f, -0.5f); glTexCoord2f(gu1, gv0); glVertex3f(-0.15f, 0.15f, 0.5f); glTexCoord2f(gu0, gv0); glVertex3f(-0.15f, -0.15f, 0.5f); glTexCoord2f(gu0, gv1); glVertex3f(0.1f, -0.1f, -0.5f); glTexCoord2f(gu1, gv1); glVertex3f(0.1f, 0.1f, -0.5f); glTexCoord2f(gu1, gv0); glVertex3f(0.15f, 0.15f, 0.5f); glTexCoord2f(gu0, gv0); glVertex3f(0.15f, -0.15f, 0.5f); glColor3f(1, 1, 1); glTexCoord2f(pu0, pv1); glVertex3f(-0.05f, 0.11f, -0.3f); glTexCoord2f(pu1, pv1); glVertex3f(0.05f, 0.11f, -0.3f); glTexCoord2f(pu1, pv0); glVertex3f(0.08f, 0.16f, 0.3f); glTexCoord2f(pu0, pv0); glVertex3f(-0.08f, 0.16f, 0.3f); glEnd();
    glDisable(GL_TEXTURE_2D); glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0, width, height, 0, -1, 1); glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glColor3f(0, 1, 0); glLineWidth(2); glBegin(GL_LINES); glVertex2i(width / 2 - 10, height / 2); glVertex2i(width / 2 + 10, height / 2); glVertex2i(width / 2, height / 2 - 10); glVertex2i(width / 2, height / 2 + 10); glEnd();
    if (p.damageFlash > 0)dRect(0, 0, width, height, 1, 0, 0, p.damageFlash * 0.5f); if (p.parryFlash > 0)dRect(0, 0, width, height, 1, 1, 1, p.parryFlash * 0.7f);
    if (cheatMode) {
        dRect(width / 2 - 150, height / 2 - 40, 300, 80, 0, 0, 0, 0.9f); glColor3f(1, 1, 0); dTxt(width / 2 - 130, height / 2 - 20, "JUMP TO WAVE:"); glColor3f(1, 1, 1); dTxt(width / 2 + 60, height / 2 - 20, cheatInput);
    }
    else {
        char buf[256]; buf[0] = 0;
        StrCat(buf, "WAVE: "); StrCatI(buf, wave); StrCat(buf, " | W: ");
        if (p.weapon == 1) { StrCat(buf, "PISTOL | COINS: "); StrCatI(buf, p.coins); }
        else if (p.weapon == 2) { StrCat(buf, "SHOTGUN | CORE"); }
        else if (p.weapon == 3) { StrCat(buf, "NAILGUN | "); StrCatI(buf, p.nailAmmo); }
        else { StrCat(buf, "RAILGUN | "); StrCat(buf, p.railgunCooldown > 0 ? "CD" : "RDY"); }
        StrCat(buf, " | HP: "); StrCatI(buf, p.hp); StrCat(buf, " | DASH: "); StrCat(buf, p.dashCooldown == 0 ? "RDY" : "CD"); StrCat(buf, " | PARRY: "); StrCat(buf, p.parryCooldown == 0 ? "RDY" : (p.parryTimer > 0 ? "ACTIVE" : "CD"));
        dRect(10, height - 50, 950, 40, 0, 0, 0, 0.7f); glColor3f(1, 1, 1); dTxt(20, height - 40, buf);
    }
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW); SwapBuffers(hDC);
}
LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_CLOSE) { running = false; return 0; }
    if (m == WM_SIZE) { width = LOWORD(l); height = HIWORD(l); if (!height)height = 1; glViewport(0, 0, width, height); }
    if (m == WM_KEYDOWN) {
        if (w == VK_ESCAPE) { if (cheatMode) { cheatMode = false; cursorVisible = false; } else { cursorVisible = !cursorVisible; if (cursorVisible)while (ShowCursor(TRUE) < 0); else while (ShowCursor(FALSE) >= 0); } }
        else if (w == 'P' && !cursorVisible && !cheatMode) { cheatMode = true; cheatInput[0] = 0; cursorVisible = true; while (ShowCursor(TRUE) < 0); }
        else if (cheatMode) {
            if (w >= '0' && w <= '9') { int cl = StrLen(cheatInput); if (cl < 30) { cheatInput[cl] = (char)w; cheatInput[cl + 1] = 0; } }
            if (w == VK_BACK && StrLen(cheatInput) > 0) cheatInput[StrLen(cheatInput) - 1] = 0;
            if (w == VK_RETURN && StrLen(cheatInput) > 0) {
                int tw = Atoi(cheatInput); if (tw > 0) { wave = tw - 1; for (int i = 0; i < MAX_DEMONS; i++) demons[i].active = false; for (int i = 0; i < MAX_STRIKES; i++)strikes[i].active = false; enemiesAlive = 0; waveTimer = 0; } cheatMode = false; cursorVisible = false; while (ShowCursor(FALSE) >= 0);
            }
        }
    } return DefWindowProc(h, m, w, l);
}
extern "C" void WinMainCRTStartup() {
    WNDCLASSA wc = { 0,WndProc,0,0,GetModuleHandle(0),0,LoadCursor(0,IDC_ARROW),0,"M","M" }; RegisterClassA(&wc);
    hWnd = CreateWindowA("M", "ULTRA-DOOM C++ (NO CRT)", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, width, height, 0, 0, wc.hInstance, 0);
    PIXELFORMATDESCRIPTOR pfd = { sizeof(pfd),1,PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,PFD_TYPE_RGBA,32,0,0,0,0,0,0,0,0,0,0,0,0,0,24 };
    hDC = GetDC(hWnd); SetPixelFormat(hDC, ChoosePixelFormat(hDC, &pfd), &pfd); hRC = wglCreateContext(hDC); wglMakeCurrent(hDC, hRC);
    setupFont(); buildAtlas(); initVBO(); generateArena(); while (!cursorVisible && ShowCursor(FALSE) >= 0);
    MSG msg;
    while (running) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg); }
        else {
            if (GetForegroundWindow() == hWnd && !cursorVisible && !cheatMode) {
                POINT pt; GetCursorPos(&pt); ScreenToClient(hWnd, &pt); int dx = pt.x - width / 2, dy = pt.y - height / 2;
                if (dx || dy) { p.yaw += dx * 0.003f; p.pitch += dy * 0.003f; if (p.pitch > 1.5f)p.pitch = 1.5f; if (p.pitch < -1.5f)p.pitch = -1.5f; POINT c = { width / 2,height / 2 }; ClientToScreen(hWnd, &c); SetCursorPos(c.x, c.y); }
                physics();
            } render(); Sleep(10);
        }
    }
    if (hasVBO && worldVBO)pDelBuf(1, &worldVBO);
    ExitProcess(0);
}