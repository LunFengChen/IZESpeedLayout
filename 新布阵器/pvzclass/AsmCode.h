#pragma once
#include <Windows.h>
#include <initializer_list>

/**
 * @brief 汇编代码生成与注入工具类
 */
class AsmCode {
public:
    // 寄存器枚举
    enum class Reg : unsigned int {
        EAX = 0,
        EBX,
        ECX,
        EDX,
        ESI,
        EDI,
        EBP,
        ESP,
    };

    // 构造函数与析构函数
    AsmCode();
    ~AsmCode();

    // 基础操作
    void asm_init();
    void asm_add_byte(unsigned char value);
    void asm_add_word(unsigned short value);
    void asm_add_dword(unsigned int value);
    void asm_add_list(std::initializer_list<unsigned char> value);

    // 汇编指令生成
    void asm_push(int value);
    void asm_mov_exx(Reg reg, int value);
    void asm_add_exx(Reg reg, int value);
    void asm_mov_exx_dword_ptr(Reg reg, int value);
    void asm_mov_exx_dword_ptr_exx_add(Reg reg, int value);
    void asm_push_exx(Reg reg);
    void asm_pop_exx(Reg reg);
    void asm_call(int addr);
    void asm_ret();

    // 代码注入功能
    void asm_code_inject(HANDLE handle);

    // 模板方法
    template <typename... Args>
    void asm_add_list(Args... value);

protected:
    unsigned char* code;    // 生成的机器码缓冲区
    unsigned int length;    // 代码长度
};

// 模板方法实现（需放在头文件中）
template <typename... Args>
void AsmCode::asm_add_list(Args... value) {
    asm_add_list({ static_cast<unsigned char>(value)... });
}

