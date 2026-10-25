#include "WindowsManager.h"

// ---------- 显示相关 ----------
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

void UpdateDisplay() {
    if (g_bProcessMode) {
        for (HWND h : g_disableControls) {
            EnableWindow(h, FALSE);
        }
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
        SetWindowTextW(g_hHandleEdit, L"");
        SetWindowTextW(g_hXEdit, L"");
        SetWindowTextW(g_hYEdit, L"");
        SetWindowTextW(g_hWEdit, L"");
        SetWindowTextW(g_hHEdit, L"");
        SendMessageW(g_hTopmostCheck, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageW(g_hNoBorderCheck, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageW(g_hFullscreenCheck, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessageW(g_hSlider, TBM_SETPOS, TRUE, 255);
        SetWindowTextW(g_hAlphaValue, L"100%");
        return;
    }

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
        SendMessageW(g_hFullscreenCheck, BM_SETCHECK, BST_UNCHECKED, 0);
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

// ---------- 窗口过程 ----------
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            CreateWindowW(L"STATIC", L"所有可见窗口:", WS_CHILD | WS_VISIBLE, 10, 10, 150, 20, hWnd, NULL, NULL, NULL);
            g_hListBox = CreateWindowW(L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
                                       10, 30, 500, 180, hWnd, (HMENU)101, NULL, NULL);

            g_hRefreshBtn = CreateWindowW(L"BUTTON", L"刷新列表", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                          520, 30, 100, 30, hWnd, (HMENU)102, NULL, NULL);

            g_hShowProcessCheck = CreateWindowW(L"BUTTON", L"显示进程列表", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                                520, 65, 120, 25, hWnd, (HMENU)104, NULL, NULL);
            SendMessageW(g_hShowProcessCheck, BM_SETCHECK, BST_UNCHECKED, 0);

            g_hInfoText = CreateWindowW(L"STATIC", L"未选中窗口", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                        10, 220, 430, 50, hWnd, NULL, NULL, NULL);

            CreateWindowW(L"STATIC", L"窗口句柄:", WS_CHILD | WS_VISIBLE, 10, 280, 80, 20, hWnd, NULL, NULL, NULL);
            g_hHandleEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 90, 278, 150, 25, hWnd, (HMENU)201, NULL, NULL);
            g_disableControls.push_back(g_hHandleEdit);
            g_hSetHandleBtn = CreateWindowW(L"BUTTON", L"设置为目标", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 250, 278, 100, 25, hWnd, (HMENU)202, NULL, NULL);
            g_disableControls.push_back(g_hSetHandleBtn);

            CreateWindowW(L"STATIC", L"透明度:", WS_CHILD | WS_VISIBLE, 10, 320, 60, 20, hWnd, NULL, NULL, NULL);
            g_hSlider = CreateWindowW(TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS,
                                      70, 315, 250, 30, hWnd, (HMENU)203, NULL, NULL);
            SendMessageW(g_hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 255));
            SendMessageW(g_hSlider, TBM_SETPOS, TRUE, 255);
            g_disableControls.push_back(g_hSlider);
            g_hAlphaValue = CreateWindowW(L"STATIC", L"100%", WS_CHILD | WS_VISIBLE, 330, 315, 50, 20, hWnd, NULL, NULL, NULL);
            g_disableControls.push_back(g_hAlphaValue);

            // 置顶 / 无边框 / 全屏
            g_hTopmostCheck = CreateWindowW(L"BUTTON", L"窗口置顶", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                            10, 360, 100, 25, hWnd, (HMENU)204, NULL, NULL);
            g_disableControls.push_back(g_hTopmostCheck);
            g_hNoBorderCheck = CreateWindowW(L"BUTTON", L"无边框", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                             120, 360, 80, 25, hWnd, (HMENU)206, NULL, NULL);
            g_disableControls.push_back(g_hNoBorderCheck);
            g_hFullscreenCheck = CreateWindowW(L"BUTTON", L"全屏", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                               210, 360, 80, 25, hWnd, (HMENU)207, NULL, NULL);
            g_disableControls.push_back(g_hFullscreenCheck);

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

            g_hProcInfoStatic = CreateWindowW(L"STATIC", L"进程: 无", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                               10, 475, 400, 25, hWnd, NULL, NULL, NULL);

            CreateWindowW(L"STATIC", L"NSudo权限:", WS_CHILD | WS_VISIBLE, 10, 505, 70, 20, hWnd, NULL, NULL, NULL);
            g_hUserCombo = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
                                         85, 503, 130, 150, hWnd, (HMENU)500, NULL, NULL);
            SendMessageW(g_hUserCombo, CB_ADDSTRING, 0, (LPARAM)L"TrustedInstaller");
            SendMessageW(g_hUserCombo, CB_ADDSTRING, 0, (LPARAM)L"SYSTEM");
            SendMessageW(g_hUserCombo, CB_ADDSTRING, 0, (LPARAM)L"管理员");
            SendMessageW(g_hUserCombo, CB_SETCURSEL, 1, 0);

            g_hEndProcessBtn = CreateWindowW(L"BUTTON", L"结束进程(普通)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                             230, 503, 100, 25, hWnd, (HMENU)501, NULL, NULL);
            g_hElevatedKillBtn = CreateWindowW(L"BUTTON", L"NSudo结束进程", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                               340, 503, 120, 25, hWnd, (HMENU)502, NULL, NULL);

            CreateWindowW(L"STATIC", L"启动命令行:", WS_CHILD | WS_VISIBLE, 10, 540, 70, 20, hWnd, NULL, NULL, NULL);
            g_hLaunchCmdEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                             85, 538, 375, 25, hWnd, (HMENU)503, NULL, NULL);
            g_disableControls.push_back(g_hLaunchCmdEdit);
            g_hLaunchBtn = CreateWindowW(L"BUTTON", L"NSudo启动", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                         470, 538, 100, 25, hWnd, (HMENU)504, NULL, NULL);
            g_disableControls.push_back(g_hLaunchBtn);

            // ===== 将 A 设为 B 的子窗口 =====
            CreateWindowW(L"STATIC", L"将", WS_CHILD | WS_VISIBLE, 10, 578, 30, 25, hWnd, NULL, NULL, NULL);
            g_hPickChildBtn = CreateWindowW(L"BUTTON", L"选择窗口A", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                            40, 576, 170, 25, hWnd, (HMENU)601, NULL, NULL);
            CreateWindowW(L"STATIC", L"设为", WS_CHILD | WS_VISIBLE, 215, 578, 45, 25, hWnd, NULL, NULL, NULL);
            g_hPickParentBtn = CreateWindowW(L"BUTTON", L"选择窗口B", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                             260, 576, 170, 25, hWnd, (HMENU)602, NULL, NULL);
            CreateWindowW(L"STATIC", L"的子窗口", WS_CHILD | WS_VISIBLE, 435, 578, 75, 25, hWnd, NULL, NULL, NULL);
            g_hApplyParentChildBtn = CreateWindowW(L"BUTTON", L"执行", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                                   515, 576, 80, 25, hWnd, (HMENU)603, NULL, NULL);
            g_hUnparentBtn = CreateWindowW(L"BUTTON", L"取消继承", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                           10, 608, 90, 25, hWnd, (HMENU)604, NULL, NULL);
            CreateWindowW(L"STATIC", L"*注：请务必在切换前取消，否则可能要关闭父窗口",
                          WS_CHILD | WS_VISIBLE, 108, 612, 490, 20, hWnd, NULL, NULL, NULL);
            // =================================

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
            } else if (id == 207) {   // ← 全屏
                if (g_targetWnd && IsWindow(g_targetWnd)) {
                    BOOL checked = (SendMessageW(g_hFullscreenCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    
                    if (checked) {
                        // 1. 保险：强制剥夺目标窗口原本的置顶属性
                        SetWindowPos(g_targetWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                        
                        // 2. 目标窗口全屏（非置顶，只铺满）
                        SetFullscreen(g_targetWnd, true);
                        
                        // 3. 自己（管理工具）强行置顶
                        SetWindowPos(g_hMainWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    } else {
                        // 取消全屏
                        SetFullscreen(g_targetWnd, false);
                    }
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
            } else if (id == 601) {
                HWND picked = PickWindowModal(hWnd);
                if (picked) {
                    g_pickChildWnd = picked;
                    UpdatePickButtonText(g_hPickChildBtn, g_pickChildWnd, L"选择窗口A");
                }
            } else if (id == 602) {
                HWND picked = PickWindowModal(hWnd);
                if (picked) {
                    g_pickParentWnd = picked;
                    UpdatePickButtonText(g_hPickParentBtn, g_pickParentWnd, L"选择窗口B");
                }
            } else if (id == 603) {
                ApplyParentChild();
            } else if (id == 604) {
                UnparentChild();
            }
            break;
        }

        case WM_GETMINMAXINFO: {
            MINMAXINFO* pInfo = (MINMAXINFO*)lParam;
            pInfo->ptMinTrackSize.x = 650;
            pInfo->ptMinTrackSize.y = 660;
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}