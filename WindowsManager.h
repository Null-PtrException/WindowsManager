#pragma once

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

// ===== 全局变量（声明，定义在 globals.cpp）=====
extern HWND g_hMainWnd;
extern HWND g_hListBox;
extern std::vector<HWND> g_windows;
extern std::vector<std::wstring> g_titles;
extern HWND g_targetWnd;
extern DWORD g_targetPid;
extern bool g_bProcessMode;

// 控件
extern HWND g_hRefreshBtn;
extern HWND g_hInfoText;
extern HWND g_hHandleEdit;
extern HWND g_hSetHandleBtn;
extern HWND g_hSlider;
extern HWND g_hAlphaValue;
extern HWND g_hTopmostCheck;
extern HWND g_hFullscreenCheck;
extern HWND g_hXEdit, g_hYEdit;
extern HWND g_hWEdit, g_hHEdit;
extern HWND g_hGetPosBtn;
extern HWND g_hMoveBtn;
extern HWND g_hResizeBtn;
extern HWND g_hProcInfoStatic;
extern HWND g_hEndProcessBtn;
extern HWND g_hElevatedKillBtn;
extern HWND g_hLaunchCmdEdit;
extern HWND g_hLaunchBtn;
extern HWND g_hUserCombo;
extern HWND g_hNoBorderCheck;
extern HWND g_hShowProcessCheck;

// 父子窗口功能控件
extern HWND g_hPickChildBtn;
extern HWND g_hPickParentBtn;
extern HWND g_hApplyParentChildBtn;
extern HWND g_hUnparentBtn;
extern HWND g_pickChildWnd;
extern HWND g_pickParentWnd;

// 需要禁用的控件列表
extern std::vector<HWND> g_disableControls;

extern DWORD g_currentPid;
extern wchar_t g_currentProcName[256];

// ===== 函数接口 =====

// ---- window_api.cpp ----
void SetTransparency(HWND hWnd, BYTE alpha);
void SetTopmost(HWND hWnd, bool top);
void SetNoBorder(HWND hWnd, bool noBorder);
void SetFullscreen(HWND hWnd, bool full);
void MoveWindowTo(HWND hWnd, int x, int y);
void ResizeWindowTo(HWND hWnd, int w, int h);
HWND ParseHandle(const wchar_t* str);

// ---- process_api.cpp ----
std::wstring GetProcessNameByPid(DWORD pid);
bool TerminateProcessByPid(DWORD pid);
bool IsSystemProcess(const std::wstring& name);
bool ProcessHasVisibleWindow(DWORD pid);
std::vector<ProcessInfo> GetProcessList();
void KillProcessNormal();

// ---- nsudo.cpp ----
std::wstring GetSelectedUserOption();
void KillProcessElevatedWithNsudo();
void LaunchProcessWithNsudo();

// ---- parent_child.cpp ----
HWND PickWindowModal(HWND hParent);
void ApplyParentChild();
void UnparentChild();
void UpdatePickButtonText(HWND hBtn, HWND hWnd, const wchar_t* placeholder);

// ---- ui.cpp ----
std::wstring GetWindowDisplay(HWND hWnd);
BOOL CALLBACK EnumProc(HWND hWnd, LPARAM lParam);
void UpdateDisplay();
void RefreshList();
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);