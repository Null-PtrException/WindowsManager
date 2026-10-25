#include "WindowsManager.h"

// ===== 全局变量定义（全项目唯一一份）=====
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
HWND g_hFullscreenCheck = NULL;
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

// 父子窗口功能控件
HWND g_hPickChildBtn = NULL;
HWND g_hPickParentBtn = NULL;
HWND g_hApplyParentChildBtn = NULL;
HWND g_hUnparentBtn = NULL;
HWND g_pickChildWnd = NULL;
HWND g_pickParentWnd = NULL;

// 需要禁用的控件列表
std::vector<HWND> g_disableControls;

DWORD g_currentPid = 0;
wchar_t g_currentProcName[256] = {0};