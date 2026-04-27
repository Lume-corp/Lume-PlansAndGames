#pragma optimize("gsy", on)
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup")
#pragma comment(linker, "/MERGE:.rdata=.text")
#pragma comment(linker, "/MERGE:.data=.text")
#pragma comment(linker, "/SECTION:.text,ERW")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
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
int WStrLen(const wchar_t* s) { int l = 0; while (s[l]) l++; return l; }
int StrLen(const char* s) { int l = 0; while (s[l]) l++; return l; }
void WStrCpy(wchar_t* d, const wchar_t* s) { while (*s) *d++ = *s++; *d = 0; }
void WStrCat(wchar_t* d, const wchar_t* s) { while (*d) d++; while (*s) *d++ = *s++; *d = 0; }
void IntToStrW(int v, wchar_t* d) {
    if (v == 0) { d[0] = L'0'; d[1] = 0; return; }
    if (v < 0) { *d++ = L'-'; v = -v; }
    wchar_t b[16]; int l = 0;
    while (v) { b[l++] = (v % 10) + L'0'; v /= 10; }
    while (l > 0) *d++ = b[--l];
    *d = 0;
}
void* MemAlloc(size_t size) { return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size); }
void MemFree(void* ptr) { if (ptr) HeapFree(GetProcessHeap(), 0, ptr); }
static const wchar_t OPEN_W[]  = { L'(', L'[', L'{', L'<', 0x2045 };
static const wchar_t CLOSE_W[] = { L')', L']', L'}', L'>', 0x2046 };
static void byte_to_digits(unsigned char b, int d[4]) {
    d[3] = b % 5; b /= 5;
    d[2] = b % 5; b /= 5;
    d[1] = b % 5; b /= 5;
    d[0] = b % 5;
}
static int digits_to_byte(int d[4]) {
    return d[0] * 125 + d[1] * 25 + d[2] * 5 + d[3];
}
static void encode_bytes(const unsigned char* input, int inputLen, wchar_t* output, int outmax) {
    int pos = 0;
    for (int i = 0; i < inputLen; i++) {
        int d[4]; byte_to_digits(input[i], d);
        for (int j = 0; j < 4; j++) {
            if (pos >= outmax - 1) goto done;
            output[pos++] = OPEN_W[d[j]];
        }
        for (int j = 3; j >= 0; j--) {
            if (pos >= outmax - 1) goto done;
            output[pos++] = CLOSE_W[d[j]];
        }
    }
done:
    output[pos] = 0;
}
static int decode_to_bytes(const wchar_t* input, unsigned char* output, int outmax) {
    int pos = 0; int len = WStrLen(input); int opos = 0;
    while (pos < len && opos < outmax - 1) {
        while (pos < len && (input[pos] == L' ' || input[pos] == L'\r' || input[pos] == L'\n' || input[pos] == L'\t')) pos++;
        if (pos >= len) break;
        int d[4]; int valid = 1;
        for (int j = 0; j < 4; j++) {
            if (pos >= len) { valid = 0; break; }
            wchar_t c = input[pos++]; int found = -1;
            for (int k = 0; k < 5; k++) { if (c == OPEN_W[k]) { found = k; break; } }
            if (found < 0) { valid = 0; break; }
            d[j] = found;
        }
        if (!valid) break;
        for (int j = 3; j >= 0; j--) { if (pos < len) pos++; }
        int val = digits_to_byte(d);
        if (val > 255) val = '?';
        output[opos++] = (unsigned char)val;
    }
    output[opos] = 0; return opos;
}
static void encode_wide(const wchar_t* input, wchar_t* output, int outmax) {
    int needed = WideCharToMultiByte(CP_UTF8, 0, input, -1, NULL, 0, NULL, NULL);
    char* utf8 = (char*)MemAlloc(needed + 1);
    WideCharToMultiByte(CP_UTF8, 0, input, -1, utf8, needed, NULL, NULL);
    encode_bytes((unsigned char*)utf8, StrLen(utf8), output, outmax);
    MemFree(utf8);
}
static void decode_wide(const wchar_t* input, wchar_t* output, int outmax) {
    unsigned char* bytes = (unsigned char*)MemAlloc(outmax);
    int blen = decode_to_bytes(input, bytes, outmax);
    int wlen = MultiByteToWideChar(CP_UTF8, 0, (char*)bytes, blen, output, outmax - 1);
    output[wlen] = 0;
    MemFree(bytes);
}
#define ID_INPUT 101
#define ID_OUTPUT 102
#define ID_ENCODE 103
#define ID_DECODE 104
#define ID_CLEAR 105
#define ID_COPY 106
#define ID_SWAP 107
#define ID_STATUS 108
static HWND hInput, hOutput, hStatus;
static HFONT hFont, hFontMono, hFontBtn, hFontTitle;
static HBRUSH hBrBg, hBrEdit, hBrOut;
static const wchar_t CLASS_NAME[] = L"BracketLangWindow";
#define CLR_BG RGB(30, 30, 46)
#define CLR_EDIT_BG RGB(49, 50, 68)
#define CLR_OUT_BG RGB(39, 40, 55)
#define CLR_TEXT RGB(205, 214, 244)
#define CLR_BTN_ENC RGB(137, 180, 250)
#define CLR_BTN_DEC RGB(166, 227, 161)
#define CLR_BTN_CLR RGB(243, 139, 168)
#define CLR_BTN_CPY RGB(249, 226, 175)
#define CLR_BTN_SWP RGB(203, 166, 247)
struct BtnInfo { HWND hwnd; COLORREF color; wchar_t text[32]; };
static BtnInfo buttons[5];
static int btnCount = 0;
static void AddButton(HWND h, COLORREF c, const wchar_t* t) {
    if (btnCount < 5) {
        buttons[btnCount].hwnd = h; buttons[btnCount].color = c;
        lstrcpynW(buttons[btnCount].text, t, 32); btnCount++;
    }
}
static void SetStatus(const wchar_t* text) { SetWindowTextW(hStatus, text); }
static void DoEncode() {
    int len = GetWindowTextLengthW(hInput);
    if (len == 0) { SetStatus(L"Input is empty!"); return; }
    wchar_t* inp = (wchar_t*)MemAlloc((len + 2) * sizeof(wchar_t));
    GetWindowTextW(hInput, inp, len + 1);
    int outmax = (len + 1) * 40;
    wchar_t* out = (wchar_t*)MemAlloc(outmax * sizeof(wchar_t));
    encode_wide(inp, out, outmax);
    SetWindowTextW(hOutput, out);
    wchar_t status[256]; wchar_t numBuf[32];
    WStrCpy(status, L"Encoded "); IntToStrW(len, numBuf); WStrCat(status, numBuf);
    WStrCat(status, L" chars -> "); IntToStrW(WStrLen(out), numBuf); WStrCat(status, numBuf);
    WStrCat(status, L" bracket symbols");
    SetStatus(status);
    MemFree(inp); MemFree(out);
}
static void DoDecode() {
    int len = GetWindowTextLengthW(hInput);
    if (len == 0) { SetStatus(L"Input is empty!"); return; }
    wchar_t* inp = (wchar_t*)MemAlloc((len + 2) * sizeof(wchar_t));
    GetWindowTextW(hInput, inp, len + 1);
    int outmax = len + 256;
    wchar_t* out = (wchar_t*)MemAlloc(outmax * sizeof(wchar_t));
    decode_wide(inp, out, outmax);
    SetWindowTextW(hOutput, out);
    wchar_t status[256]; wchar_t numBuf[32];
    WStrCpy(status, L"Decoded "); IntToStrW(len, numBuf); WStrCat(status, numBuf);
    WStrCat(status, L" symbols.");
    SetStatus(status);
    MemFree(inp); MemFree(out);
}
static void DoCopy() {
    int len = GetWindowTextLengthW(hOutput);
    if (len == 0) { SetStatus(L"Output is empty!"); return; }
    wchar_t* txt = (wchar_t*)MemAlloc((len + 2) * sizeof(wchar_t));
    GetWindowTextW(hOutput, txt, len + 1);
    if (OpenClipboard(NULL)) {
        EmptyClipboard();
        HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(wchar_t));
        if (hg) {
            wchar_t* p = (wchar_t*)GlobalLock(hg);
            memcpy(p, txt, (len + 1) * sizeof(wchar_t));
            GlobalUnlock(hg); SetClipboardData(CF_UNICODETEXT, hg);
        }
        CloseClipboard(); SetStatus(L"Copied to clipboard!");
    }
    MemFree(txt);
}
static void DoSwap() {
    int len1 = GetWindowTextLengthW(hOutput);
    if (len1 == 0) { SetStatus(L"Output is empty!"); return; }
    wchar_t* txt = (wchar_t*)MemAlloc((len1 + 2) * sizeof(wchar_t));
    GetWindowTextW(hOutput, txt, len1 + 1);
    SetWindowTextW(hInput, txt); SetWindowTextW(hOutput, L"");
    SetStatus(L"Swapped output -> input");
    MemFree(txt);
}
static void DoClear() {
    SetWindowTextW(hInput, L""); SetWindowTextW(hOutput, L"");
    SetStatus(L"Cleared.");
}
static void DrawBtn(DRAWITEMSTRUCT* di) {
    for (int i = 0; i < btnCount; i++) {
        if (buttons[i].hwnd == di->hwndItem) {
            COLORREF col = buttons[i].color;
            bool pressed = (di->itemState & ODS_SELECTED) != 0;
            bool focused = (di->itemState & ODS_FOCUS) != 0;
            if (pressed) {
                int r = GetRValue(col) * 7 / 10;
                int g = GetGValue(col) * 7 / 10;
                int b = GetBValue(col) * 7 / 10;
                col = RGB(r, g, b);
            }
            HBRUSH br = CreateSolidBrush(col);
            HPEN pen = CreatePen(PS_SOLID, focused ? 2 : 1, focused ? RGB(255, 255, 255) : col);
            HBRUSH oldBr = (HBRUSH)SelectObject(di->hDC, br);
            HPEN oldPen = (HPEN)SelectObject(di->hDC, pen);
            RoundRect(di->hDC, di->rcItem.left, di->rcItem.top, di->rcItem.right, di->rcItem.bottom, 14, 14);
            SelectObject(di->hDC, oldBr); SelectObject(di->hDC, oldPen);
            DeleteObject(br); DeleteObject(pen);
            SetBkMode(di->hDC, TRANSPARENT); SetTextColor(di->hDC, RGB(30, 30, 46));
            SelectObject(di->hDC, hFontBtn);
            DrawTextW(di->hDC, buttons[i].text, -1, &di->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            return;
        }
    }
}
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        hBrBg = CreateSolidBrush(CLR_BG); hBrEdit = CreateSolidBrush(CLR_EDIT_BG); hBrOut = CreateSolidBrush(CLR_OUT_BG);
        hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        hFontMono = CreateFontW(17, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Consolas");
        hFontBtn = CreateFontW(15, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        hFontTitle = CreateFontW(24, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        int y = 10;
        HWND hTitle = CreateWindowW(L"STATIC", L"\x2045 Bracket Language \x2046", WS_CHILD | WS_VISIBLE | SS_CENTER, 10, y, 560, 30, hwnd, NULL, NULL, NULL);
        SendMessageW(hTitle, WM_SETFONT, (WPARAM)hFontTitle, TRUE); y += 38;
        HWND hLbl1 = CreateWindowW(L"STATIC", L"INPUT:", WS_CHILD | WS_VISIBLE, 12, y, 100, 20, hwnd, NULL, NULL, NULL);
        SendMessageW(hLbl1, WM_SETFONT, (WPARAM)hFontBtn, TRUE); y += 22;
        hInput = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL, 10, y, 560, 90, hwnd, (HMENU)ID_INPUT, NULL, NULL);
        SendMessageW(hInput, WM_SETFONT, (WPARAM)hFontMono, TRUE); y += 98;
        int bw = 100, bh = 34, gap = 10; int bx = 10;
        HWND b1 = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, bx, y, bw, bh, hwnd, (HMENU)ID_ENCODE, NULL, NULL); AddButton(b1, CLR_BTN_ENC, L"ENCODE"); bx += bw + gap;
        HWND b2 = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, bx, y, bw, bh, hwnd, (HMENU)ID_DECODE, NULL, NULL); AddButton(b2, CLR_BTN_DEC, L"DECODE"); bx += bw + gap;
        HWND b3 = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, bx, y, bw, bh, hwnd, (HMENU)ID_SWAP, NULL, NULL);   AddButton(b3, CLR_BTN_SWP, L"SWAP"); bx += bw + gap;
        HWND b4 = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, bx, y, bw, bh, hwnd, (HMENU)ID_COPY, NULL, NULL);   AddButton(b4, CLR_BTN_CPY, L"COPY"); bx += bw + gap;
        HWND b5 = CreateWindowW(L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, bx, y, bw, bh, hwnd, (HMENU)ID_CLEAR, NULL, NULL);  AddButton(b5, CLR_BTN_CLR, L"CLEAR");
        y += bh + 10;
        HWND hLbl2 = CreateWindowW(L"STATIC", L"OUTPUT:", WS_CHILD | WS_VISIBLE, 12, y, 100, 20, hwnd, NULL, NULL, NULL);
        SendMessageW(hLbl2, WM_SETFONT, (WPARAM)hFontBtn, TRUE); y += 22;
        hOutput = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY, 10, y, 560, 110, hwnd, (HMENU)ID_OUTPUT, NULL, NULL);
        SendMessageW(hOutput, WM_SETFONT, (WPARAM)hFontMono, TRUE); y += 118;
        hStatus = CreateWindowW(L"STATIC", L"Type text and press ENCODE, or paste brackets and press DECODE.", WS_CHILD | WS_VISIBLE | SS_LEFT, 12, y, 556, 22, hwnd, (HMENU)ID_STATUS, NULL, NULL);
        SendMessageW(hStatus, WM_SETFONT, (WPARAM)hFont, TRUE); y += 28;
        HWND hLeg = CreateWindowW(L"STATIC", L"()=0  []=1  {}=2  <>=3  \x2045\x2046=4  | base-5, 4 digits/byte", WS_CHILD | WS_VISIBLE | SS_CENTER, 10, y, 560, 20, hwnd, NULL, NULL, NULL);
        SendMessageW(hLeg, WM_SETFONT, (WPARAM)hFontMono, TRUE);
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wp; SetTextColor(hdc, CLR_TEXT); SetBkColor(hdc, CLR_BG); return (LRESULT)hBrBg;
    }
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wp; HWND h = (HWND)lp; SetTextColor(hdc, CLR_TEXT);
        if (h == hOutput) { SetBkColor(hdc, CLR_OUT_BG); return (LRESULT)hBrOut; }
        SetBkColor(hdc, CLR_EDIT_BG); return (LRESULT)hBrEdit;
    }
    case WM_ERASEBKGND: { RECT rc; GetClientRect(hwnd, &rc); FillRect((HDC)wp, &rc, hBrBg); return 1; }
    case WM_DRAWITEM: DrawBtn((DRAWITEMSTRUCT*)lp); return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case ID_ENCODE: DoEncode(); break;
        case ID_DECODE: DoDecode(); break;
        case ID_CLEAR:  DoClear();  break;
        case ID_COPY:   DoCopy();   break;
        case ID_SWAP:   DoSwap();   break;
        }
        return 0;
    case WM_DESTROY:
        DeleteObject(hFont); DeleteObject(hFontMono); DeleteObject(hFontBtn); DeleteObject(hFontTitle);
        DeleteObject(hBrBg); DeleteObject(hBrEdit); DeleteObject(hBrOut);
        PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
extern "C" void WinMainCRTStartup() {
    HINSTANCE hInst = GetModuleHandle(NULL);
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassW(&wc);
    int ww = 596, wh = 460;
    int sx = (GetSystemMetrics(SM_CXSCREEN) - ww) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - wh) / 2;
    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"\x2045 Bracket Language \x2046",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        sx, sy, ww, wh, NULL, NULL, hInst, NULL);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    ExitProcess((UINT)msg.wParam);
}