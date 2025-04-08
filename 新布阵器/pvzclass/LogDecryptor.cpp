#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include "EncryptUtils.h"

// 获取当前 exe 所在目录
std::string getExeDirectory() {
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH) == 0) {
        throw std::runtime_error("无法获取 exe 目录");
    }
    std::string exeDir = exePath;
    size_t lastSlash = exeDir.find_last_of("\\/");
    return (lastSlash != std::string::npos) ? exeDir.substr(0, lastSlash) : exeDir;
}

// 读取加密日志文件并解密，同时写入 "游戏数据解密.log"
void decryptLogFile(const std::string& encryptedLogFile, const std::string& outputFile) {
    std::ifstream logFile(encryptedLogFile);
    if (!logFile.is_open()) {
        std::cerr << "无法打开日志文件: " << encryptedLogFile << std::endl;
        return;
    }

    std::ofstream decryptedFile(outputFile, std::ios::out);
    if (!decryptedFile.is_open()) {
        std::cerr << "无法创建解密文件: " << outputFile << std::endl;
        return;
    }
    
    std::string encryptedLine;
    while (std::getline(logFile, encryptedLine)) {
        try {
            std::string decryptedLine = EncryptUtils::aes128ECBDecrypt(
                encryptedLine, EncryptUtils::sha256(EncryptUtils::LOG_KEY)
            );
            //std::cout << decryptedLine << std::endl;
            decryptedFile << decryptedLine << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "解密失败: " << e.what() << " | 原始行: " << encryptedLine << std::endl;
        }
    }

    logFile.close();
    decryptedFile.close();

    std::cout << "解密完成，日志已保存到: " << outputFile << std::endl;
}

//int main() {
//    try {
//        std::string encryptedFile;
//        std::cout << "请输入加密日志文件路径: ";
//        std::cin >> encryptedFile;
//
//        std::string exeDir = getExeDirectory();
//        std::string outputLog = exeDir + "\\游戏数据解密.log";
//
//        decryptLogFile(encryptedFile, outputLog);
//    }
//    catch (const std::exception& e) {
//        std::cerr << "错误: " << e.what() << std::endl;
//        return 1;
//    }
//
//    return 0;
//}
