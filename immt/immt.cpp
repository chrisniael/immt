#include "framework.h"
#include "immt.h"
#include <shellapi.h>

#include "core.h"

#define MAX_LOADSTRING 100
#define TRAY_ICON_ID 1
#define WM_TRAYICON (WM_USER + 1)

// Global Variables:
HINSTANCE hInst;                                // current instance
HANDLE hMutex;                                  // Global mutex handle
NOTIFYICONDATA nid;                             // NOTIFYICONDATA structure for the tray icon
HWND hWnd;                                      // Handle to the main window
bool isAboutDialogOpen = false;                 // Flag to indicate if the About dialog is open

// Forward declarations of functions included in this code module:
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void InitTrayIcon(HWND hWnd);
void ShowContextMenu(HWND hWnd);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    hInst = hInstance; // Store instance handle in our global variable

    // Create a named mutex
    hMutex = CreateMutex(nullptr, FALSE, L"immt_single_instance_mutex");
    if (hMutex == nullptr)
    {
        // Handle error if mutex creation failed
        return FALSE;
    }

    // Check if another instance is running
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // Another instance is already running, exit the application
        CloseHandle(hMutex);
        return FALSE;
    }

    WNDCLASSEX wcex;
    ZeroMemory(&wcex, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = L"IMMTWndClass";
    RegisterClassEx(&wcex);

    hWnd = CreateWindowW(L"IMMTWndClass", L"IMMT", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd)
    {
        return FALSE;
    }

    InitTrayIcon(hWnd);
    
    if (!InstallHook()) {
        return FALSE;
    }

    MSG msg;

    // Main message loop: running without a window
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UninstallHook();

    // Release the mutex
    CloseHandle(hMutex);

    // Remove the tray icon
    Shell_NotifyIcon(NIM_DELETE, &nid);

    return (int)msg.wParam;
}

//
//  FUNCTION: InitTrayIcon(HWND)
//
//  PURPOSE: Initializes and adds the tray icon.
//
void InitTrayIcon(HWND hWnd)
{
    ZeroMemory(&nid, sizeof(NOTIFYICONDATA));

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = TRAY_ICON_ID;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_IMMT)); // Use your icon resource ID here
    wcscpy_s(nid.szTip, L"IMMT Application");

    Shell_NotifyIcon(NIM_ADD, &nid);
}

//
//  FUNCTION: ShowContextMenu(HWND)
//
//  PURPOSE: Displays a context menu for the tray icon.
//
void ShowContextMenu(HWND hWnd)
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, IDM_ABOUT, L"关于");  // Change to Chinese "关于"
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);      // Add a separator line
    AppendMenu(hMenu, MF_STRING, IDM_EXIT, L"退出");  // Change to Chinese "退出"

    // Display the menu
    SetForegroundWindow(hWnd); // Required to bring up the context menu properly
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, nullptr);
    DestroyMenu(hMenu);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP)
        {
            ShowContextMenu(hWnd);
        }
        break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDM_ABOUT:
            if (!isAboutDialogOpen) // Check if the About dialog is already open
            {
                isAboutDialogOpen = true; // Set the flag to true indicating the About dialog is open
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            }
            break;
        case IDM_EXIT:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
    {
        // Center the dialog on the screen
        RECT rcDlg, rcScreen;
        GetWindowRect(hDlg, &rcDlg);
        GetWindowRect(GetDesktopWindow(), &rcScreen);

        int posX = (rcScreen.right - rcScreen.left - (rcDlg.right - rcDlg.left)) / 2;
        int posY = (rcScreen.bottom - rcScreen.top - (rcDlg.bottom - rcDlg.top)) / 2;

        SetWindowPos(hDlg, HWND_TOP, posX, posY, 0, 0, SWP_NOSIZE);

        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            isAboutDialogOpen = false; // Reset the flag when the About dialog is closed
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}