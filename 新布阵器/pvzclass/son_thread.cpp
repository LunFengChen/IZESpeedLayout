#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <atomic>

// 全局同步变量
std::mutex mtx;
std::condition_variable cv;
std::queue<int> result_queue;
std::atomic<bool> stop_flag(false);




// 反作弊检测
void worker_function() {
    for (int i = 0; i < 5; ++i) {
        // 检测操作
        std::this_thread::sleep_for(std::chrono::seconds(1));
        // 记录检测日志
        
        // 将结果放入队列并通知主线程
        {
            std::lock_guard<std::mutex> lock(mtx);
            result_queue.push(i);
            std::cout << "Worker: 生成结果 " << i << std::endl;
        }
        cv.notify_one();
    }

    // 任务完成，设置停止标志
    stop_flag = true;
    cv.notify_one();
}

//int main() {
//    // 启动工作线程
//    std::thread worker(worker_function);
//    worker.detach();  // 分离线程，让其独立运行
//
//    // 主线程处理结果
//    while (true) {
//        std::unique_lock<std::mutex> lock(mtx);
//
//        // 等待条件满足：队列非空或任务结束
//        cv.wait(lock, [] { return !result_queue.empty() || stop_flag; });
//
//        // 处理所有当前结果
//        while (!result_queue.empty()) {
//            int result = result_queue.front();
//            result_queue.pop();
//            std::cout << "Main: 处理结果 " << result << std::endl;
//        }
//
//        // 检查停止条件
//        if (stop_flag && result_queue.empty()) {
//            std::cout << "Main: 所有任务已完成" << std::endl;
//            break;
//        }
//
//        lock.unlock();
//    }
//
//    return 0;
//}