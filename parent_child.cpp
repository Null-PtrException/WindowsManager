#include "WindowsManager.h"

// ---------- 选择器控件 ID ----------
#define IDC_PICKER_LIST    6001
#define IDC_PICKER_OK      6002
#define IDC_PICKER_CANCEL  6003

// ---------- 选择器状态 ----------
static HWND s_pickerOwner = NULL;
static HWND s_pickerList = NULL;
static HWND s_pickerResult = NULL;
static std::vector<HWND> s_pickerWindows;
static std::vector<std::wstring> s_pickerTitles;

// ---------- 枚举可见窗口 ----------
static BOOL CALLBACK PickerEnumProc(HWND hWnd, LPARAM lParam) {
    if (!IsWindowVisible(hWnd)) return TRUE;
    if (hWnd == (HWND)lParam) return TRUE; // 跳过选择器自身
    wchar_t title[256] = {0};
    GetWindowTextW(hWnd, title, 256);
    if (wcslen(title) > 0) {
        s_pickerWindows.push_back(hWnd);
        wchar_t cls[128] = {0};
        GetClassNameW(hWnd, cls, 128);
        std::wstring disp = std::wstring(title) + L" [" + cls + L"]";
        s_pickerTitles.push_back(disp);
    }
    return TRUE;
}

// ---------- 选择器窗口过程 ----------
LRESULT CALLBACK PickerProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            s_pickerList = CreateWindowW(L"LISTBOX", NULL,
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
                10, 10, 460, 300, hWnd, (HMENU)IDC_PICKER_LIST, NULL, NULL);

            // 填充窗口列表
            s_pickerWindows.clear();
            s_pickerTitles.clear();
            EnumWindows(PickerEnumProc, (LPARAM)hWnd);
            for (size_t i = 0; i < s_pickerTitles.size(); i++) {
                SendMessageW(s_pickerList, LB_ADDSTRING, 0, (LPARAM)s_pickerTitles[i].c_str());
            }

            // 确定 / 取消 按钮
            CreateWindowW(L"BUTTON", L"确定", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                280, 320, 90, 30, hWnd, (HMENU)IDC_PICKER_OK, NULL, NULL);
            CreateWindowW(L"BUTTON", L"取消", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                380, 320, 90, 30, hWnd, (HMENU)IDC_PICKER_CANCEL, NULL, NULL);
            break;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);

            if (id == IDC_PICKER_OK) {
                int idx = (int)SendMessageW(s_pickerList, LB_GETCURSEL, 0, 0);
                if (idx != LB_ERR && idx >= 0 && idx < (int)s_pickerWindows.size()) {
                    s_pickerResult = s_pickerWindows[idx];
                    DestroyWindow(hWnd);
                } else {
                    MessageBoxW(hWnd, L"请先选择一个窗口", L"提示", MB_OK);
                }
            } else if (id == IDC_PICKER_CANCEL) {
                s_pickerResult = NULL;
                DestroyWindow(hWnd);
            } else if (id == IDC_PICKER_LIST && code == LBN_DBLCLK) {
                int idx = (int)SendMessageW(s_pickerList, LB_GETCURSEL, 0, 0);
                if (idx != LB_ERR && idx >= 0 && idx < (int)s_pickerWindows.size()) {
                    s_pickerResult = s_pickerWindows[idx];
                    DestroyWindow(hWnd);
                }
            }
            break;
        }

        case WM_CLOSE:
            s_pickerResult = NULL;
            DestroyWindow(hWnd);
            break;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ---------- 模态打开选择器 ----------
HWND PickWindowModal(HWND hParent) {
    static bool registered = false;
    HINSTANCE hInst = GetModuleHandleW(NULL);

    if (!registered) {
        WNDCLASSEXW wc = {sizeof(WNDCLASSEXW), 0, PickerProc, 0, 0, hInst,
                          NULL,
                          LoadCursor(NULL, IDC_ARROW),
                          (HBRUSH)(COLOR_WINDOW + 1),
                          NULL,
                          L"WindowPickerClass",
                          NULL};
        RegisterClassExW(&wc);
        registered = true;
    }

    s_pickerOwner = hParent;
    s_pickerResult = NULL;

    RECT rcOwner;
    GetWindowRect(hParent, &rcOwner);
    int w = 500, h = 380;
    int x = rcOwner.left + (rcOwner.right - rcOwner.left - w) / 2;
    int y = rcOwner.top + (rcOwner.bottom - rcOwner.top - h) / 2;

    HWND hPicker = CreateWindowExW(WS_EX_DLGMODALFRAME, L"WindowPickerClass",
        L"选择一个窗口", WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, w, h, hParent, NULL, hInst, NULL);

    if (!hPicker) return NULL;

    EnableWindow(hParent, FALSE);
    ShowWindow(hPicker, SW_SHOW);
    UpdateWindow(hPicker);

    MSG msg;
    while (IsWindow(hPicker) && GetMessageW(&msg, NULL, 0, 0)) {
        if (!IsDialogMessageW(hPicker, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    EnableWindow(hParent, TRUE);
    SetActiveWindow(hParent);
    return s_pickerResult;
}

// ---------- 更新按钮显示文本 ----------
static std::wstring EscapeAmp(const std::wstring& s) {
    std::wstring r;
    r.reserve(s.size());
    for (wchar_t c : s) {
        if (c == L'&') r += L"&&";
        else r += c;
    }
    return r;
}

void UpdatePickButtonText(HWND hBtn, HWND hWnd, const wchar_t* placeholder) {
    if (hWnd && IsWindow(hWnd)) {
        wchar_t title[256] = {0};
        GetWindowTextW(hWnd, title, 256);
        std::wstring disp = title;
        if (disp.empty()) disp = L"[无标题]";
        if (disp.length() > 25) disp = disp.substr(0, 22) + L"...";
        SetWindowTextW(hBtn, EscapeAmp(disp).c_str());
    } else {
        SetWindowTextW(hBtn, placeholder);
    }
}

// ---------- 执行：将 A 设为 B 的子窗口 ----------
void ApplyParentChild() {
    if (!g_pickChildWnd || !IsWindow(g_pickChildWnd)) {
        MessageBoxW(g_hMainWnd, L"请先选择窗口A（要作为子窗口的窗口）", L"提示", MB_OK);
        return;
    }
    if (!g_pickParentWnd || !IsWindow(g_pickParentWnd)) {
        MessageBoxW(g_hMainWnd, L"请先选择窗口B（要作为父窗口的窗口）", L"提示", MB_OK);
        return;
    }
    if (g_pickChildWnd == g_pickParentWnd) {
        MessageBoxW(g_hMainWnd, L"窗口A和窗口B不能是同一个窗口", L"提示", MB_OK);
        return;
    }

    DWORD pidA = 0, pidB = 0;
    GetWindowThreadProcessId(g_pickChildWnd, &pidA);
    GetWindowThreadProcessId(g_pickParentWnd, &pidB);
    if (pidA != pidB) {
        if (MessageBoxW(g_hMainWnd,
            L"窗口A和窗口B属于不同的进程。\n"
            L"Windows 的 SetParent 只能在同一进程内生效，\n"
            L"跨进程调用通常会失败或出现异常行为。\n\n"
            L"是否仍要继续尝试？",
            L"警告", MB_YESNO | MB_ICONWARNING) != IDYES) {
            return;
        }
    }

    SetLastError(0);
    HWND hOldParent = SetParent(g_pickChildWnd, g_pickParentWnd);
    if (hOldParent == NULL && GetLastError() != 0) {
        wchar_t msg[128];
        swprintf(msg, 128, L"设置父子关系失败，错误代码: %lu", GetLastError());
        MessageBoxW(g_hMainWnd, msg, L"错误", MB_ICONERROR);
    } else {
        MessageBoxW(g_hMainWnd, L"已将窗口A设为窗口B的子窗口。", L"成功", MB_OK);
    }
}

// ---------- 取消继承：把 A 的父窗口还原为桌面 ----------
void UnparentChild() {
    if (!g_pickChildWnd || !IsWindow(g_pickChildWnd)) {
        MessageBoxW(g_hMainWnd, L"请先选择窗口A（要取消继承的窗口）", L"提示", MB_OK);
        return;
    }

    HWND hCurParent = (HWND)GetWindowLongPtrW(g_pickChildWnd, GWLP_HWNDPARENT);
    if (hCurParent == NULL) {
        MessageBoxW(g_hMainWnd, L"窗口A当前没有父窗口，无需取消。", L"提示", MB_OK);
        return;
    }

    if (MessageBoxW(g_hMainWnd,
        L"确定要取消窗口A的父子关系吗？\n"
        L"窗口A的父窗口将被设为桌面（还原为顶层窗口）。",
        L"确认", MB_YESNO | MB_ICONQUESTION) != IDYES) {
        return;
    }

    SetLastError(0);
    HWND hOldParent = SetParent(g_pickChildWnd, NULL);
    if (hOldParent == NULL && GetLastError() != 0) {
        wchar_t msg[128];
        swprintf(msg, 128, L"取消父子关系失败，错误代码: %lu", GetLastError());
        MessageBoxW(g_hMainWnd, msg, L"错误", MB_ICONERROR);
    } else {
        // 还原为顶层窗口样式
        LONG_PTR style = GetWindowLongPtr(g_pickChildWnd, GWL_STYLE);
        style |= WS_OVERLAPPEDWINDOW;
        style &= ~WS_CHILD;
        SetWindowLongPtr(g_pickChildWnd, GWL_STYLE, style);
        SetWindowPos(g_pickChildWnd, HWND_TOP, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        MessageBoxW(g_hMainWnd, L"已取消窗口A的父子关系。", L"成功", MB_OK);
    }
}