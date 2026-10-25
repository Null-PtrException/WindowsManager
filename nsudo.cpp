#include "WindowsManager.h"

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

// 定位 NSudo 可执行文件路径，返回是否找到
static bool LocateNsudoPath(wchar_t* nsudoPath, size_t cch) {
    wchar_t exePath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    PathRemoveFileSpecW(exePath);

    swprintf(nsudoPath, cch, L"%ls\\NSudoL.exe", exePath);
    if (GetFileAttributesW(nsudoPath) != INVALID_FILE_ATTRIBUTES)
        return true;

    swprintf(nsudoPath, cch, L"%ls\\NSudo.exe", exePath);
    if (GetFileAttributesW(nsudoPath) != INVALID_FILE_ATTRIBUTES)
        return true;

    return false;
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

    wchar_t nsudoPath[MAX_PATH];
    if (!LocateNsudoPath(nsudoPath, MAX_PATH)) {
        MessageBoxW(g_hMainWnd, L"未找到 NSudoL.exe 或 NSudo.exe\n请将无窗口化 NSudo 放在本程序目录下。", L"错误", MB_ICONERROR);
        return;
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

    wchar_t nsudoPath[MAX_PATH];
    if (!LocateNsudoPath(nsudoPath, MAX_PATH)) {
        MessageBoxW(g_hMainWnd, L"未找到 NSudoL.exe 或 NSudo.exe\n请将无窗口化 NSudo 放在本程序目录下。", L"错误", MB_ICONERROR);
        return;
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