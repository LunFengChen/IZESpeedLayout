#include <windows.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "pvzclass.h"
const wchar_t CLASS_NAME[] = L"MagnetShroomTime";

float getFloatValue() {
    auto board = PVZ::GetBoard();
    if (!board) return 0.00;
    for (auto plant : board->GetAllPlants()) {
        if (plant->Type == PlantType::Magnetshroom && plant->AttributeCountdown > 0) {
            return plant->AttributeCountdown / 100.0;
        }
    }
    return 0.00;
}

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// 使用标准main函数
int main() {
    DWORD pid = ProcessOpener::Open();
    if (!pid) return 1;
    PVZ::InitPVZ(pid);
    HINSTANCE hInstance = GetModuleHandle(NULL);

    // 注册窗口类
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"窗口类注册失败!", L"错误", MB_ICONERROR);
        return 1;
    }

    // 创建窗口
    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"磁铁倒计时",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 100,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        MessageBoxW(NULL, L"窗口创建失败!", L"错误", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    // 消息循环
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;

    PVZ::QuitPVZ();
}

// 确保此实现与函数声明完全一致
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static UINT_PTR timerID;
    static float currentValue;

    switch (uMsg) {
    case WM_CREATE:
        timerID = SetTimer(hwnd, 1, 10, NULL);
        break;

    case WM_TIMER:
        currentValue = getFloatValue();
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        // 清空背景
        FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));

        // 转换浮点数为宽字符串
        std::wostringstream woss;
        woss << std::fixed << std::setprecision(2) << currentValue;
        std::wstring text = woss.str();

        // 居中绘制文本
        DrawTextW(hdc, text.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY:
        KillTimer(hwnd, timerID);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}