#include "WindowsManager.h"

// ---------- 窗口属性操作 ----------
void SetTransparency(HWND hWnd, BYTE alpha) {
    if (!hWnd || !IsWindow(hWnd)) return;
    LONG ex = GetWindowLong(hWnd, GWL_EXSTYLE);
    SetWindowLong(hWnd, GWL_EXSTYLE, ex | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hWnd, 0, alpha, LWA_ALPHA);
}

void SetTopmost(HWND hWnd, bool top) {
    if (!hWnd || !IsWindow(hWnd)) return;
    SetWindowPos(hWnd, top ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void SetNoBorder(HWND hWnd, bool noBorder) {
    if (!hWnd || !IsWindow(hWnd)) return;
    LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
    if (noBorder) {
        SetPropW(hWnd, L"OrigStyle", (HANDLE)style);
        style = (style & ~(WS_CAPTION | WS_SYSMENU | WS_BORDER | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)) | WS_POPUP;
        SetWindowLongPtr(hWnd, GWL_STYLE, style);
    } else {
        LONG_PTR origStyle = (LONG_PTR)GetPropW(hWnd, L"OrigStyle");
        if (origStyle) {
            SetWindowLongPtr(hWnd, GWL_STYLE, origStyle);
            RemovePropW(hWnd, L"OrigStyle");
        } else {
            style |= WS_CAPTION | WS_SYSMENU | WS_BORDER | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
            style &= ~WS_POPUP;
            SetWindowLongPtr(hWnd, GWL_STYLE, style);
        }
    }
    SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

// ---------- 全屏 ----------
void SetFullscreen(HWND hWnd, bool full) {
    if (!hWnd || !IsWindow(hWnd)) return;

    if (full) {
        // 记录原始位置和样式，以便恢复
        RECT rc;
        GetWindowRect(hWnd, &rc);
        SetPropW(hWnd, L"FullOrigRect", (HANDLE)malloc(sizeof(RECT)));
        RECT* p = (RECT*)GetPropW(hWnd, L"FullOrigRect");
        if (p) *p = rc;

        LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
        SetPropW(hWnd, L"FullOrigStyle", (HANDLE)style);

        // 去掉标题栏、边框，变成纯弹出窗口
        style &= ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
        style |= WS_POPUP;
        SetWindowLongPtr(hWnd, GWL_STYLE, style);

        int sw = GetSystemMetrics(SM_CXSCREEN);
        int sh = GetSystemMetrics(SM_CYSCREEN);
        
        // 【关键修正】使用 HWND_TOP 而不是 HWND_TOPMOST，绝不让目标窗口强制置顶
        SetWindowPos(hWnd, HWND_TOP, 0, 0, sw, sh,
                     SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    } else {
        RECT* p = (RECT*)GetPropW(hWnd, L"FullOrigRect");
        if (p) {
            SetWindowPos(hWnd, HWND_TOP, p->left, p->top,
                         p->right - p->left, p->bottom - p->top,
                         SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOZORDER);
            free(p);
            RemovePropW(hWnd, L"FullOrigRect");
        }
        LONG_PTR origStyle = (LONG_PTR)GetPropW(hWnd, L"FullOrigStyle");
        if (origStyle) {
            SetWindowLongPtr(hWnd, GWL_STYLE, origStyle);
            RemovePropW(hWnd, L"FullOrigStyle");
        }
        SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

void MoveWindowTo(HWND hWnd, int x, int y) {
    if (!hWnd || !IsWindow(hWnd)) return;
    SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void ResizeWindowTo(HWND hWnd, int w, int h) {
    if (!hWnd || !IsWindow(hWnd)) return;
    SetWindowPos(hWnd, NULL, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER);
}

HWND ParseHandle(const wchar_t* str) {
    unsigned long long h = 0;
    std::wstring s = str;
    if (s.find(L"0x") == 0 || s.find(L"0X") == 0) s = s.substr(2);
    std::wstringstream ss;
    ss << std::hex << s;
    ss >> h;
    HWND hWnd = (HWND)h;
    return IsWindow(hWnd) ? hWnd : NULL;
}