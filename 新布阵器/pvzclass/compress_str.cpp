#include <string>
#include <vector>
#include <cstdint>
#include <iostream>

class UltraCompressor {
private:
    const std::string charset =
        "!#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_`"
        "abcdefghijklmnopqrstuvwxyz{|}~"; // 91个可打印ASCII字符
    uint64_t key;

    // 流式加密算法（可逆）
    void cryptoTransform(uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            data[i] ^= static_cast<uint8_t>(key >> ((i % 8) * 8));
        }
    }

public:
    UltraCompressor(uint64_t secretKey = 0xCAFEBABEDEADBEEF) : key(secretKey) {}

    std::string compressEncrypt(uint64_t number) {
        std::vector<uint8_t> bytes;
        // 将64位数字转换为字节流
        for (int i = 0; i < 8; ++i) {
            bytes.push_back(static_cast<uint8_t>(number >> (56 - i * 8)));
        }

        // 加密字节流
        cryptoTransform(bytes.data(), bytes.size());

        // Base91编码
        std::string result;
        uint32_t buffer = 0;
        int bits = 0;
        const int base = 91;

        for (uint8_t byte : bytes) {
            buffer = (buffer << 8) | byte;
            bits += 8;
            while (bits >= 13) { // 13 bits可表示91^5范围
                bits -= 13;
                uint32_t val = (buffer >> bits) & 0x1FFF;
                result += charset[val % base];
                result += charset[val / base];
            }
        }

        if (bits > 0) {
            uint32_t val = buffer << (13 - bits);
            result += charset[val % base];
            result += charset[val / base];
        }

        return result.substr(0, 5); // 强制截断为5字符（可能损失精度）
    }

    uint64_t decryptDecompress(const std::string& str) {
        std::vector<uint8_t> bytes;
        uint32_t buffer = 0;
        int bits = 0;
        const int base = 91;

        // Base91解码
        for (size_t i = 0; i < str.size(); i += 2) {
            if (i + 1 >= str.size()) break;
            uint32_t val = charset.find(str[i])
                + base * charset.find(str[i + 1]);
            buffer = (buffer << 13) | val;
            bits += 13;
            while (bits >= 8) {
                bits -= 8;
                bytes.push_back(static_cast<uint8_t>(buffer >> bits));
            }
        }

        // 解密字节流
        cryptoTransform(bytes.data(), bytes.size());

        // 还原64位数字
        uint64_t num = 0;
        for (int i = 0; i < 8; ++i) {
            if (i < bytes.size()) {
                num = (num << 8) | bytes[i];
            }
            else {
                num <<= 8;
            }
        }
        return num;
    }
};

//int main() {
//    UltraCompressor uc;
//
//    // 安全范围输入（32位）
//    uint64_t num = 12625226327801356754; // 32位最大值
//    std::string s = uc.compressEncrypt(num); // 输出类似 "8f$qZ" (5字符)
//    std::cout<< s << std::endl;
//
//    //// 64位大数（自动扩展长度）
//    //uint64_t bigNum = 18446744073709551615ULL;
//    //std::string s2 = uc.compressEncrypt(bigNum); // 输出类似 "p~kLt^m7" (8字符)
//    //std::cout << s2 << std::endl;
//
//}