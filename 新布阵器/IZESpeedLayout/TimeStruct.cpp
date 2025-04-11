#include "TimeStruct.h"
#include <ctime>
#include <sstream>
#include <iomanip>

// 构造函数：通过分钟和秒数初始化
TimeStruct::TimeStruct(std::size_t _minute, std::size_t _second)
    : minute(_minute), second(_second) {
}

// 构造函数：通过总秒数初始化，自动转换为分钟和秒数
TimeStruct::TimeStruct(std::size_t _second) {
    minute = _second / 60;
    second = _second % 60;
}

std::string TimeStruct::cnPrint() const {
    return std::to_string(minute) + " 分 " + std::to_string(second) + " 秒";
}

std::string TimeStruct::enPrint() const {
    std::string firstStr = (minute >= 10) ? std::to_string(minute) : "0" + std::to_string(minute);
    std::string secondStr = (second >= 10) ? std::to_string(second) : "0" + std::to_string(second);
    return firstStr + ":" + secondStr;
}

TimeStruct TimeStruct::operator+(const TimeStruct& ts) const {
    auto totalSeconds = this->minute * 60 + this->second + ts.minute * 60 + ts.second;
    return TimeStruct(totalSeconds);
}

TimeStruct TimeStruct::operator-(const TimeStruct& ts) const {
    auto totalSeconds = (this->minute * 60 + this->second) - (ts.minute * 60 + ts.second);
    return TimeStruct(totalSeconds);
}

bool TimeStruct::operator==(const TimeStruct& ts) const {
    return (this->minute == ts.minute) && (this->second == ts.second);
}

TimeStruct TimeStruct::getNow() {
    return TimeStruct(static_cast<std::size_t>(std::time(nullptr)));
}

std::string TimeStruct::getCurrentTime() {
    auto t = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &t);  // Windows 平台下使用 localtime_s
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

std::string TimeStruct::getCurrentDateTime() {
    auto t = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &t);  // Windows 平台下使用 localtime_s
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y_%m_%d_%H_%M_%S");
    return oss.str();
}

// 解析时间字符串，成功则返回 true 并通过 outTimestamp 输出时间戳，失败则返回 false
bool TimeStruct::parseTimestamp(const std::string& timeStr, std::time_t& outTimestamp) {
    std::tm tm = {};
    std::istringstream ss(timeStr);
    // 注意这里的格式和你输入时用的分隔符要一致
    ss >> std::get_time(&tm, "%Y-%m-%d-%H-%M-%S");
    if (ss.fail()) {
        return false;
    }
    // mktime 假定 tm 是本地时间
    std::time_t t = std::mktime(&tm);
    if (t == -1) {
        return false;
    }
    outTimestamp = t;
    return true;
}
