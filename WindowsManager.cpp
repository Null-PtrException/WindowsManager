// 定义 Unicode
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <sstream>
#include <tlhelp32.h>
#include <shlwapi.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

// ===== 进程信息结构 =====
enum ProcessType {
    PT_APP = 0,
    PT_SYSTEM = 1,
    PT_BACKGROUND = 2
};

struct ProcessInfo {
    DWORD pid;
    std::wstring name;
    ProcessType type;
};
// =========================

// 全局变量
HWND g_hMainWnd = NULL;
HWND g_hListBox = NULL;
std::vector<HWND> g_windows;
std::vector<std::wstring> g_titles;
HWND g_targetWnd = NULL;
DWORD g_targetPid = 0;
bool g_bProcessMode = false;

// 控件
HWND g_hRefreshBtn = NULL;
HWND g_hInfoText = NULL;
HWND g_hHandleEdit = NULL;
HWND g_hSetHandleBtn = NULL;
HWND g_hSlider = NULL;
HWND g_hAlphaValue = NULL;
HWND g_hTopmostCheck = NULL;
HWND g_hApplyAttrBtn = NULL;
HWND g_hXEdit = NULL, g_hYEdit = NULL;
HWND g_hWEdit = NULL, g_hHEdit = NULL;
HWND g_hGetPosBtn = NULL;
HWND g_hMoveBtn = NULL;
HWND g_hResizeBtn = NULL;
HWND g_hProcInfoStatic = NULL;
HWND g_hEndProcessBtn = NULL;
HWND g_hElevatedKillBtn = NULL;
HWND g_hLaunchCmdEdit = NULL;
HWND g_hLaunchBtn = NULL;
HWND g_hUserCombo = NULL;
HWND g_hNoBorderCheck = NULL;
HWND g_hShowProcessCheck = NULL;

// 需要禁用的控件列表
std::vector<HWND> g_disableControls;

DWORD g_currentPid = 0;
wchar_t g_currentProcName[256] = {0};

// 函数声明
std::wstring GetWindowDisplay(HWND hWnd);
BOOL CALLBACK EnumProc(HWND hWnd, LPARAM lParam);
void UpdateDisplay();
void RefreshList();
void SetTransparency(HWND hWnd, BYTE alpha);
void SetTopmost(HWND hWnd, bool top);
void SetNoBorder(HWND hWnd, bool noBorder);
void MoveWindowTo(HWND hWnd, int x, int y);
void ResizeWindowTo(HWND hWnd, int w, int h);
HWND ParseHandle(const wchar_t* str);
std::wstring GetProcessNameByPid(DWORD pid);
void KillProcessNormal();
void KillProcessElevatedWithNsudo();
void LaunchProcessWithNsudo();
std::wstring GetSelectedUserOption();
bool TerminateProcessByPid(DWORD pid);
std::vector<ProcessInfo> GetProcessList();
bool ProcessHasVisibleWindow(DWORD pid);
bool IsSystemProcess(const std::wstring& name);

// ---------- 辅助函数 ----------
bool TerminateProcessByPid(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProcess) return false;
    bool result = TerminateProcess(hProcess, 0) != 0;
    CloseHandle(hProcess);
    return result;
}

std::wstring GetProcessNameByPid(DWORD pid) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return L"未知";
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (pe.th32ProcessID == pid) {
                CloseHandle(hSnapshot);
                return pe.szExeFile;
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
    return L"未知";
}

std::wstring GetSelectedUserOption() {
    int idx = (int)SendMessageW(g_hUserCombo, CB_GETCURSEL, 0, 0);
    if (idx == CB_ERR) return L"S";
    switch (idx) {
        case 0: return L"T";
        case 1: return L"S";
        case 2: return L"E";
        default: return L"S";
    }
}

// ---------- 结束进程 ----------
void KillProcessNormal() {
    DWORD pid = 0;
    if (g_bProcessMode) {
        pid = g_targetPid;
    } else {
        if (!g_targetWnd || !IsWindow(g_targetWnd)) {
            MessageBoxW(g_hMainWnd, L"没有选中有效窗口", L"提示", MB_OK);
            return;
        }
        GetWindowThreadProcessId(g_targetWnd, &pid);
    }

    if (pid == 0) {
        MessageBoxW(g_hMainWnd, L"无法获取进程ID", L"错误", MB_ICONERROR);
        return;
    }

    if (MessageBoxW(g_hMainWnd, L"确定要结束该进程吗？\n进程将被强制终止，数据可能丢失。", L"确认", MB_YESNO | MB_ICONWARNING) != IDYES)
        return;

    if (TerminateProcessByPid(pid)) {
        MessageBoxW(g_hMainWnd, L"进程已终止", L"成功", MB_OK);
        RefreshList();
    } else {
        MessageBoxW(g_hMainWnd, L"终止失败，权限不足或进程已退出。", L"错误", MB_ICONERROR);
    }
}

void KillProcessElevatedWithNsudo() {
    DWORD pid = 0;
    if (g_bProcessMode) {
        pid = g_targetPid;
    } else {
        if (!g_targetWnd || !IsWindow(g_targetWnd)) {
            MessageBoxW(g_hMainWnd, L"没有选中有效窗口", L"提示", MB_OK);
            return;
        }
        GetWindowThreadProcessId(g_targetWnd, &pid);
    }

    if (pid == 0) {
        MessageBoxW(g_hMainWnd, L"无法获取进程ID", L"错误", MB_ICONERROR);
        return;
    }

    if (MessageBoxW(g_hMainWnd, L"使用 NSudo 以选中的权限强制结束进程？\n将调用 taskkill /f", L"确认", MB_YESNO | MB_ICONWARNING) != IDYES)
        return;

    wchar_t exePath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);
    wchar_t nsudoPath[MAX_PATH];
    swprintf(nsudoPath, MAX_PATH, L"%ls\\NSudoL.exe", exePath);
    if (GetFileAttributesW(nsudoPath) == INVALID_FILE_ATTRIBUTES) {
        swprintf(nsudoPath, MAX_PATH, L"%ls\\NSudo.exe", exePath);
        if (GetFileAttributesW(nsudoPath) == INVALID_FILE_ATTRIBUTES) {
            MessageBoxW(g_hMainWnd, L"未找到 NSudoL.exe 或 NSudo.exe\n请将无窗口化 NSudo 放在本程序目录下。", L"错误", MB_ICONERROR);
            return;
        }
    }

    std::wstring userOpt = GetSelectedUserOption();
    wchar_t cmdLine[1024];
    swprintf(cmdLine, 1024, L"\"%ls\" -U:%ls -P:E -Wait taskkill /f /pid %d", nsudoPath, userOpt.c_str(), pid);

    STARTUPINFOW si = {sizeof(STARTUPINFOW)};
    PROCESS_INFORMATION pi = {0};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    BOOL success = CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    if (success) {
        WaitForSingleObject(pi.hProcess, 10000);
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        if (exitCode == 0) {
            MessageBoxW(g_hMainWnd, L"进程已成功终止", L"NSudo执行", MB_OK);
        } else {
            MessageBoxW(g_hMainWnd, L"终止失败，进程可能受保护或已退出。", L"NSudo执行", MB_ICONERROR);
        }
        RefreshList();
    } else {
        MessageBoxW(g_hMainWnd, L"启动 NSudo 失败，请检查文件是否存在或是否有足够权限。", L"错误", MB_ICONERROR);
    }
}

void LaunchProcessWithNsudo() {
    wchar_t cmdLine[1024];
    GetWindowTextW(g_hLaunchCmdEdit, cmdLine, 1024);
    if (wcslen(cmdLine) == 0) {
        MessageBoxW(g_hMainWnd, L"请输入要启动的命令行或程序路径。", L"提示", MB_OK);
        return;
    }

    wchar_t exePath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);
    wchar_t nsudoPath[MAX_PATH];
    swprintf(nsudoPath, MAX_PATH, L"%ls\\NSudoL.exe", exePath);
    if (GetFileAttributesW(nsudoPath) == INVALID_FILE_ATTRIBUTES) {
        swprintf(nsudoPath, MAX_PATH, L"%ls\\NSudo.exe", exePath);
        if (GetFileAttributesW(nsudoPath) == INVALID_FILE_ATTRIBUTES) {
            MessageBoxW(g_hMainWnd, L"未找到 NSudoL.exe 或 NSudo.exe\n请将无窗口化 NSudo 放在本程序目录下。", L"错误", MB_ICONERROR);
            return;
        }
    }

    std::wstring userOpt = GetSelectedUserOption();
    wchar_t fullCmd[2048];
    swprintf(fullCmd, 2048, L"\"%ls\" -U:%ls -P:E %ls", nsudoPath, userOpt.c_str(), cmdLine);

    STARTUPINFOW si = {sizeof(STARTUPINFOW)};
    PROCESS_INFORMATION pi = {0};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL;

    BOOL success = CreateProcessW(NULL, fullCmd, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
    if (success) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        MessageBoxW(g_hMainWnd, L"已启动进程。", L"NSudo执行", MB_OK);
    } else {
        wchar_t errMsg[256];
        swprintf(errMsg, 256, L"启动失败，错误代码: %lu", GetLastError());
        MessageBoxW(g_hMainWnd, errMsg, L"错误", MB_ICONERROR);
    }
}

// ---------- 界面功能 ----------
std::wstring GetWindowDisplay(HWND hWnd) {
    wchar_t title[256] = {0}, cls[256] = {0};
    GetWindowTextW(hWnd, title, 256);
    GetClassNameW(hWnd, cls, 256);
    if (wcslen(title) > 0)
        return std::wstring(title) + L" [" + cls + L"]";
    else
        return std::wstring(L"[无标题] ") + cls;
}

BOOL CALLBACK EnumProc(HWND hWnd, LPARAM lParam) {
    if (!IsWindowVisible(hWnd)) return TRUE;
    wchar_t title[256];
    GetWindowTextW(hWnd, title, 256);
    if (wcslen(title) > 0) {
        g_windows.push_back(hWnd);
        g_titles.push_back(GetWindowDisplay(hWnd));
    }
    return TRUE;
}

bool IsSystemProcess(const std::wstring& name) {
    static const wchar_t* sysNames[] = {
        L"svchost.exe", L"services.exe", L"lsass.exe", L"winlogon.exe",
        L"csrss.exe", L"smss.exe", L"wininit.exe", L"spoolsv.exe",
        L"SearchIndexer.exe", L"taskhostw.exe", L"dwm.exe"
    };
    for (auto& s : sysNames) {
        if (_wcsicmp(name.c_str(), s) == 0)
            return true;
    }
    return false;
}

void UpdateDisplay() {
    if (g_bProcessMode) {
        // 禁用所有非必需控件
        for (HWND h : g_disableControls) {
            EnableWindow(h, FALSE);
        }

        // 确保NSudo相关控件保持启用
        EnableWindow(g_hUserCombo, TRUE);
        EnableWindow(g_hElevatedKillBtn, TRUE);

        if (g_targetPid != 0) {
            std::wstring procName = GetProcessNameByPid(g_targetPid);
            wchar_t buf[256];
            swprintf(buf, 256, L"当前选中进程: %ls  (PID: %d)", procName.c_str(), g_targetPid);
            SetWindowTextW(g_hInfoText, buf);
            SetWindowTextW(g_hProcInfoStatic, L"进程模式 - 可使用 NSudo 结束进程");
        } else {
            SetWindowTextW(g_hInfoText, L"请从列表中选择一个进程");
            SetWindowTextW(g_hProcInfoStatic, L"进程模式 - 未选中");
        }
        // 清空无关控件的数值（虽然变灰，但保持干净）
        SetWindowTextW(g_hHandleEdit, L"");
        SetWindowTextW(g_hXEdit, L"");
        SetWindowTextW(g_hYEdit, L"");
        SetWindowTextW(g_hWEdit, L"");
        SetWindowTextW(g_hHEdit, L"");
        SendMessageW(g_hTopmostCheck, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageW(g_hNoBorderCheck, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageW(g_hSlider, TBM_SETPOS, TRUE, 255);
        SetWindowTextW(g_hAlphaValue, L"100%");
        return;
    }

    // 窗口模式：启用所有控件
    for (HWND h : g_disableControls) {
        EnableWindow(h, TRUE);
    }

    if (!g_targetWnd || !IsWindow(g_targetWnd)) {
        SetWindowTextW(g_hInfoText, L"未选中窗口，请从列表中点击选择");
        SetWindowTextW(g_hHandleEdit, L"");
        SetWindowTextW(g_hXEdit, L"");
        SetWindowTextW(g_hYEdit, L"");
        SetWindowTextW(g_hWEdit, L"");
        SetWindowTextW(g_hHEdit, L"");
        SendMessageW(g_hTopmostCheck, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageW(g_hSlider, TBM_SETPOS, TRUE, 255);
        SetWindowTextW(g_hAlphaValue, L"100%");
        SetWindowTextW(g_hProcInfoStatic, L"进程: 无");
        SendMessageW(g_hNoBorderCheck, BM_SETCHECK, BST_UNCHECKED, 0);
        g_currentPid = 0;
        return;
    }

    std::wstring info = L"当前选中: " + GetWindowDisplay(g_targetWnd) + L"\n句柄: 0x";
    wchar_t handleBuf[32];
    swprintf(handleBuf, 32, L"%p", g_targetWnd);
    info += handleBuf;
    SetWindowTextW(g_hInfoText, info.c_str());

    wchar_t buf[64];
    swprintf(buf, 64, L"%p", g_targetWnd);
    SetWindowTextW(g_hHandleEdit, buf);

    RECT rect;
    GetWindowRect(g_targetWnd, &rect);
    swprintf(buf, 64, L"%d", rect.left);   SetWindowTextW(g_hXEdit, buf);
    swprintf(buf, 64, L"%d", rect.top);    SetWindowTextW(g_hYEdit, buf);
    swprintf(buf, 64, L"%d", rect.right - rect.left); SetWindowTextW(g_hWEdit, buf);
    swprintf(buf, 64, L"%d", rect.bottom - rect.top); SetWindowTextW(g_hHEdit, buf);

    LONG exStyle = GetWindowLong(g_targetWnd, GWL_EXSTYLE);
    SendMessageW(g_hTopmostCheck, BM_SETCHECK, (exStyle & WS_EX_TOPMOST) ? BST_CHECKED : BST_UNCHECKED, 0);

    BYTE alpha = 255;
    if (GetLayeredWindowAttributes(g_targetWnd, NULL, &alpha, NULL)) {
        SendMessageW(g_hSlider, TBM_SETPOS, TRUE, alpha);
        swprintf(buf, 64, L"%d%%", alpha * 100 / 255);
        SetWindowTextW(g_hAlphaValue, buf);
    } else {
        SendMessageW(g_hSlider, TBM_SETPOS, TRUE, 255);
        SetWindowTextW(g_hAlphaValue, L"100%");
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(g_targetWnd, &pid);
    g_currentPid = pid;
    if (pid != 0) {
        std::wstring procName = GetProcessNameByPid(pid);
        wcscpy(g_currentProcName, procName.c_str());
        swprintf(buf, 64, L"进程: %ls (PID: %d)", procName.c_str(), pid);
    } else {
        swprintf(buf, 64, L"进程: 无法获取");
        g_currentProcName[0] = 0;
    }
    SetWindowTextW(g_hProcInfoStatic, buf);

    LONG_PTR style = GetWindowLongPtr(g_targetWnd, GWL_STYLE);
    BOOL noBorder = (style & WS_CAPTION) == 0;
    SendMessageW(g_hNoBorderCheck, BM_SETCHECK, noBorder ? BST_CHECKED : BST_UNCHECKED, 0);
}

void RefreshList() {
    SendMessageW(g_hListBox, LB_RESETCONTENT, 0, 0);
    g_windows.clear();
    g_titles.clear();

    BOOL showProcess = SendMessageW(g_hShowProcessCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
    g_bProcessMode = (showProcess == TRUE);

    if (showProcess) {
        auto processes = GetProcessList();
        std::vector<ProcessInfo> apps, bg, sys;
        for (const auto& p : processes) {
            if (p.type == PT_APP) apps.push_back(p);
            else if (p.type == PT_SYSTEM) sys.push_back(p);
            else bg.push_back(p);
        }

        SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)L"--- 应用程序（可见窗口） ---");
        for (const auto& p : apps) {
            wchar_t buf[256];
            swprintf(buf, 256, L"%ls  (PID: %d)", p.name.c_str(), p.pid);
            SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)buf);
        }

        SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)L"--- 后台进程（无窗口） ---");
        for (const auto& p : bg) {
            wchar_t buf[256];
            swprintf(buf, 256, L"%ls  (PID: %d)", p.name.c_str(), p.pid);
            SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)buf);
        }

        SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)L"--- 系统进程（系统组件） ---");
        for (const auto& p : sys) {
            wchar_t buf[256];
            swprintf(buf, 256, L"%ls  (PID: %d)", p.name.c_str(), p.pid);
            SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)buf);
        }

        g_targetWnd = NULL;
        g_targetPid = 0;
        UpdateDisplay();
    } else {
        EnumWindows(EnumProc, 0);
        for (size_t i = 0; i < g_titles.size(); i++) {
            SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)g_titles[i].c_str());
        }
        g_targetPid = 0;
        if (!g_windows.empty()) {
            SendMessageW(g_hListBox, LB_SETCURSEL, 0, 0);
            g_targetWnd = g_windows[0];
        } else {
            g_targetWnd = NULL;
        }
        UpdateDisplay();
    }
}

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

// ========== 进程列表相关函数 ==========
BOOL CALLBACK EnumWindowsForPid(HWND hWnd, LPARAM lParam) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hWnd, &pid);
    if (pid == (DWORD)lParam && IsWindowVisible(hWnd)) {
        return FALSE;
    }
    return TRUE;
}

bool ProcessHasVisibleWindow(DWORD pid) {
    return !EnumWindows(EnumWindowsForPid, (LPARAM)pid);
}

std::vector<ProcessInfo> GetProcessList() {
    std::vector<ProcessInfo> result;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return result;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            DWORD pid = pe.th32ProcessID;
            if (pid == 0 || pid == 4) continue;
            ProcessInfo info;
            info.pid = pid;
            info.name = pe.szExeFile;
            bool hasWnd = ProcessHasVisibleWindow(pid);
            if (hasWnd) {
                info.type = PT_APP;
            } else {
                if (IsSystemProcess(info.name)) {
                    info.type = PT_SYSTEM;
                } else {
                    info.type = PT_BACKGROUND;
                }
            }
            result.push_back(info);
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
    return result;
}
// ====================================

// 窗口过程
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // 窗口列表
            CreateWindowW(L"STATIC", L"所有可见窗口:", WS_CHILD | WS_VISIBLE, 10, 10, 150, 20, hWnd, NULL, NULL, NULL);
            g_hListBox = CreateWindowW(L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
                                       10, 30, 500, 180, hWnd, (HMENU)101, NULL, NULL);

            g_hRefreshBtn = CreateWindowW(L"BUTTON", L"刷新列表", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                          520, 30, 100, 30, hWnd, (HMENU)102, NULL, NULL);

            g_hShowProcessCheck = CreateWindowW(L"BUTTON", L"显示进程列表", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                                520, 65, 120, 25, hWnd, (HMENU)104, NULL, NULL);
            SendMessageW(g_hShowProcessCheck, BM_SETCHECK, BST_UNCHECKED, 0);

            // 信息区
            g_hInfoText = CreateWindowW(L"STATIC", L"未选中窗口", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                        10, 220, 430, 50, hWnd, NULL, NULL, NULL);

            // ---- 以下控件在进程模式下被禁用 ----
            // 窗口句柄
            CreateWindowW(L"STATIC", L"窗口句柄:", WS_CHILD | WS_VISIBLE, 10, 280, 80, 20, hWnd, NULL, NULL, NULL);
            g_hHandleEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 90, 278, 150, 25, hWnd, (HMENU)201, NULL, NULL);
            g_disableControls.push_back(g_hHandleEdit);
            g_hSetHandleBtn = CreateWindowW(L"BUTTON", L"设置为目标", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 250, 278, 100, 25, hWnd, (HMENU)202, NULL, NULL);
            g_disableControls.push_back(g_hSetHandleBtn);

            // 透明度
            CreateWindowW(L"STATIC", L"透明度:", WS_CHILD | WS_VISIBLE, 10, 320, 60, 20, hWnd, NULL, NULL, NULL);
            g_hSlider = CreateWindowW(TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS,
                                      70, 315, 250, 30, hWnd, (HMENU)203, NULL, NULL);
            // ===== 关键修复：设置滑块范围为 0~255（0%~100%） =====
            SendMessageW(g_hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 255));
            SendMessageW(g_hSlider, TBM_SETPOS, TRUE, 255);
            // =====================================================
            g_disableControls.push_back(g_hSlider);
            g_hAlphaValue = CreateWindowW(L"STATIC", L"100%", WS_CHILD | WS_VISIBLE, 330, 315, 50, 20, hWnd, NULL, NULL, NULL);
            g_disableControls.push_back(g_hAlphaValue);

            // 置顶 & 无边框
            g_hTopmostCheck = CreateWindowW(L"BUTTON", L"窗口置顶", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                            10, 360, 100, 25, hWnd, (HMENU)204, NULL, NULL);
            g_disableControls.push_back(g_hTopmostCheck);
            g_hNoBorderCheck = CreateWindowW(L"BUTTON", L"无边框", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                             120, 360, 80, 25, hWnd, (HMENU)206, NULL, NULL);
            g_disableControls.push_back(g_hNoBorderCheck);
            g_hApplyAttrBtn = CreateWindowW(L"BUTTON", L"应用透明度和置顶", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                            210, 360, 150, 25, hWnd, (HMENU)205, NULL, NULL);
            g_disableControls.push_back(g_hApplyAttrBtn);

            // 位置大小
            CreateWindowW(L"STATIC", L"X:", WS_CHILD | WS_VISIBLE, 10, 400, 30, 20, hWnd, NULL, NULL, NULL);
            g_hXEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 40, 398, 60, 25, hWnd, (HMENU)301, NULL, NULL);
            g_disableControls.push_back(g_hXEdit);
            CreateWindowW(L"STATIC", L"Y:", WS_CHILD | WS_VISIBLE, 110, 400, 30, 20, hWnd, NULL, NULL, NULL);
            g_hYEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 140, 398, 60, 25, hWnd, (HMENU)302, NULL, NULL);
            g_disableControls.push_back(g_hYEdit);
            g_hGetPosBtn = CreateWindowW(L"BUTTON", L"获取", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 210, 398, 60, 25, hWnd, (HMENU)303, NULL, NULL);
            g_disableControls.push_back(g_hGetPosBtn);
            g_hMoveBtn = CreateWindowW(L"BUTTON", L"移动窗口", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 280, 398, 80, 25, hWnd, (HMENU)304, NULL, NULL);
            g_disableControls.push_back(g_hMoveBtn);

            CreateWindowW(L"STATIC", L"宽度:", WS_CHILD | WS_VISIBLE, 10, 440, 40, 20, hWnd, NULL, NULL, NULL);
            g_hWEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 55, 438, 60, 25, hWnd, (HMENU)305, NULL, NULL);
            g_disableControls.push_back(g_hWEdit);
            CreateWindowW(L"STATIC", L"高度:", WS_CHILD | WS_VISIBLE, 125, 440, 40, 20, hWnd, NULL, NULL, NULL);
            g_hHEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 170, 438, 60, 25, hWnd, (HMENU)306, NULL, NULL);
            g_disableControls.push_back(g_hHEdit);
            g_hResizeBtn = CreateWindowW(L"BUTTON", L"调整大小", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 240, 438, 100, 25, hWnd, (HMENU)307, NULL, NULL);
            g_disableControls.push_back(g_hResizeBtn);

            // 进程信息（保留可用）
            g_hProcInfoStatic = CreateWindowW(L"STATIC", L"进程: 无", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                               10, 475, 400, 25, hWnd, NULL, NULL, NULL);

            // NSudo权限（保留可用）
            CreateWindowW(L"STATIC", L"NSudo权限:", WS_CHILD | WS_VISIBLE, 10, 505, 70, 20, hWnd, NULL, NULL, NULL);
            g_hUserCombo = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
                                         85, 503, 130, 150, hWnd, (HMENU)500, NULL, NULL);
            SendMessageW(g_hUserCombo, CB_ADDSTRING, 0, (LPARAM)L"TrustedInstaller");
            SendMessageW(g_hUserCombo, CB_ADDSTRING, 0, (LPARAM)L"SYSTEM");
            SendMessageW(g_hUserCombo, CB_ADDSTRING, 0, (LPARAM)L"管理员");
            SendMessageW(g_hUserCombo, CB_SETCURSEL, 1, 0);

            // 结束进程按钮（普通可用，NSudo保留）
            g_hEndProcessBtn = CreateWindowW(L"BUTTON", L"结束进程(普通)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                             230, 503, 100, 25, hWnd, (HMENU)501, NULL, NULL);
            // 注意：普通结束进程不加入禁用列表，所以进程模式下仍然可用
            g_hElevatedKillBtn = CreateWindowW(L"BUTTON", L"NSudo结束进程", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                               340, 503, 120, 25, hWnd, (HMENU)502, NULL, NULL);

            // 启动命令行（禁用）
            CreateWindowW(L"STATIC", L"启动命令行:", WS_CHILD | WS_VISIBLE, 10, 540, 70, 20, hWnd, NULL, NULL, NULL);
            g_hLaunchCmdEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                             85, 538, 375, 25, hWnd, (HMENU)503, NULL, NULL);
            g_disableControls.push_back(g_hLaunchCmdEdit);
            g_hLaunchBtn = CreateWindowW(L"BUTTON", L"NSudo启动", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                         470, 538, 100, 25, hWnd, (HMENU)504, NULL, NULL);
            g_disableControls.push_back(g_hLaunchBtn);

            RefreshList();
            break;
        }

        case WM_HSCROLL: {
            if ((HWND)lParam == g_hSlider && g_targetWnd && IsWindow(g_targetWnd)) {
                int pos = (int)SendMessageW(g_hSlider, TBM_GETPOS, 0, 0);
                wchar_t buf[16];
                swprintf(buf, 16, L"%d%%", pos * 100 / 255);
                SetWindowTextW(g_hAlphaValue, buf);
                SetTransparency(g_targetWnd, (BYTE)pos);
            }
            break;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == 101) {
                int idx = (int)SendMessageW(g_hListBox, LB_GETCURSEL, 0, 0);
                if (idx == LB_ERR) return 0;

                if (!g_bProcessMode) {
                    if (idx >= 0 && idx < (int)g_windows.size()) {
                        g_targetWnd = g_windows[idx];
                        g_targetPid = 0;
                        UpdateDisplay();
                    } else {
                        SetWindowTextW(g_hInfoText, L"选中失败，请刷新列表重试");
                    }
                } else {
                    wchar_t text[256];
                    SendMessageW(g_hListBox, LB_GETTEXT, idx, (LPARAM)text);
                    if (wcsstr(text, L"---") == text) {
                        g_targetPid = 0;
                        g_targetWnd = NULL;
                        UpdateDisplay();
                        return 0;
                    }
                    wchar_t* pidStart = wcsstr(text, L"(PID: ");
                    if (pidStart) {
                        pidStart += 6;
                        DWORD pid = _wtoi(pidStart);
                        if (pid > 0) {
                            g_targetPid = pid;
                            g_targetWnd = NULL;
                            UpdateDisplay();
                        } else {
                            g_targetPid = 0;
                            g_targetWnd = NULL;
                            UpdateDisplay();
                        }
                    } else {
                        g_targetPid = 0;
                        g_targetWnd = NULL;
                        UpdateDisplay();
                    }
                }
            } else if (id == 102) {
                RefreshList();
            } else if (id == 104) {
                g_targetWnd = NULL;
                g_targetPid = 0;
                RefreshList();
            } else if (id == 202) {
                wchar_t buf[64];
                GetWindowTextW(g_hHandleEdit, buf, 64);
                HWND h = ParseHandle(buf);
                if (h) {
                    g_targetWnd = h;
                    g_targetPid = 0;
                    UpdateDisplay();
                    MessageBoxW(hWnd, L"成功设置目标窗口", L"提示", MB_OK);
                } else {
                    MessageBoxW(hWnd, L"无效的窗口句柄", L"错误", MB_ICONERROR);
                }
            } else if (id == 204) {
                if (g_targetWnd && IsWindow(g_targetWnd)) {
                    BOOL checked = (SendMessageW(g_hTopmostCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    SetTopmost(g_targetWnd, checked);
                }
            } else if (id == 206) {
                if (g_targetWnd && IsWindow(g_targetWnd)) {
                    BOOL checked = (SendMessageW(g_hNoBorderCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    SetNoBorder(g_targetWnd, checked);
                }
            } else if (id == 205) {
                if (g_targetWnd && IsWindow(g_targetWnd)) {
                    int pos = (int)SendMessageW(g_hSlider, TBM_GETPOS, 0, 0);
                    SetTransparency(g_targetWnd, (BYTE)pos);
                    BOOL checked = (SendMessageW(g_hTopmostCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    SetTopmost(g_targetWnd, checked);
                    MessageBoxW(hWnd, L"已应用透明度和置顶设置", L"成功", MB_OK);
                } else {
                    MessageBoxW(hWnd, L"请先在列表中选中一个窗口", L"提示", MB_OK);
                }
            } else if (id == 303) {
                if (g_targetWnd && IsWindow(g_targetWnd)) {
                    RECT rc;
                    GetWindowRect(g_targetWnd, &rc);
                    wchar_t buf[32];
                    swprintf(buf, 32, L"%d", rc.left);   SetWindowTextW(g_hXEdit, buf);
                    swprintf(buf, 32, L"%d", rc.top);    SetWindowTextW(g_hYEdit, buf);
                    swprintf(buf, 32, L"%d", rc.right - rc.left); SetWindowTextW(g_hWEdit, buf);
                    swprintf(buf, 32, L"%d", rc.bottom - rc.top); SetWindowTextW(g_hHEdit, buf);
                } else {
                    MessageBoxW(hWnd, L"没有选中有效窗口", L"提示", MB_OK);
                }
            } else if (id == 304) {
                if (g_targetWnd && IsWindow(g_targetWnd)) {
                    int x = GetDlgItemInt(hWnd, 301, NULL, FALSE);
                    int y = GetDlgItemInt(hWnd, 302, NULL, FALSE);
                    MoveWindowTo(g_targetWnd, x, y);
                } else {
                    MessageBoxW(hWnd, L"没有选中有效窗口", L"提示", MB_OK);
                }
            } else if (id == 307) {
                if (g_targetWnd && IsWindow(g_targetWnd)) {
                    int w = GetDlgItemInt(hWnd, 305, NULL, FALSE);
                    int hgt = GetDlgItemInt(hWnd, 306, NULL, FALSE);
                    ResizeWindowTo(g_targetWnd, w, hgt);
                } else {
                    MessageBoxW(hWnd, L"没有选中有效窗口", L"提示", MB_OK);
                }
            } else if (id == 501) {
                KillProcessNormal();
            } else if (id == 502) {
                KillProcessElevatedWithNsudo();
            } else if (id == 504) {
                LaunchProcessWithNsudo();
            }
            break;
        }

        case WM_GETMINMAXINFO: {
            MINMAXINFO* pInfo = (MINMAXINFO*)lParam;
            pInfo->ptMinTrackSize.x = 650;
            pInfo->ptMinTrackSize.y = 650;
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icc);

    WNDCLASSEXW wc = {sizeof(WNDCLASSEXW), 0, WndProc, 0, 0, hInst,
                      NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1), NULL,
                      L"SimpleManagerClass"};
    RegisterClassExW(&wc);

    g_hMainWnd = CreateWindowExW(0, L"SimpleManagerClass", L"窗口管理工具 - 可选择NSudo权限",
                                 WS_OVERLAPPEDWINDOW,
                                 100, 100, 650, 650, NULL, NULL, hInst, NULL);
    ShowWindow(g_hMainWnd, nShow);
    UpdateWindow(g_hMainWnd);

    SetWindowPos(g_hMainWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}