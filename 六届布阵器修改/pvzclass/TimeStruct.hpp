#pragma once

#include <string>
#include <cstddef>
#include <ctime>
#include <sstream>
#include <iomanip>

class TimeStruct {
public:
    std::size_t minute;
    std::size_t second;

    // 构造函数：通过分钟和秒数初始化
    TimeStruct(std::size_t _minute, std::size_t _second)
        : minute(_minute), second(_second) {
    }
    // 构造函数：通过总秒数初始化，自动转换为分钟和秒数
    TimeStruct(std::size_t _second) {
        minute = _second / 60;
        second = _second % 60;
    }
    // 以中文格式输出，如 "2 分 05 秒"
    std::string cnPrint() const {
        return std::to_string(minute) + " 分 " + std::to_string(second) + " 秒";
    }
    // 以英文格式输出，如 "02:05"
    std::string enPrint() const {
        std::string firstStr = (minute >= 10) ? std::to_string(minute) : "0" + std::to_string(minute);
        std::string secondStr = (second >= 10) ? std::to_string(second) : "0" + std::to_string(second);
        return firstStr + ":" + secondStr;
    }
    // 重载加法运算符，将两个时间对象相加
    TimeStruct operator+(const TimeStruct& ts) const {
        auto totalSeconds = this->minute * 60 + this->second + ts.minute * 60 + ts.second;
        return TimeStruct(totalSeconds);
    }

    // 重载减法运算符，计算两个时间对象的差值
    TimeStruct operator-(const TimeStruct& ts) const {
        auto totalSeconds = (this->minute * 60 + this->second) - (ts.minute * 60 + ts.second);
        return TimeStruct(totalSeconds);
    }
    // 重载相等运算符，比较两个时间对象是否相等
    bool operator==(const TimeStruct& ts) const {
        return (this->minute == ts.minute) && (this->second == ts.second);
    }
    // 静态方法：返回当前时间（以自1970年以来的秒数转换为 TimeStruct）
    static TimeStruct getNow() {
        return TimeStruct(static_cast<std::size_t>(std::time(nullptr)));
    }

    // 静态方法：返回当前时间的字符串，格式为 "HH:MM:SS"
    static std::string getCurrentTime() {
        auto t = std::time(nullptr);
        std::tm tm;
        localtime_s(&tm, &t);  // Windows 平台下使用 localtime_s
        std::ostringstream oss;
        oss << std::put_time(&tm, "%H:%M:%S");
        return oss.str();
    }
    //// 静态方法：返回当前时间的字符串，格式为 "YYYY_MM_DD_HH_MM_SS"
    //static std::string getCurrentDateTime() {
    //    auto t = std::time(nullptr);
    //    std::tm tm;
    //    localtime_s(&tm, &t);  // Windows 平台下使用 localtime_s
    //    std::ostringstream oss;
    //    oss << std::put_time(&tm, "%Y_%m_%d_%H_%M_%S");
    //    return oss.str();
    //};
}