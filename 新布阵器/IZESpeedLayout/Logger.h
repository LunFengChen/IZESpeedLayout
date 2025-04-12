#pragma once

#include <string>
#include <fstream>
#include <iostream>

// 注意：需要包含 windows.h 用于 GetModuleFileNameA、CreateDirectoryA、MAX_PATH 等宏和函数
#include <windows.h>

class Logger {
public:
    enum LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERR,
    };
    bool encryptLogs;
    std::string logFilePath;


    // 构造函数，指定日志文件名和日志级别（默认 INFO）
    Logger(const std::string& filename, LogLevel level = INFO, bool encrypt = false);
    ~Logger();
    static std::string calc_hash(std::string log_file_path);

    void setLogLevel(LogLevel level);
    // 线程安全的非阻塞输出函数
    void safe_print(const std::string& msg);

    void log(const std::string& message, LogLevel level);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);

private:
    // 获取当前时间字符串，格式为 "YYYY-MM-DD HH:MM:SS"
    std::string getCurrentTime();

    LogLevel logLevel;
    std::ofstream logFile;
};
