#include <windows.h>
#include <iomanip>
#include <sstream>
#include "pvzclass.h"

const wchar_t CLASS_NAME[] = L"CountdownTimer";

class CountdownTimer {
public:
    CountdownTimer()
        : hwnd(NULL), timerID(0), time_countdown(30*60), magnetShroom_time(0.0f) {
    }

    // 更新倒计时数据
    void UpdateData(size_t time_countdown, float magnetShroom_time) {
        this->time_countdown = time_countdown;
        this->magnetShroom_time = magnetShroom_time;
    }

    size_t time_countdown;
    float magnetShroom_time;

private:
    HWND hwnd;
    UINT_PTR timerID;


};


//int main() {
//    CountdownTimer timer;
//
//    // 主循环负责定期更新数据
//    for (size_t i = 30 * 60; i > 0; --i) {
//        // 更新数据
//        timer.UpdateData(i, 0.00f);
//        std::cout << timer.time_countdown << " " << timer.magnetShroom_time << std::endl;
//        Sleep(1000);
//    }
//
//    return 0;
//}
