#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#include <windows.h>
const int W = 10, H = 20;
static int field[H][W] = { 0 };
static int score = 0;
static HANDLE hOut;
static const char shapes[7][16] = {
    {0,1,0,0, 0,1,0,0, 0,1,0,0, 0,1,0,0},
    {0,0,0,0, 0,1,1,0, 0,1,1,0, 0,0,0,0},
    {0,1,0,0, 1,1,1,0, 0,0,0,0, 0,0,0,0},
    {1,1,0,0, 0,1,1,0, 0,0,0,0, 0,0,0,0},
    {0,1,1,0, 1,1,0,0, 0,0,0,0, 0,0,0,0},
    {1,0,0,0, 1,1,1,0, 0,0,0,0, 0,0,0,0},
    {0,0,1,0, 1,1,1,0, 0,0,0,0, 0,0,0,0}
};
struct Current {
    int id, x, y, r;
} curr;
int getShapeCell(int id, int r, int x, int y) {
    if (r == 0) return shapes[id][y * 4 + x];
    if (r == 1) return shapes[id][12 - x * 4 + y];
    if (r == 2) return shapes[id][15 - y * 4 - x];
    return shapes[id][3 + x * 4 - y];
}
bool check(int nx, int ny, int nr) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (getShapeCell(curr.id, nr, i, j)) {
                int fx = nx + i, fy = ny + j;
                if (fx < 0 || fx >= W || fy >= H) return false;
                if (fy >= 0 && field[fy][fx]) return false;
            }
        }
    }
    return true;
}
void Draw() {
    char buf[1024]; int p = 0;
    SetConsoleCursorPosition(hOut, {0,0});
    const char* head = " TETRIS | SCORE: ";
    while(*head) buf[p++] = *head++;
    buf[p++] = (score / 100 % 10) + '0'; buf[p++] = (score / 10 % 10) + '0'; buf[p++] = '0';
    buf[p++] = '\n';
    for (int y = 0; y < H; y++) {
        buf[p++] = '<'; buf[p++] = '!';
        for (int x = 0; x < W; x++) {
            bool active = false;
            if (x >= curr.x && x < curr.x + 4 && y >= curr.y && y < curr.y + 4)
                if (getShapeCell(curr.id, curr.r, x - curr.x, y - curr.y)) active = true;
            if (field[y][x]) { buf[p++] = '['; buf[p++] = ']'; }
            else if (active) { buf[p++] = '#'; buf[p++] = '#'; }
            else { buf[p++] = ' '; buf[p++] = ' '; }
        }
        buf[p++] = '!'; buf[p++] = '>'; buf[p++] = '\n';
    }
    DWORD w;
    WriteConsoleA(hOut, buf, p, &w, 0);
}
extern "C" void mainCRTStartup() {
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci = { 1, FALSE };
    SetConsoleCursorInfo(hOut, &ci);
    curr = { 0, 3, 0, 0 };
    DWORD lastDrop = GetTickCount();
    while (1) {
        if (GetAsyncKeyState(VK_LEFT) & 1) if (check(curr.x - 1, curr.y, curr.r)) curr.x--;
        if (GetAsyncKeyState(VK_RIGHT) & 1) if (check(curr.x + 1, curr.y, curr.r)) curr.x++;
        if (GetAsyncKeyState(VK_UP) & 1) if (check(curr.x, curr.y, (curr.r + 1) % 4)) curr.r = (curr.r + 1) % 4;
        if (GetAsyncKeyState(VK_DOWN) & 1) if (check(curr.x, curr.y + 1, curr.r)) curr.y++;
        if (GetAsyncKeyState(VK_ESCAPE)) ExitProcess(0);
        if (GetTickCount() - lastDrop > 500) {
            if (check(curr.x, curr.y + 1, curr.r)) {
                curr.y++;
            } else {
                for (int i = 0; i < 4; i++)
                    for (int j = 0; j < 4; j++)
                        if (getShapeCell(curr.id, curr.r, i, j)) 
                            field[curr.y + j][curr.x + i] = 1;
                for (int y = H - 1; y >= 0; y--) {
                    bool full = true;
                    for (int x = 0; x < W; x++) if (!field[y][x]) full = false;
                    if (full) {
                        for (int ty = y; ty > 0; ty--) 
                            for (int tx = 0; tx < W; tx++) field[ty][tx] = field[ty - 1][tx];
                        score += 10; y++;
                    }
                }
                curr = { (int)GetTickCount() % 7, 3, 0, 0 };
                if (!check(curr.x, curr.y, curr.r)) {
                    for(int y=0; y<H; y++) for(int x=0; x<W; x++) field[y][x] = 0;
                    score = 0;
                }
            }
            lastDrop = GetTickCount();
        }

        Draw();
        Sleep(30);
    }
}