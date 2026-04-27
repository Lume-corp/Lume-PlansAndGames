#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#pragma optimize("gsy", on)
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup")
#pragma comment(linker, "/MERGE:.rdata=.text")
#pragma comment(linker, "/MERGE:.data=.text")
#pragma comment(linker, "/SECTION:.text,ERW")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <uxtheme.h>
typedef decltype(sizeof(0)) size_t;
#pragma function(memset)
extern "C" void* __cdecl memset(void* dest, int c, size_t count) {
    char* d = (char*)dest; while (count--) *d++ = (char)c; return dest;
}
#pragma function(memcpy)
extern "C" void* __cdecl memcpy(void* dest, const void* src, size_t count) {
    char* d = (char*)dest; const char* s = (const char*)src; while (count--) *d++ = *s++; return dest;
}
void* MemAlloc(size_t size) {return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);}
void* MemReAlloc(void* ptr, size_t size) {return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ptr, size);}
void MemFree(void* ptr) {if (ptr) HeapFree(GetProcessHeap(), 0, ptr);}

int WStrLen(const wchar_t* s) {int l = 0; while (s[l]) l++; return l;}
int StrLen(const char* s) {int l = 0; while (s[l]) l++; return l;}
void WStrCpy(wchar_t* d, const wchar_t* s) {while (*s) *d++ = *s++; *d = 0;}
void WStrCat(wchar_t* d, const wchar_t* s) {while (*d) d++; while (*s) *d++ = *s++; *d = 0;}
wchar_t* WStrDup(const wchar_t* s) {
    wchar_t* d = (wchar_t*)MemAlloc((WStrLen(s) + 1) * sizeof(wchar_t));
    WStrCpy(d, s); return d;
}
const wchar_t* WStrChr(const wchar_t* s, wchar_t c) {
    while (*s) {if (*s == c) return s; s++;} return nullptr;
}
const wchar_t* WStrStr(const wchar_t* str, const wchar_t* sub) {
    if (!*sub) return str;
    while (*str) {
        const wchar_t* p1 = str, *p2 = sub;
        while (*p1 && *p2 && *p1 == *p2) {p1++; p2++;}
        if (!*p2) return str; str++;
    } return nullptr;
}
void ToLowerInPlace(wchar_t* s) {
    while (*s) {if (*s >= L'A' && *s <= L'Z') *s += 32; s++;}
}
#define ID_FILE_NEW     1001
#define ID_FILE_LOAD    1002
#define ID_FILE_SAVE    1003
#define ID_BTN_ADD      2001
#define ID_BTN_DEL      2002
#define ID_BTN_EDIT     2003
#define ID_SEARCH_EDIT  2004
#define ID_LISTVIEW     3001
#define ID_INPUT_WORD   4001
#define ID_INPUT_TRANS  4002
#define ID_INPUT_OK     4003
#define ID_FAKE_HEADER  5001
#define HEADER_H        24
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
HWND hMainWnd, hListView, hFakeHeader;
HWND hBtnAdd, hBtnDel, hBtnEdit, hSearchEdit;
wchar_t currentFilePath[MAX_PATH] = L"";
bool isDirty = false;
HBRUSH hbrBg = NULL, hbrCtrl = NULL;
HFONT hFont = NULL;
COLORREF colorText = RGB(230, 230, 230);
COLORREF colorBg = RGB(30, 30, 30);
COLORREF colorCtrl = RGB(45, 45, 45);
COLORREF colorHeader = RGB(35, 35, 35);
struct DictEntry {wchar_t* word; wchar_t* translation;};
DictEntry* dictionary = nullptr;
int dictCount = 0, dictCap = 0;
int* filteredIndices = nullptr;
int filteredCount = 0, filteredCap = 0;
wchar_t currentSearchText[256] = L"";
wchar_t input_wordResult[256] = L"";
wchar_t input_transResult[256] = L"";
bool input_success = false;
void DictClear() {
    for (int i = 0; i < dictCount; i++) {MemFree(dictionary[i].word); MemFree(dictionary[i].translation);}
    dictCount = 0;
}
void DictPush(const wchar_t* w, const wchar_t* t) {
    if (dictCount >= dictCap) {
        dictCap = dictCap ? dictCap * 2 : 16;
        dictionary = dictionary ? (DictEntry*)MemReAlloc(dictionary, dictCap * sizeof(DictEntry)) : (DictEntry*)MemAlloc(dictCap * sizeof(DictEntry));
    }
    dictionary[dictCount].word = WStrDup(w);
    dictionary[dictCount].translation = WStrDup(t);
    dictCount++;
}
void DictErase(int idx) {
    MemFree(dictionary[idx].word); MemFree(dictionary[idx].translation);
    for (int i = idx; i < dictCount - 1; i++) dictionary[i] = dictionary[i + 1];
    dictCount--;
}
void UpdateTitle() {
    wchar_t t[MAX_PATH + 64] = L"Fast Dictionary (Dark)";
    if (currentFilePath[0] != 0) {
        WStrCat(t, L" - ");
        const wchar_t* p = currentFilePath;
        const wchar_t* lastSlash = WStrChr(p, L'\\');
        const wchar_t* temp = p;
        while ((temp = WStrChr(temp, L'\\')) != nullptr) { lastSlash = temp; temp++; }
        const wchar_t* lastFSlash = p; temp = p;
        while ((temp = WStrChr(temp, L'/')) != nullptr) { lastFSlash = temp; temp++; }
        
        if (lastSlash && lastFSlash) p = (lastSlash > lastFSlash) ? lastSlash + 1 : lastFSlash + 1;
        else if (lastSlash) p = lastSlash + 1;
        else if (lastFSlash) p = lastFSlash + 1;
        WStrCat(t, p);
    }
    if (isDirty) WStrCat(t, L" *");
    SetWindowTextW(hMainWnd, t);
}
void UpdateListView() {
    ListView_DeleteAllItems(hListView);
    for (int i = 0; i < filteredCount; ++i) {
        int idx = filteredIndices[i];
        LVITEMW lvi = { 0 }; lvi.mask = LVIF_TEXT; lvi.iItem = i;
        lvi.pszText = dictionary[idx].word;
        ListView_InsertItem(hListView, &lvi);
        ListView_SetItemText(hListView, i, 1, dictionary[idx].translation);
    }
}
bool MatchesFilter(const wchar_t* w, const wchar_t* t) {
    if (currentSearchText[0] == 0) return true;
    wchar_t query[256]; WStrCpy(query, currentSearchText); ToLowerInPlace(query);
    wchar_t* lw = WStrDup(w); ToLowerInPlace(lw);
    wchar_t* lt = WStrDup(t); ToLowerInPlace(lt);
    bool match = (WStrStr(lw, query) != nullptr) || (WStrStr(lt, query) != nullptr);
    MemFree(lw); MemFree(lt);
    return match;
}
void ApplyFilter() {
    filteredCount = 0;
    if (filteredCap < dictCount) {
        filteredCap = dictCount;
        if (filteredIndices) MemFree(filteredIndices);
        filteredIndices = (int*)MemAlloc(filteredCap * sizeof(int));
    }
    for (int i = 0; i < dictCount; ++i) {
        if (MatchesFilter(dictionary[i].word, dictionary[i].translation)) {
            filteredIndices[filteredCount++] = i;
        }
    }
    UpdateListView();
}
LRESULT CALLBACK FakeHeaderProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_ERASEBKGND) return 1;
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        HBRUSH br = CreateSolidBrush(colorHeader); FillRect(hdc, &rc, br); DeleteObject(br);
        int col0w = ListView_GetColumnWidth(hListView, 0);
        HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
        SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, colorText);
        RECT r0 = {rc.left + 6, rc.top, rc.left + col0w, rc.bottom};
        DrawTextW(hdc, L"Word", -1, &r0, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        RECT r1 = {rc.left + col0w + 6, rc.top, rc.right, rc.bottom};
        DrawTextW(hdc, L"Translation", -1, &r1, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, oldFont);
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(70, 70, 70)); HGDIOBJ op = SelectObject(hdc, pen);
        MoveToEx(hdc, rc.left + col0w, rc.top, NULL); LineTo(hdc, rc.left + col0w, rc.bottom);
        MoveToEx(hdc, rc.left, rc.bottom - 1, NULL); LineTo(hdc, rc.right, rc.bottom - 1);
        SelectObject(hdc, op); DeleteObject(pen); EndPaint(hwnd, &ps); return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
bool SaveDictionary() {
    if (currentFilePath[0] == 0) return false;
    HANDLE hFile = CreateFileW(currentFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD written;
    WriteFile(hFile, "{\n", 2, &written, NULL);
    for (int i = 0; i < dictCount; ++i) {
        int nw = WideCharToMultiByte(CP_UTF8, 0, dictionary[i].word, -1, NULL, 0, NULL, NULL);
        char* utf8w = (char*)MemAlloc(nw);
        WideCharToMultiByte(CP_UTF8, 0, dictionary[i].word, -1, utf8w, nw, NULL, NULL);
        int nt = WideCharToMultiByte(CP_UTF8, 0, dictionary[i].translation, -1, NULL, 0, NULL, NULL);
        char* utf8t = (char*)MemAlloc(nt);
        WideCharToMultiByte(CP_UTF8, 0, dictionary[i].translation, -1, utf8t, nt, NULL, NULL);
        WriteFile(hFile, "  \"", 3, &written, NULL); WriteFile(hFile, utf8w, nw - 1, &written, NULL);
        WriteFile(hFile, "\": \"", 4, &written, NULL); WriteFile(hFile, utf8t, nt - 1, &written, NULL);
        if (i + 1 < dictCount) WriteFile(hFile, "\",\n", 3, &written, NULL);
        else WriteFile(hFile, "\"\n", 2, &written, NULL);
        MemFree(utf8w); MemFree(utf8t);
    }
    WriteFile(hFile, "}\n", 2, &written, NULL);
    CloseHandle(hFile);
    isDirty = false; UpdateTitle(); return true;
}
bool PromptSave(HWND hwnd) {
    wchar_t fn[MAX_PATH] = L""; OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"JSON Files\0*.json\0All Files\0*.*\0";
    ofn.lpstrFile = fn; ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST; ofn.lpstrDefExt = L"json";
    if (GetSaveFileNameW(&ofn)) {WStrCpy(currentFilePath, fn); return SaveDictionary();}
    return false;
}
void LoadDictionary(const wchar_t* path) {
    HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;
    DWORD size = GetFileSize(hFile, NULL);
    char* content = (char*)MemAlloc(size + 1);
    DWORD read; ReadFile(hFile, content, size, &read, NULL); CloseHandle(hFile);
    int wlen = MultiByteToWideChar(CP_UTF8, 0, content, size, NULL, 0);
    wchar_t* wc = (wchar_t*)MemAlloc((wlen + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, content, size, wc, wlen); MemFree(content);
    DictClear();
    const wchar_t* pos = wc;
    while ((pos = WStrChr(pos, L'\"')) != nullptr) {
        const wchar_t* ew = WStrChr(pos + 1, L'\"'); if (!ew) break;
        int wLen = ew - (pos + 1);
        wchar_t* word = (wchar_t*)MemAlloc((wLen + 1) * sizeof(wchar_t));
        memcpy(word, pos + 1, wLen * sizeof(wchar_t));
        pos = WStrChr(ew + 1, L':'); if (!pos) {MemFree(word); break;}
        pos = WStrChr(pos, L'\"'); if (!pos) {MemFree(word); break;}
        const wchar_t* et = WStrChr(pos + 1, L'\"'); if (!et) {MemFree(word); break;}
        int tLen = et - (pos + 1);
        wchar_t* trans = (wchar_t*)MemAlloc((tLen + 1) * sizeof(wchar_t));
        memcpy(trans, pos + 1, tLen * sizeof(wchar_t));
        DictPush(word, trans);
        MemFree(word); MemFree(trans);
        pos = et + 1;
    }
    MemFree(wc); isDirty = false; ApplyFilter(); UpdateTitle();
}
bool CheckUnsavedChanges(HWND hwnd) {
    if (!isDirty) return true;
    int r = MessageBoxW(hwnd, L"You have unsaved changes.\nDo you want to save before exiting?", L"Unsaved Changes", MB_YESNOCANCEL | MB_ICONWARNING);
    if (r == IDCANCEL) return false;
    if (r == IDYES) { if (currentFilePath[0] == 0) {if (!PromptSave(hwnd)) return false;} else SaveDictionary();}
    return true;
}
void DrawColoredButton(LPDRAWITEMSTRUCT pdis, COLORREF color) {
    HDC hdc = pdis->hDC; RECT rect = pdis->rcItem;
    if (pdis->itemState & ODS_SELECTED) color = RGB((BYTE)(GetRValue(color) * 7 / 10), (BYTE)(GetGValue(color) * 7 / 10), (BYTE)(GetBValue(color) * 7 / 10));
    HBRUSH br = CreateSolidBrush(color); FillRect(hdc, &rect, br); DeleteObject(br);
    HBRUSH bb = CreateSolidBrush(RGB(20, 20, 20)); FrameRect(hdc, &rect, bb); DeleteObject(bb);
    wchar_t text[64]; GetWindowTextW(pdis->hwndItem, text, 64);
    SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, RGB(255, 255, 255));
    DrawTextW(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}
LRESULT CALLBACK InputWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        HWND ew = CreateWindowW(L"EDIT", input_wordResult, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 100, 10, 200, 20, hwnd, (HMENU)ID_INPUT_WORD, NULL, NULL);
        HWND et = CreateWindowW(L"EDIT", input_transResult, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 100, 40, 200, 20, hwnd, (HMENU)ID_INPUT_TRANS, NULL, NULL);
        CreateWindowW(L"STATIC", L"Word:", WS_CHILD | WS_VISIBLE, 10, 10, 85, 20, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"Translation:", WS_CHILD | WS_VISIBLE, 10, 40, 85, 20, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 100, 80, 100, 30, hwnd, (HMENU)ID_INPUT_OK, NULL, NULL);
        SendMessage(ew, WM_SETFONT, (WPARAM)hFont, TRUE); SendMessage(et, WM_SETFONT, (WPARAM)hFont, TRUE);
        BOOL v = TRUE; DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &v, sizeof(v));
        break;
    }
    case WM_CTLCOLORSTATIC: {HDC h = (HDC)wParam; SetTextColor(h, colorText); SetBkColor(h, colorBg); return (LRESULT)hbrBg;}
    case WM_CTLCOLOREDIT: {HDC h = (HDC)wParam; SetTextColor(h, colorText); SetBkColor(h, colorCtrl); return (LRESULT)hbrCtrl;}
    case WM_DRAWITEM: {LPDRAWITEMSTRUCT p = (LPDRAWITEMSTRUCT)lParam; if (p->CtlID == ID_INPUT_OK) DrawColoredButton(p, RGB(46, 204, 113)); return TRUE;}
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_INPUT_OK) {
            GetDlgItemTextW(hwnd, ID_INPUT_WORD, input_wordResult, 256);
            GetDlgItemTextW(hwnd, ID_INPUT_TRANS, input_transResult, 256);
            input_success = true; EnableWindow(hMainWnd, TRUE); DestroyWindow(hwnd);
        } break;
    case WM_CLOSE: EnableWindow(hMainWnd, TRUE); DestroyWindow(hwnd); break;
    default: return DefWindowProcW(hwnd, msg, wParam, lParam);
    } return 0;
}
void OpenInputDialog(HWND parent, const wchar_t* iw = L"", const wchar_t* it = L"") {
    WStrCpy(input_wordResult, iw); WStrCpy(input_transResult, it); input_success = false;
    WNDCLASSW wc = { 0 }; wc.lpfnWndProc = InputWndProc; wc.hInstance = GetModuleHandle(NULL);
    wc.hbrBackground = hbrBg; wc.lpszClassName = L"InputDialogClass"; RegisterClassW(&wc);
    HWND h = CreateWindowW(L"InputDialogClass", L"Dictionary Entry", WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME, CW_USEDEFAULT, CW_USEDEFAULT, 340, 160, parent, NULL, wc.hInstance, NULL);
    EnableWindow(parent, FALSE); ShowWindow(h, SW_SHOW);
    MSG msg; while (IsWindow(h) && GetMessage(&msg, NULL, 0, 0)) {TranslateMessage(&msg); DispatchMessage(&msg);}
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        hbrBg = CreateSolidBrush(colorBg); hbrCtrl = CreateSolidBrush(colorCtrl);
        hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        BOOL v = TRUE; DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &v, sizeof(v));
        HMENU hMenu = CreateMenu(), hFile = CreatePopupMenu();
        AppendMenuW(hFile, MF_STRING, ID_FILE_NEW, L"Create A New List");
        AppendMenuW(hFile, MF_STRING, ID_FILE_LOAD, L"Load Existing List"); AppendMenuW(hFile, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hFile, MF_STRING, ID_FILE_SAVE, L"Save List\tCtrl+S"); AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFile, L"File"); SetMenu(hwnd, hMenu);
        hBtnAdd = CreateWindowW(L"BUTTON", L"Add Word", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 10, 10, 100, 30, hwnd, (HMENU)ID_BTN_ADD, NULL, NULL);
        hBtnEdit = CreateWindowW(L"BUTTON", L"Edit Word", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 120, 10, 100, 30, hwnd, (HMENU)ID_BTN_EDIT, NULL, NULL);
        hBtnDel = CreateWindowW(L"BUTTON", L"Delete Word", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 230, 10, 100, 30, hwnd, (HMENU)ID_BTN_DEL, NULL, NULL);
        hSearchEdit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 340, 10, 200, 30, hwnd, (HMENU)ID_SEARCH_EDIT, NULL, NULL);
        SendMessageW(hSearchEdit, EM_SETCUEBANNER, FALSE, (LPARAM)L"Search words...");
        hListView = CreateWindowExW(0, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOCOLUMNHEADER, 10, 50 + HEADER_H, 560, 380 - HEADER_H, hwnd, (HMENU)ID_LISTVIEW, NULL, NULL);
        ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        SetWindowTheme(hListView, L"DarkMode_Explorer", NULL);
        ListView_SetBkColor(hListView, colorCtrl); ListView_SetTextBkColor(hListView, colorCtrl); ListView_SetTextColor(hListView, colorText);
        LVCOLUMNW lvc = { 0 }; lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvc.pszText = (LPWSTR)L"Word"; lvc.iSubItem = 0; ListView_InsertColumn(hListView, 0, &lvc);
        lvc.pszText = (LPWSTR)L"Translation"; lvc.iSubItem = 1; ListView_InsertColumn(hListView, 1, &lvc);
        SendMessage(hListView, WM_SETFONT, (WPARAM)hFont, TRUE); SendMessage(hSearchEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
        WNDCLASSW whc = { 0 }; whc.lpfnWndProc = FakeHeaderProc; whc.hInstance = GetModuleHandle(NULL);
        whc.hbrBackground = (HBRUSH)CreateSolidBrush(colorHeader); whc.lpszClassName = L"FakeHeaderClass"; RegisterClassW(&whc);
        hFakeHeader = CreateWindowW(L"FakeHeaderClass", L"", WS_CHILD | WS_VISIBLE, 10, 50, 560, HEADER_H, hwnd, (HMENU)ID_FAKE_HEADER, NULL, NULL);
        break;
    }
    case WM_CTLCOLOREDIT: {if ((HWND)lParam == hSearchEdit) {HDC h = (HDC)wParam; SetTextColor(h, colorText); SetBkColor(h, colorCtrl); return (LRESULT)hbrCtrl;} break;}
    case WM_SIZE: {
        int W = LOWORD(lParam), H = HIWORD(lParam);
        MoveWindow(hSearchEdit, 340, 10, W - 350, 30, TRUE); MoveWindow(hFakeHeader, 10, 50, W - 20, HEADER_H, TRUE);
        MoveWindow(hListView, 10, 50 + HEADER_H, W - 20, H - 60 - HEADER_H, TRUE);
        RECT rc; GetClientRect(hListView, &rc); int c0 = rc.right / 3;
        ListView_SetColumnWidth(hListView, 0, c0); ListView_SetColumnWidth(hListView, 1, LVSCW_AUTOSIZE_USEHEADER);
        InvalidateRect(hFakeHeader, NULL, TRUE); break;
    }
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT p = (LPDRAWITEMSTRUCT)lParam;
        if (p->CtlID == ID_BTN_ADD) DrawColoredButton(p, RGB(46, 204, 113));
        else if (p->CtlID == ID_BTN_EDIT) DrawColoredButton(p, RGB(52, 152, 219));
        else if (p->CtlID == ID_BTN_DEL) DrawColoredButton(p, RGB(231, 76, 60));
        return TRUE;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_SEARCH_EDIT && HIWORD(wParam) == EN_CHANGE) {
            GetWindowTextW(hSearchEdit, currentSearchText, 256); ApplyFilter(); break;
        }
        if (id == ID_FILE_NEW) {
            if (!CheckUnsavedChanges(hwnd)) break;
            currentFilePath[0] = 0; DictClear(); isDirty = false; ApplyFilter(); UpdateTitle();
        }
        else if (id == ID_FILE_LOAD) {
            if (!CheckUnsavedChanges(hwnd)) break;
            wchar_t fn[MAX_PATH] = L""; OPENFILENAMEW ofn = { 0 };
            ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = L"JSON Files\0*.json\0All Files\0*.*\0"; ofn.lpstrFile = fn;
            ofn.nMaxFile = MAX_PATH; ofn.Flags = OFN_FILEMUSTEXIST;
            if (GetOpenFileNameW(&ofn)) {WStrCpy(currentFilePath, fn); LoadDictionary(currentFilePath);}
        }
        else if (id == ID_FILE_SAVE) {if (currentFilePath[0] == 0) PromptSave(hwnd); else SaveDictionary();}
        else if (id == ID_BTN_ADD) {
            OpenInputDialog(hwnd);
            if (input_success && input_wordResult[0] != 0) {
                DictPush(input_wordResult, input_transResult); isDirty = true; UpdateTitle();
                if (MatchesFilter(input_wordResult, input_transResult)) {
                    int ri = dictCount - 1; filteredIndices[filteredCount++] = ri;
                    int ui = ListView_GetItemCount(hListView);
                    LVITEMW lvi = { 0 }; lvi.mask = LVIF_TEXT; lvi.iItem = ui; lvi.pszText = dictionary[ri].word;
                    ListView_InsertItem(hListView, &lvi); ListView_SetItemText(hListView, ui, 1, dictionary[ri].translation);
                    ListView_EnsureVisible(hListView, ui, FALSE);
                }
            }
        }
        else if (id == ID_BTN_EDIT) {
            int sel = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
            if (sel != -1) {
                int ri = filteredIndices[sel];
                OpenInputDialog(hwnd, dictionary[ri].word, dictionary[ri].translation);
                if (input_success) {
                    MemFree(dictionary[ri].word); MemFree(dictionary[ri].translation);
                    dictionary[ri].word = WStrDup(input_wordResult); dictionary[ri].translation = WStrDup(input_transResult);
                    isDirty = true; UpdateTitle();
                    if (MatchesFilter(input_wordResult, input_transResult)) {
                        ListView_SetItemText(hListView, sel, 0, dictionary[ri].word);
                        ListView_SetItemText(hListView, sel, 1, dictionary[ri].translation);
                    } else ApplyFilter();
                }
            }
        }
        else if (id == ID_BTN_DEL) {
            int sel = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
            if (sel != -1) {DictErase(filteredIndices[sel]); isDirty = true; ApplyFilter(); UpdateTitle();}
        }
        break;
    }
    case WM_CLOSE:   if (CheckUnsavedChanges(hwnd)) DestroyWindow(hwnd); break;
    case WM_DESTROY: DeleteObject(hbrBg); DeleteObject(hbrCtrl); DeleteObject(hFont); PostQuitMessage(0); break;
    default: return DefWindowProcW(hwnd, msg, wParam, lParam);
    } return 0;
}
extern "C" void WinMainCRTStartup() {
    INITCOMMONCONTROLSEX icex = { 0 };
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);
    HINSTANCE hInst = GetModuleHandle(NULL);
    ACCEL accel[1]; accel[0].fVirt = FCONTROL | FVIRTKEY; accel[0].key = 'S'; accel[0].cmd = ID_FILE_SAVE;
    HACCEL hAccel = CreateAcceleratorTableW(accel, 1);
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc; wc.hInstance = hInst;
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(colorBg);
    wc.lpszClassName = L"DictionaryAppClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);
    hMainWnd = CreateWindowExW(0, L"DictionaryAppClass", L"Fast Dictionary (Dark)", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 700, 500, NULL, NULL, hInst, NULL);
    ShowWindow(hMainWnd, SW_SHOW); UpdateWindow(hMainWnd);
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (!TranslateAcceleratorW(hMainWnd, hAccel, &msg)) {TranslateMessage(&msg); DispatchMessageW(&msg);}
    }
    DestroyAcceleratorTable(hAccel); ExitProcess((UINT)msg.wParam);
}