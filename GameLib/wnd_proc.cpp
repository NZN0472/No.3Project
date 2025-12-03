#include "game_lib.h"

namespace GameLib {
    void startHrTimer();
    void stopHrTimer();

    //--------------------------------------------------------------
    //  ウインドウプロシージャ
    //--------------------------------------------------------------
    LRESULT CALLBACK fnWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        using DirectX::Keyboard;

        // マウスホイール用
        static int nWheelFraction = 0;	// 回転量の端数

        // マウス移動用
        static POINT prevPos;

        switch (msg)
        {

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            hdc = BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            break;
        }

        case WM_CLOSE:
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_ACTIVATEAPP:
            Keyboard::ProcessMessage(msg, wParam, lParam);
            nWheelFraction = 0;
            break;

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE)
            {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                break;
            }
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
            Keyboard::ProcessMessage(msg, wParam, lParam);
            break;

        case WM_ENTERSIZEMOVE:
            // WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
            stopHrTimer();
            //m.hrTimer.stop();
            break;

        case WM_EXITSIZEMOVE:
            // WM_EXITSIZEMOVE is sent when the user releases the resize bars.
            // Here we reset everything based on the new window dimensions.
            startHrTimer();
            //m.hrTimer.start();
            break;

        case WM_LBUTTONDOWN:
        {
            SetCapture(hwnd);   // カーソルが画面外に出てもつかんだまま
            break;
        }

        case WM_LBUTTONUP:
        {
            ReleaseCapture();   // ウィンドウを放す
            break;
        }

        case WM_RBUTTONDOWN:
        {
            SetCapture(hwnd);   // カーソルが画面外に出てもつかんだまま
            break;
        }

        case WM_RBUTTONUP:
        {
            ReleaseCapture();   // ウィンドウを放す
            break;
        }

        case WM_MOUSEMOVE:
        {
            break;
        }

        case WM_MOUSEWHEEL:
        {
            break;
        }

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

        return 0;
    }
}
