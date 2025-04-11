#include "../pvzclass/PVZ.h"
#include "../pvzclass/ProcessOpener.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

// 退出标志
std::atomic<bool> running(true);

// 子线程执行的禁女仆操作函数
void disableMaidFunction() {
    // 初始化状态
    int mj_clock = PVZ::Memory::ReadPointer(0x6a9ec0, 0x838);
    bool was_paused = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x164);

    while (running) { // 循环条件中检测退出标志
        bool currentPaused = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x164);

        if (!currentPaused && was_paused) {
            PVZ::Memory::WriteMemory<int>(PVZ::Memory::ReadMemory<int>(0x6a9ec0) + 0x838, mj_clock);
        }
        else if (currentPaused && !was_paused) {
            mj_clock = PVZ::Memory::ReadPointer(0x6a9ec0, 0x838);
        }

        was_paused = currentPaused;

        // 为了防止线程占用过高 CPU，可以加个短暂休眠
        //std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

//int main() {
//    DWORD pid = ProcessOpener::Open();
//    if (!pid) return 1;
//    PVZ::InitPVZ(pid);
//
//    // 开启子线程执行禁女仆操作
//    std::thread maidThread(disableMaidFunction);
//
//    std::cout << "已开启禁女仆功能，输入-1退出程序" << std::endl;
//
//    int input;
//    while (true) {
//        std::cin >> input;
//        if (input == -1) {
//            // 用户输入-1后退出循环
//            running = false;  // 通知子线程退出
//            break;
//        }
//    }
//
//    // 等待子线程安全退出
//    if (maidThread.joinable()) {
//        maidThread.join();
//    }
//
//    std::cout << "程序已退出" << std::endl;
//
//    PVZ::QuitPVZ();
//    return 0;
//}
