#include <windows.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "pvzclass.h"

const wchar_t CLASS_NAME[] = L"CountdownTimer";



























class CountdownTimer {
public:
    CountdownTimer() : hwnd(NULL), timerID(0), magnetCountdown(0.0), countdown(30 * 60 * 1000), pid(0) {  // 30 min in milliseconds
    }

    void StartWindow() {
        HINSTANCE hInstance = GetModuleHandle(NULL);

        // 注册窗口类
        WNDCLASSW wc = {};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

        if (!RegisterClassW(&wc)) {
            MessageBoxW(NULL, L"窗口类注册失败!", L"错误", MB_ICONERROR);
            return;
        }

        // 创建窗口
        hwnd = CreateWindowExW(
            0,
            CLASS_NAME,
            L"IZE竞速模式倒计时",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 300, 100,
            NULL, NULL, hInstance, NULL);

        if (!hwnd) {
            MessageBoxW(NULL, L"窗口创建失败!", L"错误", MB_ICONERROR);
            return;
        }

        ShowWindow(hwnd, SW_SHOWNORMAL);
        UpdateWindow(hwnd);

        // 启动定时器
        timerID = SetTimer(hwnd, 1, 10, NULL);  // 每1ms更新数据
        // 消息循环
        MSG msg;
        while (GetMessageW(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    void StopWindow() {
        if (hwnd) {
            KillTimer(hwnd, timerID);
            PostQuitMessage(0);
        }
    }

    void UpdateCountdown() {
        // 每1ms更新倒计时
        timeLeft -= 1;  // 1ms倒计时
        if (timeLeft < 0) timeLeft = 0;  // 防止倒计时小于0
    }

    void UpdateMagnetCountdown() {
        magnetCountdown = getFloatValue();
    }

    std::wstring GetCountdownString(size_t start_time) {
        // 获取当前时间戳
        auto now = std::chrono::system_clock::now();
        auto now_time = std::chrono::system_clock::to_time_t(now);

        // 计算经过的时间（单位：秒）
        size_t elapsed_time = static_cast<size_t>(now_time) - start_time;

        // 计算剩余时间（单位：秒）
        size_t remaining_time = (countdown / 1000) - elapsed_time;

        if (remaining_time < 0) remaining_time = 0; // 防止负数

        int minutes = remaining_time / 60;
        int seconds = remaining_time % 60;

        std::wostringstream woss;
        woss << std::setw(2) << std::setfill(L'0') << minutes << L":"
            << std::setw(2) << std::setfill(L'0') << seconds;
        return woss.str();
    }

    // 获取磁铁倒计时的字符串
    std::wstring GetMagnetCountdownString() {
        std::wostringstream woss;
        woss << std::fixed << std::setprecision(1) << getFloatValue();
        return woss.str();
    }

private:
    HWND hwnd;
    UINT_PTR timerID;
    float magnetCountdown;
    DWORD countdown;
    DWORD pid;
    DWORD timeLeft;  // 30分钟倒计时（单位：毫秒）

    // 获取磁铁倒计时的值
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

    // 确保此实现与函数声明完全一致
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        static CountdownTimer* timer = nullptr;

        switch (uMsg) {
        case WM_CREATE:
            timer = reinterpret_cast<CountdownTimer*>(lParam);
            break;

        case WM_TIMER:
            if (timer) {
                timer->UpdateCountdown();  // 更新30分钟倒计时
                timer->UpdateMagnetCountdown();  // 更新磁铁倒计时
                InvalidateRect(hwnd, NULL, TRUE);  // 重绘窗口
            }
            break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);

            // 清空背景
            FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));

            // 获取倒计时和磁铁倒计时的字符串
            std::wstring countdownStr = timer->GetCountdownString(1743570420);
            std::wstring magnetStr = timer->GetMagnetCountdownString();

            // 居中绘制文本
            std::wstring text = L"倒计时: " + countdownStr + L" 磁铁倒计时: " + magnetStr;
            DrawTextW(hdc, text.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            EndPaint(hwnd, &ps);
            break;
        }

        case WM_DESTROY:
            if (timer) {
                timer->StopWindow();
            }
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
        return 0;
    }
};

//int main() {
//    CountdownTimer timer;
//    timer.StartWindow();  // 启动窗口并开始定时器
//    return 0;
//}
