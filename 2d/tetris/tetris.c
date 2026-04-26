#pragma optimize("gsy", on)
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(linker, "/SUBSYSTEM:windows")
#pragma comment(linker, "/MERGE:.rdata=.text")
#pragma comment(linker, "/MERGE:.data=.text")
#pragma comment(linker, "/SECTION:.text,ERW")
#pragma comment(linker, "/ENTRY:WinMainCRTStartup")
#include <windows.h>
enum { C=10, R=20, B=30, TS=500, TM=80, TA=20 };
static const char P[7][4][4][2] = {
{{{0,1},{1,1},{2,1},{3,1}},{{2,0},{2,1},{2,2},{2,3}},{{0,2},{1,2},{2,2},{3,2}},{{1,0},{1,1},{1,2},{1,3}}},
{{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}}},
{{{1,0},{0,1},{1,1},{2,1}},{{1,0},{1,1},{2,1},{1,2}},{{0,1},{1,1},{2,1},{1,2}},{{1,0},{0,1},{1,1},{1,2}}},
{{{1,0},{2,0},{0,1},{1,1}},{{1,0},{1,1},{2,1},{2,2}},{{1,1},{2,1},{0,2},{1,2}},{{0,0},{0,1},{1,1},{1,2}}},
{{{0,0},{1,0},{1,1},{2,1}},{{2,0},{1,1},{2,1},{1,2}},{{0,1},{1,1},{1,2},{2,2}},{{1,0},{0,1},{1,1},{0,2}}},
{{{0,0},{0,1},{1,1},{2,1}},{{1,0},{2,0},{1,1},{1,2}},{{0,1},{1,1},{2,1},{2,2}},{{1,0},{1,1},{0,2},{1,2}}},
{{{2,0},{0,1},{1,1},{2,1}},{{1,0},{1,1},{1,2},{2,2}},{{0,1},{1,1},{2,1},{0,2}},{{0,0},{1,0},{1,1},{1,2}}}};
static const COLORREF PC[7] = {0xF0F000, 0x00F0F0, 0xF000A0, 0x00F000, 0x0000F0, 0xF00000, 0x00A0F0};
static int bd[R][C], pc, rt, px, py, np, sc, bs, lv, ln, ov, pa, sW, sH;
static DWORD ld; 
static unsigned rs;
static COLORREF ModC(COLORREF c, int m, int d) {
    int r = (GetRValue(c) * m) / d, g = (GetGValue(c) * m) / d, b = (GetBValue(c) * m) / d;
    return RGB(r > 255 ? 255 : r, g > 255 ? 255 : g, b > 255 ? 255 : b);
}
static int RP(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs % 7; }
static int F(int p, int r, int x, int y) {
    for (int i = 0; i < 4; i++) {
        int a = x + P[p][r][i][0], b = y + P[p][r][i][1];
        if (a < 0 || a >= C || b >= R) return 0;
        if (b >= 0 && bd[b][a]) return 0;
    } return 1;
}
static void CL2(void) {
    int cl = 0, y, x, yy; static const int pt[] = {0, 100, 300, 500, 800};
    for (y = R - 1; y >= 0; y--) {
        int fu = 1;
        for (x = 0; x < C; x++) if (!bd[y][x]) { fu = 0; break; }
        if (fu) {
            cl++; for (yy = y; yy > 0; yy--) for (x = 0; x < C; x++) bd[yy][x] = bd[yy - 1][x];
            for (x = 0; x < C; x++) bd[0][x] = 0; y++;
        }
    }
    if (cl) { sc += pt[cl] * lv; ln += cl; lv = 1 + ln / 10; }
}
static void Sp(void) {
    pc = np; np = RP(); rt = 0; px = C / 2 - 2; py = -1; ld = GetTickCount();
    if (!F(pc, rt, px, py)) { ov = 1; if (sc > bs) bs = sc; }
}
static void Lk(void) {
    for (int i = 0; i < 4; i++) {
        int a = px + P[pc][rt][i][0], b = py + P[pc][rt][i][1];
        if (b >= 0 && b < R && a >= 0 && a < C) bd[b][a] = pc + 1;
    } CL2(); Sp();
}
static void Rs(void) {
    for (int y = 0; y < R; y++) for (int x = 0; x < C; x++) bd[y][x] = 0;
    sc = 0; lv = 1; ln = 0; ov = 0; pa = 0; np = RP(); Sp();
}
static void DB(HDC d, int x, int y, int s, COLORREF c) {
    RECT r = {x, y, x + s, y + s}; HBRUSH b = CreateSolidBrush(c); FillRect(d, &r, b); DeleteObject(b);
    HPEN p = CreatePen(PS_SOLID, 1, ModC(c, 15, 10)); HGDIOBJ o = SelectObject(d, p);
    MoveToEx(d, x, y + s - 1, 0); LineTo(d, x, y); LineTo(d, x + s - 1, y);
    SelectObject(d, o); DeleteObject(p);
    p = CreatePen(PS_SOLID, 1, ModC(c, 4, 10)); o = SelectObject(d, p);
    MoveToEx(d, x + s - 1, y, 0); LineTo(d, x + s - 1, y + s - 1); LineTo(d, x, y + s - 1);
    SelectObject(d, o); DeleteObject(p);
    r.left += 3; r.top += 3; r.right -= 3; r.bottom -= 3;
    b = CreateSolidBrush(ModC(c, 115, 100)); FillRect(d, &r, b); DeleteObject(b);
}
static void DG(HDC d, int x, int y, int s, COLORREF c) {
    HPEN p = CreatePen(PS_SOLID, 1, ModC(c, 6, 10)); HGDIOBJ o = SelectObject(d, p);
    SelectObject(d, GetStockObject(NULL_BRUSH)); Rectangle(d, x + 2, y + 2, x + s - 2, y + s - 2);
    SelectObject(d, o); DeleteObject(p);
}
static void L(HDC d, const char* t, int x, int y, int s, COLORREF c, int b) {
    HFONT f = CreateFontA(s, 0, 0, 0, b ? FW_BOLD : FW_NORMAL, 0, 0, 0, 0, 0, 0, CLEARTYPE_QUALITY, 0, "Consolas");
    HGDIOBJ o = SelectObject(d, f); SetTextColor(d, c);
    TextOutA(d, x, y, t, lstrlenA(t)); SelectObject(d, o); DeleteObject(f);
}
static void SO(HDC d, int x, int y, int w, int h) {
    for (int i = y; i < y + h; i += 2) {
        RECT r = {x, i, x + w, i + 1}; HBRUSH b = CreateSolidBrush(0); FillRect(d, &r, b); DeleteObject(b);
    }
}
static void Rd(HDC hdc) {
    HDC d; HBITMAP bm; RECT r; HBRUSH b; HPEN p; HGDIOBJ o;
    int bw = C * B, bh = R * B, ox = (sW - bw - 200) / 2, oy = (sH - bh) / 2, ix = ox + bw + 30, iy, i;
    char buf[64]; SIZE sz;
    d = CreateCompatibleDC(hdc); bm = CreateCompatibleBitmap(hdc, sW, sH); SelectObject(d, bm);
    SetRect(&r, 0, 0, sW, sH); b = CreateSolidBrush(RGB(15, 15, 15)); FillRect(d, &r, b); DeleteObject(b);
    SetRect(&r, ox - 4, oy - 4, ox + bw + 4, oy + bh + 4); b = CreateSolidBrush(RGB(40, 40, 40)); FillRect(d, &r, b); DeleteObject(b);
    SetRect(&r, ox, oy, ox + bw, oy + bh); b = CreateSolidBrush(RGB(25, 25, 30)); FillRect(d, &r, b); DeleteObject(b);
    p = CreatePen(PS_SOLID, 1, RGB(30, 30, 38)); o = SelectObject(d, p);
    for (i = 0; i <= C; i++) {MoveToEx(d, ox + i * B, oy, 0); LineTo(d, ox + i * B, oy + bh);}
    for (i = 0; i <= R; i++) {MoveToEx(d, ox, oy + i * B, 0); LineTo(d, ox + bw, oy + i * B);}
    SelectObject(d, o); DeleteObject(p);
    for (int y = 0; y < R; y++) for (int x = 0; x < C; x++) if (bd[y][x]) DB(d, ox + x * B, oy + y * B, B, PC[bd[y][x] - 1]);
    if (!ov && !pa) {
        int gy = py; while (F(pc, rt, px, gy + 1)) gy++;
        if (gy != py) for (i = 0; i < 4; i++) {int a = px + P[pc][rt][i][0], e = gy + P[pc][rt][i][1]; if (e >= 0 && e < R && a >= 0 && a < C) DG(d, ox + a * B, oy + e * B, B, PC[pc]);}
    }
    if (!ov) for (i = 0; i < 4; i++) {int a = px + P[pc][rt][i][0], e = py + P[pc][rt][i][1]; if (e >= 0 && e < R && a >= 0 && a < C) DB(d, ox + a * B, oy + e * B, B, PC[pc]);}
    SetBkMode(d, TRANSPARENT); L(d, "NEXT", ix, oy, 20, RGB(100, 100, 100), 1);
    {int qx = ix, qy = oy + 30, qs = 20; SetRect(&r, qx - 5, qy - 5, qx + 4 * qs + 5, qy + 4 * qs + 5);
      b = CreateSolidBrush(RGB(20, 20, 25)); FillRect(d, &r, b); DeleteObject(b);
      p = CreatePen(PS_SOLID, 1, RGB(40, 40, 40)); o = SelectObject(d, p);
      SelectObject(d, GetStockObject(NULL_BRUSH)); Rectangle(d, r.left, r.top, r.right, r.bottom);
      SelectObject(d, o); DeleteObject(p);
      for (i = 0; i < 4; i++) DB(d, qx + P[np][0][i][0] * qs, qy + P[np][0][i][1] * qs, qs, PC[np]);}
    iy = oy + 130;
    L(d, "SCORE", ix, iy, 18, RGB(100, 100, 100), 1); wsprintfA(buf, "%d", sc); L(d, buf, ix, iy + 22, 24, RGB(100, 220, 120),1);
    L(d, "BEST", ix, iy + 60, 18, RGB(100, 100, 100), 1); wsprintfA(buf, "%d", bs); L(d, buf, ix, iy + 82, 24, RGB(180, 180, 100),1);
    L(d, "LEVEL", ix, iy + 120, 18, RGB(100, 100, 100), 1); wsprintfA(buf, "%d", lv); L(d, buf, ix, iy + 142, 24, RGB(120, 160, 240),1);
    L(d, "LINES", ix, iy + 180, 18, RGB(100, 100, 100), 1); wsprintfA(buf, "%d", ln); L(d, buf, ix, iy + 202, 24, RGB(200, 120, 200),1);
    L(d, "Arrows/WASD Move  Up/W Rotate", ox, oy + bh + 15, 14, RGB(50, 50, 50), 0);
    L(d, "Space=Drop P=Pause R=Reset ESC=Quit", ox, oy + bh + 33, 14, RGB(50, 50, 50), 0);
    if (pa && !ov) {
        SO(d, ox, oy, bw, bh); HFONT f = CreateFontA(60, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, CLEARTYPE_QUALITY, 0, "Consolas");
        HGDIOBJ fo = SelectObject(d, f); SetTextColor(d, RGB(200, 200, 200)); GetTextExtentPoint32A(d, "PAUSED", 6, &sz);
        TextOutA(d, ox + (bw - sz.cx) / 2, oy + (bh - sz.cy) / 2, "PAUSED", 6); SelectObject(d, fo); DeleteObject(f);
    }
    if (ov) {
        int pw = 280, ph = 130, qx = ox + (bw - pw) / 2, qy = oy + (bh - ph) / 2, len; SO(d, ox, oy, bw, bh);
        SetRect(&r, qx, qy, qx + pw, qy + ph); b = CreateSolidBrush(RGB(15, 15, 15)); FillRect(d, &r, b); DeleteObject(b);
        p = CreatePen(PS_SOLID, 2, RGB(231, 76, 60)); o = SelectObject(d, p);
        SelectObject(d, GetStockObject(NULL_BRUSH)); Rectangle(d, qx, qy, qx + pw, qy + ph); SelectObject(d, o); DeleteObject(p);
        HFONT f = CreateFontA(40, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, CLEARTYPE_QUALITY, 0, "Consolas");
        HGDIOBJ fo = SelectObject(d, f); SetTextColor(d, RGB(231, 76, 60)); GetTextExtentPoint32A(d, "GAME OVER", 9, &sz);
        TextOutA(d, qx + (pw - sz.cx) / 2, qy + 12, "GAME OVER", 9); SelectObject(d, fo); DeleteObject(f);
        f = CreateFontA(18, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, CLEARTYPE_QUALITY, 0, "Consolas"); fo = SelectObject(d, f);
        SetTextColor(d, RGB(100, 100, 100));
        len = wsprintfA(buf, "Score: %d  Best: %d", sc, bs);
        GetTextExtentPoint32A(d, buf, len, &sz); TextOutA(d, qx + (pw - sz.cx) / 2, qy + 65, buf, len);
        SetTextColor(d, RGB(60, 60, 60)); GetTextExtentPoint32A(d, "R=Restart | ESC=Quit", 20, &sz);
        TextOutA(d, qx + (pw - sz.cx) / 2, qy + ph - 28, "R=Restart | ESC=Quit", 20); SelectObject(d, fo); DeleteObject(f);
    }
    BitBlt(hdc, 0, 0, sW, sH, d, 0, 0, SRCCOPY); DeleteObject(bm); DeleteDC(d);
}
static LRESULT CALLBACK WP(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
        case WM_KEYDOWN: switch (w) {
            case VK_LEFT: case 'A': if (!ov && !pa && F(pc, rt, px - 1, py)) px--; break;
            case VK_RIGHT:case 'D': if (!ov && !pa && F(pc, rt, px + 1, py)) px++; break;
            case VK_DOWN: case 'S': if (!ov && !pa) { if (F(pc, rt, px, py + 1)) { py++; sc++; } else Lk(); ld = GetTickCount();} break;
            case VK_UP:   case 'W': if (!ov && !pa) { int n = (rt + 1) % 4; if(F(pc,n,px,py)) rt = n; else if (F(pc,n,px - 1, py)) {px--; rt = n;} else if (F(pc, n, px + 1, py)) {px++; rt = n;} else if (F(pc,n,px - 2,py)){px -= 2; rt = n;}else if(F(pc,n,px + 2,py)) {px += 2; rt = n;}} break;
            case VK_SPACE: if (!ov && !pa) { while (F(pc, rt, px, py + 1)) {py++; sc += 2;} Lk(); ld = GetTickCount();} break;
            case 'P': if (!ov) pa = !pa; break; case 'R': Rs(); break;
            case VK_ESCAPE: PostQuitMessage(0); break;
        } return 0;
        case WM_DESTROY: PostQuitMessage(0); return 0;
    } return DefWindowProcA(h, m, w, l);
}

void WinMainCRTStartup(void) {
    HWND hw; MSG m; HDC dc; WNDCLASSA wc;
    wc.style = 0;
    wc.lpfnWndProc = WP;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(0);
    wc.hIcon = 0;
    wc.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = "T";
    rs = GetTickCount(); if (!rs) rs = 1;
    RegisterClassA(&wc);
    sW = GetSystemMetrics(SM_CXSCREEN); sH = GetSystemMetrics(SM_CYSCREEN);
    hw = CreateWindowExA(0, "T", "Tetris", WS_POPUP | WS_VISIBLE, 0, 0, sW, sH, 0, 0, wc.hInstance, 0);
    ShowCursor(0); Rs(); dc = GetDC(hw);
    for (;;) {
        if (PeekMessageA(&m, 0, 0, 0, PM_REMOVE)) {
            if (m.message == WM_QUIT) break;
            TranslateMessage(&m); DispatchMessageA(&m);
        } else {
            if (!ov && !pa) {
                DWORD now = GetTickCount();
                int t = TS - (lv - 1) * TA; if (t < TM) t = TM;
                if (now - ld >= (DWORD)t) {
                    if (F(pc, rt, px, py + 1)) py++; else Lk(); ld = GetTickCount();
                }
            }
            Rd(dc); Sleep(10);
        }
    }
    ShowCursor(1); ReleaseDC(hw, dc);
    ExitProcess(0);
}
