#include <iostream>
#include <ctime>

// 必须先包含Winsock2头文件并定义版本宏
#define _WINSOCKAPI_
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")


#include"EncryptUtils.h"
// 布阵器描述
constexpr wchar_t WINDOW_NAME[] = L"IZE竞速布阵器-加密布阵码加密工具";



time_t GetNetworkTime() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return 0;
    }

    // 候选NTP服务器列表（可根据需要增减）
    const char* ntpServers[] = {
        //"ntp.ntsc.ac.cn",    // 原服务器
        "cn.ntp.org.cn",     // 中国NTP快速服务
        "edu.ntp.org.cn",     
        "ntp1.nim.ac.cn",   
        "ntp2.nim.ac.cn",
        "cn.pool.ntp.org",
        "ntp.aliyun.com",    
        "time.windows.com",  // 微软NTP
        "pool.ntp.org"       // 国际NTP池
    };

    time_t result = 0;

    for (const char* server : ntpServers) {
        struct addrinfo hints, * res = nullptr;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;

        // ======== 尝试解析当前服务器 ========
        if (getaddrinfo(server, "123", &hints, &res) != 0) {
            std::cerr << "[" << server << "] 解析失败，尝试下一服务器..." << std::endl;
            continue;
        }

        // ======== 创建套接字 ========
        SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCKET) {
            std::cerr << "[" << server << "] 套接字创建失败: " << WSAGetLastError() << std::endl;
            freeaddrinfo(res);
            continue;
        }

        // ======== 设置双超时 ========
        DWORD timeout = 1500; // 缩短超时到1.5秒
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

        // ======== 发送NTP请求 ========
        char ntpPacket[48] = { 0 };
        ntpPacket[0] = 0x1B;  // NTPv3客户端模式

        sockaddr_in* serverAddr = (sockaddr_in*)res->ai_addr;
        if (sendto(sock, ntpPacket, sizeof(ntpPacket), 0,
            (sockaddr*)serverAddr, res->ai_addrlen) == SOCKET_ERROR) {
            std::cerr << "[" << server << "] 发送失败: " << WSAGetLastError() << std::endl;
            freeaddrinfo(res);
            closesocket(sock);
            continue;
        }

        // ======== 接收响应 ========
        char buffer[48];
        int bytesReceived = recvfrom(sock, buffer, sizeof(buffer), 0, nullptr, nullptr);

        if (bytesReceived >= 40) {
            // 成功获取有效响应
            uint32_t secs = ntohl(*reinterpret_cast<uint32_t*>(buffer + 40));
            const uint32_t ntpToUnix = 2208988800UL;
            result = secs - ntpToUnix;

            std::cout << "成功从服务器 [" << server << "] 获取时间" << std::endl;
            freeaddrinfo(res);
            closesocket(sock);
            WSACleanup();
            return result;
        }

        // ======== 清理当前服务器资源 ========
        freeaddrinfo(res);
        closesocket(sock);
        std::cerr << "[" << server << "] 无有效响应，尝试下一服务器..." << std::endl;
    }

    WSACleanup();
    std::cerr << "所有服务器尝试失败" << std::endl;
    return 0;
}


// 时间转换函数保持不变
void UnixTimeToSystemTime(time_t unixTime, SYSTEMTIME& st) {
    struct tm timeInfo;
    gmtime_s(&timeInfo, &unixTime);
    st.wYear = timeInfo.tm_year + 1900;
    st.wMonth = timeInfo.tm_mon + 1;
    st.wDay = timeInfo.tm_mday;
    st.wHour = timeInfo.tm_hour;
    st.wMinute = timeInfo.tm_min;
    st.wSecond = timeInfo.tm_sec;
    st.wMilliseconds = 0;
}


bool update_internet_time() {
    time_t correct_time = GetNetworkTime();
    if (correct_time == 0) {
        std::cerr << "Failed to get network time" << std::endl;
        return false;
    }
    SYSTEMTIME st;
    UnixTimeToSystemTime(correct_time, st);
    if (!SetSystemTime(&st)) {
        std::cerr << "SetSystemTime failed: " << GetLastError() << std::endl;
        return false;
    }
    std::cout << "System time updated successfully! (UTC)" << std::endl;
    return true;
}


// 把字符串丢进剪切板
void copyToClipBoard(const std::string& str)
{
    auto hGlobalMemorry = GlobalAlloc(GPTR, static_cast<DWORD>(str.length()) + 1);
    auto hWnd = FindWindow(NULL, WINDOW_NAME);
    if (OpenClipboard(hWnd))
    {
        EmptyClipboard();
        strcpy_s(static_cast<char*>(hGlobalMemorry), str.length() + 1, str.c_str());
        SetClipboardData(CF_TEXT, hGlobalMemorry);
        CloseClipboard();
        return;
    }
    else return;
}


/*int main() {
    while (true)
    {
        std::string enc_ls;
        std::cout << "请输入加密布阵码：" << std::endl;
        std::cin >> enc_ls;
        std::vector<std::array<std::string, 3>> machine_code_info;
        std::string ls;
        if (!EncryptUtils::decode_ls(enc_ls, machine_code_info, ls)) {
            std::cout << "解密失败, 请重新尝试!" << std::endl;
            continue;
        }
        std::size_t ts_end = 0;
        for (auto& player_info : machine_code_info) {
            // 解析开始时间戳
            try { // 找到最晚的时间戳
                if (std::stoull(player_info[2]) > ts_end) ts_end = std::stoull(player_info[2]);
            }
            catch (...) {
                std::cout << "起始时间错误!" << std::endl;
                continue;
            }
        }
        // 获取最新时间
        if (!update_internet_time()) {
            std::cout << "请检查网络!" << std::endl;
            continue;
        }
        else {
            std::cout << "已联网更新时间! " << std::endl;
        }
        // 获取当前时间戳（秒为单位）
        auto now = std::chrono::system_clock::now();
        auto current_ts = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count();
        // 40min后才可解禁
        if (ts_end != 0 && current_ts > ts_end + 40 * 60) {
            std::cout << ls << std::endl;
            // 把字符串丢到剪切板
            copyToClipBoard(ls);
            std::cout << "满足解密时间要求! 已解密布阵码, 并存入剪切板!" << std::endl;
            SetConsoleTitle(WINDOW_NAME);
            continue;
        }
        else {
            std::cerr << "起始时间错误!必须等到最后选手布阵有效期后40min才可解密!" << std::endl;
            continue;
        }
        return 0;
    }
}*/