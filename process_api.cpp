#include "WindowsManager.h"

// ---------- 进程辅助 ----------
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

// ---------- 普通结束进程 ----------
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