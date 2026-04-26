#pragma optimize("gsy", on)
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup")
#pragma comment(linker, "/MERGE:.rdata=.text")
#pragma comment(linker, "/MERGE:.data=.text")
#pragma comment(linker, "/SECTION:.text,ERW")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#include <windows.h>
typedef decltype(sizeof(0)) size_t;
#pragma function(memset)
extern "C" void* __cdecl memset(void* dest, int c, size_t count) {
    char* d = (char*)dest; while (count--) *d++ = (char)c; return dest;
}
#pragma function(memcpy)
extern "C" void* __cdecl memcpy(void* dest, const void* src, size_t count) {
    char* d = (char*)dest; const char* s = (const char*)src; while (count--) *d++ = *s++; return dest;
}
int StrLen(const char* s) { int l = 0; while (s[l]) l++; return l; }
void StrCpy(char* d, const char* s) { while (*s) *d++ = *s++; *d = 0; }
void StrCat(char* d, const char* s) { while (*d) d++; while (*s) *d++ = *s++; *d = 0; }
void IntToStr(int v, char* d) {
    if (v == 0) { d[0] = '0'; d[1] = 0; return; }
    if (v < 0) { *d++ = '-'; v = -v; }
    char b[16]; int l = 0;
    while (v) { b[l++] = (v % 10) + '0'; v /= 10; }
    while (l > 0) *d++ = b[--l];
    *d = 0;
}
unsigned int rs = 1337;
unsigned int Rand() { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
int RandRange(int min, int max) { return min + (Rand() % (max - min + 1)); }
const int BOARD_W = 40;
const int BOARD_H = 25;
const int BLOCK_SIZE = 30;
const int GAME_TICK_MS = 100;
const int MAX_SNAKE = BOARD_W * BOARD_H;
struct Point {
    int x, y;
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
};
Point snake[MAX_SNAKE];
int snakeSize = 0;
Point food;
Point currentDir;
Point nextDir;
int score = 0;
int bestScore = 0;
bool gameOver = false;
bool paused = false;
int screenW, screenH;
DWORD lastTick = 0;
void SpawnFood() {
    bool valid = false;
    while (!valid) {
        food.x = RandRange(0, BOARD_W - 1);
        food.y = RandRange(0, BOARD_H - 1);
        valid = true;
        for (int i = 0; i < snakeSize; ++i) {
            if (snake[i] == food) { valid = false; break; }
        }
    }
}
void ResetGame() {
    snakeSize = 0;
    int sx = BOARD_W / 2, sy = BOARD_H / 2;
    snake[0] = { sx, sy };
    snake[1] = { sx - 1, sy };
    snake[2] = { sx - 2, sy };
    snake[3] = { sx - 3, sy };
    snakeSize = 4;

    currentDir = { 1, 0 };
    nextDir = { 1, 0 };
    score = 0;
    gameOver = false;
    paused = false;
    SpawnFood();
}
void Turn(int dx, int dy) {
    if (gameOver) return;
    if (currentDir.x + dx == 0 && currentDir.y + dy == 0) return;
    nextDir = { dx, dy };
}
void Update() {
    if (gameOver || paused) return;
    DWORD now = GetTickCount();
    if (now - lastTick < GAME_TICK_MS) return;
    lastTick = now;
    currentDir = nextDir;
    Point head = snake[0];
    Point newHead = { head.x + currentDir.x, head.y + currentDir.y };
    if (newHead.x < 0 || newHead.x >= BOARD_W || newHead.y < 0 || newHead.y >= BOARD_H) {
        gameOver = true;
        if (score > bestScore) bestScore = score;
        return;
    }
    for (int i = 0; i < snakeSize - 1; ++i) {
        if (newHead == snake[i]) {
            gameOver = true;
            if (score > bestScore) bestScore = score;
            return;
        }
    }
    for (int i = snakeSize; i > 0; i--) {
        snake[i] = snake[i - 1];
    }
    snake[0] = newHead;
    snakeSize++;
    if (newHead == food) {
        score += 10;
        SpawnFood();
    } else {
        snakeSize--;
    }
}
void Render(HDC hdc) {
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, screenW, screenH);
    SelectObject(memDC, memBitmap);
    RECT screenRect = { 0, 0, screenW, screenH };
    HBRUSH bgBrush = CreateSolidBrush(RGB(15, 15, 15));
    FillRect(memDC, &screenRect, bgBrush);
    DeleteObject(bgBrush);
    int boardPixelW = BOARD_W * BLOCK_SIZE;
    int boardPixelH = BOARD_H * BLOCK_SIZE;
    int offsetX = (screenW - boardPixelW) / 2;
    int offsetY = (screenH - boardPixelH) / 2;
    RECT borderRect = { offsetX - 4, offsetY - 4, offsetX + boardPixelW + 4, offsetY + boardPixelH + 4 };
    HBRUSH borderBrush = CreateSolidBrush(RGB(40, 40, 40));
    FillRect(memDC, &borderRect, borderBrush);
    DeleteObject(borderBrush);
    RECT innerRect = { offsetX, offsetY, offsetX + boardPixelW, offsetY + boardPixelH };
    HBRUSH innerBrush = CreateSolidBrush(RGB(25, 25, 30));
    FillRect(memDC, &innerRect, innerBrush);
    DeleteObject(innerBrush);
    HBRUSH foodBrush = CreateSolidBrush(RGB(231, 76, 60));
    SelectObject(memDC, foodBrush);
    SelectObject(memDC, GetStockObject(NULL_PEN));
    Ellipse(memDC,
        offsetX + food.x * BLOCK_SIZE + 2,
        offsetY + food.y * BLOCK_SIZE + 2,
        offsetX + (food.x + 1) * BLOCK_SIZE - 2,
        offsetY + (food.y + 1) * BLOCK_SIZE - 2);
    DeleteObject(foodBrush);
    for (int i = 0; i < snakeSize; ++i) {
        int greenVal = 255;
        if (snakeSize > 1) {
            greenVal = 255 - (i * 180) / (snakeSize - 1);
        }
        if (greenVal < 75) greenVal = 75;
        HBRUSH snakeBrush = CreateSolidBrush(RGB(46, greenVal, 113));
        RECT sRect = {
            offsetX + snake[i].x * BLOCK_SIZE + 1,
            offsetY + snake[i].y * BLOCK_SIZE + 1,
            offsetX + (snake[i].x + 1) * BLOCK_SIZE - 1,
            offsetY + (snake[i].y + 1) * BLOCK_SIZE - 1
        };
        FillRect(memDC, &sRect, snakeBrush);
        DeleteObject(snakeBrush);
    }
    SetBkMode(memDC, TRANSPARENT);
    SetTextColor(memDC, RGB(100, 100, 100));
    char scoreTxt[128];
    char numBuf[32];
    StrCpy(scoreTxt, "SNAKE | SCORE: ");
    IntToStr(score, numBuf); StrCat(scoreTxt, numBuf);
    StrCat(scoreTxt, "  BEST: ");
    IntToStr(bestScore, numBuf); StrCat(scoreTxt, numBuf);
    TextOutA(memDC, offsetX, offsetY - 30, scoreTxt, StrLen(scoreTxt));
    SetTextColor(memDC, RGB(60, 60, 60));
    const char* help = "WASD / Arrows = Move | P = Pause | R = Restart | ESC = Quit";
    TextOutA(memDC, offsetX, offsetY + boardPixelH + 10, help, StrLen(help));
    if (paused && !gameOver) {
        for (int y = offsetY; y < offsetY + boardPixelH; y += 2) {
            RECT line = { offsetX, y, offsetX + boardPixelW, y + 1 };
            HBRUSH db = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(memDC, &line, db);
            DeleteObject(db);
        }
        HFONT hf = CreateFontA(60, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Consolas");
        HGDIOBJ of = SelectObject(memDC, hf);
        SetTextColor(memDC, RGB(200, 200, 200));
        const char* ptxt = "PAUSED";
        SIZE sz; GetTextExtentPoint32A(memDC, ptxt, 6, &sz);
        TextOutA(memDC, offsetX + (boardPixelW - sz.cx) / 2, offsetY + (boardPixelH - sz.cy) / 2, ptxt, 6);
        SelectObject(memDC, of); DeleteObject(hf);
    }
    if (gameOver) {
        for (int y = offsetY; y < offsetY + boardPixelH; y += 2) {
            RECT line = { offsetX, y, offsetX + boardPixelW, y + 1 };
            HBRUSH db = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(memDC, &line, db);
            DeleteObject(db);
        }
        int panW = 400, panH = 120;
        int panX = offsetX + (boardPixelW - panW) / 2;
        int panY = offsetY + (boardPixelH - panH) / 2;
        RECT panRect = { panX, panY, panX + panW, panY + panH };
        HBRUSH panBr = CreateSolidBrush(RGB(15, 15, 15));
        FillRect(memDC, &panRect, panBr);
        DeleteObject(panBr);
        HPEN goPen = CreatePen(PS_SOLID, 2, RGB(231, 76, 60));
        HGDIOBJ opGo = SelectObject(memDC, goPen);
        SelectObject(memDC, GetStockObject(NULL_BRUSH));
        Rectangle(memDC, panX, panY, panX + panW, panY + panH);
        SelectObject(memDC, opGo);
        DeleteObject(goPen);
        SetBkMode(memDC, TRANSPARENT);
        HFONT hf1 = CreateFontA(44, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Consolas");
        HGDIOBJ of1 = SelectObject(memDC, hf1);
        SetTextColor(memDC, RGB(231, 76, 60));
        const char* goTxt = "GAME OVER";
        SIZE sz; GetTextExtentPoint32A(memDC, goTxt, StrLen(goTxt), &sz);
        TextOutA(memDC, panX + (panW - sz.cx) / 2, panY + 15, goTxt, StrLen(goTxt));
        SelectObject(memDC, of1); DeleteObject(hf1);
        HFONT hf2 = CreateFontA(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Consolas");
        HGDIOBJ of2 = SelectObject(memDC, hf2);
        SetTextColor(memDC, RGB(100, 100, 100));
        char finalTxt[128];
        StrCpy(finalTxt, "Score: ");
        IntToStr(score, numBuf); StrCat(finalTxt, numBuf);
        StrCat(finalTxt, "  Best: ");
        IntToStr(bestScore, numBuf); StrCat(finalTxt, numBuf);
        GetTextExtentPoint32A(memDC, finalTxt, StrLen(finalTxt), &sz);
        TextOutA(memDC, panX + (panW - sz.cx) / 2, panY + 65, finalTxt, StrLen(finalTxt));
        SetTextColor(memDC, RGB(60, 60, 60));
        const char* hTxt = "R = Restart  |  ESC = Quit";
        GetTextExtentPoint32A(memDC, hTxt, StrLen(hTxt), &sz);
        TextOutA(memDC, panX + (panW - sz.cx) / 2, panY + panH - 28, hTxt, StrLen(hTxt));
        SelectObject(memDC, of2); DeleteObject(hf2);
    }
    BitBlt(hdc, 0, 0, screenW, screenH, memDC, 0, 0, SRCCOPY);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_UP: case 'W': Turn(0, -1); break;
        case VK_DOWN: case 'S': Turn(0, 1); break;
        case VK_LEFT: case 'A': Turn(-1, 0); break;
        case VK_RIGHT:case 'D': Turn(1, 0); break;
        case 'P': if (!gameOver) paused = !paused; break;
        case 'R': ResetGame(); lastTick = GetTickCount(); break;
        case VK_ESCAPE: PostQuitMessage(0); break;
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}
extern "C" void WinMainCRTStartup() {
    rs = GetTickCount();
    lastTick = GetTickCount();
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "SnakeGame";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);
    screenW = GetSystemMetrics(SM_CXSCREEN);
    screenH = GetSystemMetrics(SM_CYSCREEN);
    HWND hwnd = CreateWindowExA(
        0, wc.lpszClassName, "Snake",
        WS_POPUP | WS_VISIBLE,
        0, 0, screenW, screenH,
        NULL, NULL, hInstance, NULL
    );
    ShowCursor(FALSE);
    ResetGame();
    MSG msg = {};
    HDC hdc = GetDC(hwnd);
    while (true) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            Update();
            Render(hdc);
            Sleep(10);
        }
    }
    ShowCursor(TRUE);
    ReleaseDC(hwnd, hdc);
    ExitProcess(0);
}