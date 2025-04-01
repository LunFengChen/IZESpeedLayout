#include "Logger.h"
#include <sstream>
#include <chrono>
#include <iomanip>
#include "EncryptUtils.h"


Logger::Logger(const std::string& filename, LogLevel level, bool encrypt)
    : logLevel(level), encryptLogs(encrypt)
{
    // 1. 获取 exe 所在目录
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH) == 0) {
        std::cerr << "Failed to get exe path!" << std::endl;
        return;
    }

    // 2. 提取 exe 所在目录（去除文件名部分）
    std::string exeDir(exePath);
    size_t lastSlash = exeDir.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        exeDir = exeDir.substr(0, lastSlash);
    }

    // 3. 拼接日志目录路径
    std::string logDir = exeDir + "\\IZESpeedLayoutDatas";

    // 4. 创建日志目录（如果不存在）
    if (!CreateDirectoryA(logDir.c_str(), NULL)) {
        DWORD error = GetLastError();
        if (error != ERROR_ALREADY_EXISTS) {
            std::cerr << "Failed to create log directory: " << logDir << std::endl;
            return;
        }
    }

    // 5. 拼接完整的日志文件路径
    std::string logFilePath = logDir + "\\" + filename;

    // 6. 打开日志文件（追加模式）
    logFile.open(logFilePath, std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: " << logFilePath << std::endl;
    }
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Logger::setLogLevel(LogLevel level) {
    logLevel = level;
}


// 线程安全的非阻塞输出函数
void Logger::safe_print(const std::string& msg) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written;
    WriteConsoleA(hConsole, msg.c_str(), msg.size(), &written, NULL);
}


void Logger::log(const std::string& message, LogLevel level) {
    if (level < logLevel) {
        return; // 如果日志级别低于设置的最低级别，则不记录任何日志
    }

    std::string levelStr;
    switch (level) {
    case DEBUG:   levelStr = "DEBUG"; break;
    case INFO:    levelStr = "INFO"; break;
    case WARNING: levelStr = "WARNING"; break;
    case ERR:     levelStr = "ERROR"; break;
    }

    // 生成完整的日志消息（包括时间戳和日志级别）
    std::string logMessage = getCurrentTime() + " [" + levelStr + "] " + message;

    // 如果加密，进行加密处理
    if (encryptLogs) {
        logMessage = EncryptUtils::aes128ECBEncrypt(logMessage, EncryptUtils::sha256(EncryptUtils::LOG_KEY));
    }


    // 控制台输出：只有 INFO 及以上级别才打印原始消息
    if (level >= INFO) {
        safe_print(message+"\n");
    }

    // 文件输出：记录完整的日志消息
    if (logFile.is_open()) {
        logFile << logMessage << std::endl;
    }
}

void Logger::debug(const std::string& message) {
    log(message, DEBUG);
}

void Logger::info(const std::string& message) {
    log(message, INFO);
}

void Logger::warning(const std::string& message) {
    log(message, WARNING);
}

void Logger::error(const std::string& message) {
    log(message, ERR);
}

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    tm local_time;
    localtime_s(&local_time, &in_time_t);  // 使用 localtime_s 获取线程安全的本地时间
    ss << std::put_time(&local_time, "%Y-%m-%d %X");
    return ss.str();
}
