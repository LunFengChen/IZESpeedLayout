#pragma once

#include <string>
#include <cstddef>

class TimeStruct {
public:
    std::size_t minute;
    std::size_t second;

    // 构造函数：通过分钟和秒数初始化
    TimeStruct(std::size_t _minute, std::size_t _second);

    // 构造函数：通过总秒数初始化，自动转换为分钟和秒数
    TimeStruct(std::size_t _second);

    // 以中文格式输出，如 "2 分 05 秒"
    std::string cnPrint() const;

    // 以英文格式输出，如 "02:05"
    std::string enPrint() const;

    // 重载加法运算符，将两个时间对象相加
    TimeStruct operator+(const TimeStruct& ts) const;

    // 重载减法运算符，计算两个时间对象的差值
    TimeStruct operator-(const TimeStruct& ts) const;

    // 重载相等运算符，比较两个时间对象是否相等
    bool operator==(const TimeStruct& ts) const;

    // 静态方法：返回当前时间（以自1970年以来的秒数转换为 TimeStruct）
    static TimeStruct getNow();

    // 静态方法：返回当前时间的字符串，格式为 "HH:MM:SS"
    static std::string getCurrentTime();

    // 静态方法：返回当前时间的字符串，格式为 "YYYY_MM_DD_HH_MM_SS"
    static std::string getCurrentDateTime();
};
