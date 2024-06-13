#include <SDL2/SDL.h>
#include <windows.h>
#include <iostream>
#include <sstream>
#include <chrono>
#include "Shell.h"
 
uint8_t scale = 1;
Shell shell;

HBITMAP hbrBackground;
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);
TCHAR szTitle[] = "Clover classic mini";     
TCHAR szWindowClass[] = "CLV-FMC";
HANDLE hThread;
DWORD threadID;
HWND hWnd;
HDC hdc;

LRESULT CALLBACK WindowProcedure (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_DESTROY:
            CloseHandle(hThread);
            ReleaseDC(hWnd, hdc);
            DeleteDC(hdc);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}
namespace ENGINE {
    SDL_AudioDeviceID adid;
    SDL_AudioSpec wS;
    uint16_t W = SCREEN_SIZE*scale;
    uint16_t H = 240*scale;
    uint32_t * frm_buffer = new uint32_t[SCREEN_SIZE*240];
    bool running = false;
    int pitch = 0;
    void Init() {
        SDL_Init(SDL_INIT_AUDIO);
        shell.SetFrameBuffer(frm_buffer);
        SDL_zero(wS);
        wS.freq =   shell.SoundSamplesPerSec;
        wS.format = AUDIO_S16SYS;
        wS.channels = 1;
        adid = SDL_OpenAudioDevice(NULL, 0, &wS, NULL, 0);
        if (adid) SDL_PauseAudioDevice(adid, 0);
        running = true;
    }

    void FlushScanline() { 
        HDC src_hdc = CreateCompatibleDC(hdc);
        HBITMAP frm_bmp = CreateBitmap(SCREEN_SIZE,240,1,32, frm_buffer);
        SelectObject(src_hdc, frm_bmp);
        StretchBlt(hdc, 0, 0, W, H, src_hdc, 0,0,SCREEN_SIZE,240, SRCCOPY);
        DeleteDC(src_hdc);
        DeleteObject(frm_bmp);
    }

    void flip() {
        FlushScanline();
    }
}

DWORD WINAPI ThreadProc(LPVOID lpParameter) {
    bool& running = *((bool*)lpParameter);
    auto lastTime = std::chrono::steady_clock::now();
    double FPS = 0;
    double minFPS = -1;
    double aFPS = -1;
    const double maxPeriod = 1.0 / 61.0f;
    char windowFpsString[_MAX_PATH];
    while( ENGINE::running ) {
        auto time = std::chrono::steady_clock::now();
        double diff = std::chrono::duration<double> (time - lastTime).count();
        if ( diff >= maxPeriod ) {
            FPS  =  1.0 / diff;
            if (minFPS < 0 || minFPS > FPS) minFPS = FPS;
            if (aFPS < 0 || FPS < 60.9f) aFPS = FPS; 
            lastTime = time;
            sprintf(windowFpsString, "Clover Mini | FPS: %.1f | minFPS:  %.1f | aFPS:  %.1f", FPS, minFPS, aFPS);
            SetWindowText(hWnd, (LPCTSTR)windowFpsString);  
            uint8_t kb = 0x00;
            kb |= GetAsyncKeyState(0x58) ? 0x80 : 0x00; // A Button
            kb |= GetAsyncKeyState(0x5A) ? 0x40 : 0x00; // B Button 2
            kb |= GetAsyncKeyState(VK_RSHIFT) ? 0x20 : 0x00; // Select
            kb |= GetAsyncKeyState(VK_RETURN) ? 0x10 : 0x00; // Start
            kb |= (GetAsyncKeyState(VK_UP)&&!GetAsyncKeyState(VK_DOWN)) ? 0x08 : 0x00;
            kb |= (GetAsyncKeyState(VK_DOWN)&&!GetAsyncKeyState(VK_UP)) ? 0x04 : 0x00;
            kb |= (GetAsyncKeyState(VK_LEFT)&&!GetAsyncKeyState(VK_RIGHT)) ? 0x02 : 0x00;
            kb |= (GetAsyncKeyState(VK_RIGHT)&&!GetAsyncKeyState(VK_LEFT))  ? 0x01 : 0x00;
            shell.setJoyState(kb);
            shell.Update(FPS);
            SDL_QueueAudio(ENGINE::adid, shell.getAudioSample(), shell.audioBufferCount*2);
            ENGINE::flip();  
        }
    } 
    return 0;
}


int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    if (!shell.LoadData()) return 0;
    MSG msg;
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)WindowProcedure;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = CreateSolidBrush(0x00);
    wcex.lpszMenuName   = 0;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(NULL, IDI_APPLICATION);
    if(!RegisterClassEx(&wcex)) { MessageBox(hWnd, "Register class error", "Error", IDI_ERROR || MB_OK); return 1; }
    //hWnd = CreateWindow(szWindowClass, szTitle, (WS_OVERLAPPEDWINDOW | WS_SYSMENU) & ~(WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME), 0, 0,  ENGINE::W + GetSystemMetrics(SM_CXFIXEDFRAME)*2,ENGINE::H + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFIXEDFRAME)*2,  NULL, NULL, hInstance, NULL);
    hWnd = CreateWindow(szWindowClass, szTitle, WS_POPUP  & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE), CW_USEDEFAULT, 0, ENGINE::W, ENGINE::H, NULL, NULL, hInstance, NULL);

    if(!hWnd) { MessageBox(hWnd, "Error create window", "Error", IDI_ERROR || MB_OK);   return 1; }
    ShowWindow(hWnd, nCmdShow);

    SetWindowPos(hWnd, HWND_TOPMOST, (GetSystemMetrics(SM_CXSCREEN)-ENGINE::W)>>1, (GetSystemMetrics(SM_CYSCREEN)-ENGINE::H)>>1, ENGINE::W, ENGINE::H, SWP_FRAMECHANGED);

    hdc = GetDC(hWnd);
    ENGINE::Init();
    if (hbrBackground) {
        BITMAPINFO bmiInfo;
        GetObject(hbrBackground, sizeof(bmiInfo), &bmiInfo);
        HDC back_hdc = CreateCompatibleDC(hdc);
        SelectObject(back_hdc, hbrBackground);
        StretchBlt(hdc,0,0,bmiInfo.bmiHeader.biWidth,bmiInfo.bmiHeader.biHeight,back_hdc,0,0,bmiInfo.bmiHeader.biWidth,bmiInfo.bmiHeader.biHeight, SRCCOPY);
        DeleteDC(back_hdc);
        DeleteObject(hbrBackground);
    }
    ENGINE::FlushScanline();
    hThread = CreateThread(NULL, 0, &ThreadProc, &ENGINE::running, 0, &threadID);
    while(GetMessage(&msg, NULL, 0, 0) && !GetAsyncKeyState(VK_ESCAPE)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return msg.wParam;
}
