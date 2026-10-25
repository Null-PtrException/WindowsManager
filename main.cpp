#include "WindowsManager.h"

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
                                 100, 100, 650, 680, NULL, NULL, hInst, NULL);
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