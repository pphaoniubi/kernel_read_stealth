#include "draw.h"

static HWND g_foundWindow = nullptr;
static DWORD g_targetPid = 0;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    if (pid == g_targetPid && IsWindowVisible(hwnd))
    {
        g_foundWindow = hwnd;
        return FALSE; // stop
    }

    return TRUE;
}

HWND FindWindowByPID(DWORD pid)
{
    g_targetPid = pid;
    g_foundWindow = nullptr;

    EnumWindows(EnumWindowsProc, 0);

    return g_foundWindow;
}