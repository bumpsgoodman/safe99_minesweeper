#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <windowsx.h>

#include "game.h"
#include "mouse_event.h"

HINSTANCE g_hinstance;
HWND g_hwnd;

game_t* gp_game;

HRESULT init_window(const size_t width, const size_t height);
LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);

int main(void)
{
    int rows;
    int cols;

    printf("행: ");
    scanf("%d", &rows);

    printf("열: ");
    scanf("%d", &cols);

    const size_t window_width = cols * 16;
    const size_t window_height = rows * 16 + 48;

    if (FAILED(init_window(window_width, window_height)))
    {
        return 0;
    }

    gp_game = (game_t*)malloc(sizeof(game_t));
    if (!init_game(g_hwnd, gp_game, rows, cols, 10))
    {
        return 0;
    }

    // Main message loop
    MSG msg = { 0 };
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            update_game(gp_game);
            draw_game(gp_game);
        }
    }

    shutdown_game(gp_game);
    free(gp_game);

    return (int)msg.wParam;
}

HRESULT init_window(const size_t width, const size_t height)
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = wnd_proc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetModuleHandleW(NULL);
    wcex.hIcon = NULL;
    wcex.hCursor = NULL;
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"safe99";
    wcex.hIconSm = NULL;
    if (!RegisterClassEx(&wcex))
    {
        return E_FAIL;
    }

    // Create window
    g_hinstance = wcex.hInstance;
    RECT rc = { 0, 0, (LONG)width, (LONG)height };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hwnd = CreateWindow(wcex.lpszClassName, L"safe99 minesweeper",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, wcex.hInstance,
        NULL);
    if (!g_hwnd)
    {
        return E_FAIL;
    }

    ShowWindow(g_hwnd, SW_SHOWDEFAULT);

    return S_OK;
}

LRESULT CALLBACK wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_LBUTTONDOWN:
    {
        uint32_t x = GET_X_LPARAM(lParam);
        uint32_t y = GET_Y_LPARAM(lParam);
        on_left_down_mouse(x, y);
        break;
    }
    case WM_LBUTTONUP:
    {
        uint32_t x = GET_X_LPARAM(lParam);
        uint32_t y = GET_Y_LPARAM(lParam);
        on_left_up_mouse(x, y);
        break;
    }
    case WM_MOUSEMOVE:
    {
        uint32_t x = GET_X_LPARAM(lParam);
        uint32_t y = GET_Y_LPARAM(lParam);
        on_move_mouse(x, y);
        break;
    }

    case WM_MOVE:
        if (gp_game != NULL && gp_game->pa_renderer != NULL)
        {
            renderer_ddraw_update_window_pos(gp_game->pa_renderer);
        }
        break;
    case WM_SIZE:
        if (gp_game != NULL && gp_game->pa_renderer != NULL)
        {
            renderer_ddraw_update_window_size(gp_game->pa_renderer);
        }
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}