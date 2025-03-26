// RecorderException.h
#pragma once
#include <stdexcept>
#include <string>

// 错误类型枚举
enum class RecorderError {
    UnknownError,           // 未知错误
    InitFailed,             // 初始化失败
    CaptureFailed,          // 捕获帧失败
    EncodeFailed,           // 编码失败
    FileWriteFailed,        // 文件写入失败
    InvalidWindowHandle,    // 无效窗口句柄
    FFmpegError             // FFmpeg 内部错误
};

class RecorderException : public std::runtime_error {
public:
    RecorderException(RecorderError code, const std::string& msg)
        : std::runtime_error(msg), m_errorCode(code) {}

    // 获取错误码
    RecorderError errorCode() const noexcept { return m_errorCode; }

    // 获取完整错误描述
    virtual const char* what() const noexcept override {
        return formatMessage().c_str();
    }

private:
    std::string formatMessage() const {
        return "[错误码: " + std::to_string(static_cast<int>(m_errorCode)) + "] "
            + std::runtime_error::what();
    }

    RecorderError m_errorCode;
};