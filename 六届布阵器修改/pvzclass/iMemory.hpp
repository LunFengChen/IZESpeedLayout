#pragma once
#include "PVZ.h"
#include <initializer_list>
#include <vector>
#include <deque>
#ifdef JNE
#undef JNE
#endif
enum class REG : byte { EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI };

using iPTR = uint32_t;

inline byte toByte(REG r) { return static_cast<byte>(r); }

class Injector
{
private:
	uint32_t baseAddr; // 被写的初始地址
	std::vector<byte> originalCodes = {}; // 被写处的初始内存
	std::vector<byte> codes = {}; // 被写处的内存
	size_t size = 0; // 被写的大小.
	bool isInEffect = false;

	// 读出baseAddr开始大小为vec.size()的字节, 存入vec中.
	void readMemory(std::vector<byte>& vec);

	// 将vec的size字节写入baseAddr处.
	void writeMemory(const std::vector<byte>& vec);

	// 把num视作字节数为size的数压入codes中
	template<size_t SIZE>
	Injector& add(size_t num, bool reverse = false);
public:
	
	// 读ptr + args处的内存, args为偏移
	template <typename T, typename... ARGS>
	static T readMemory(uint32_t ptr, const ARGS&... args);

	// 往ptr+args处写入value, args为偏移
	template<typename T, typename... ARGS>
	static void writeMemory(T value, uint32_t ptr, const ARGS&... args);

	Injector(uint32_t _baseAddr) : baseAddr(_baseAddr) {};

	Injector(uint32_t _baseAddr, const std::vector<byte>& _codes);

	~Injector() { invalid(); }

	// 注入内存, 返回此次注入是否有效
	bool effect();

	// 恢复修改的内存
	void invalid();

	// 清除内容
	void clear();

	// push c
	Injector& push(uint32_t c);

	// push r
	Injector& push(REG r) { return add<1>(0x50 + toByte(r)); }

	// pushad
	Injector& pushad() { return add<1>(0x60); }

	// pop r	
	Injector& pop(REG r) { return add<1>(0x58 + toByte(r)); }

	// popad
	Injector& popad() { return add<1>(0x61); }
	
	// mov r, r2
	Injector& mov(REG r, REG r2) { return add<1>(0x8b).add<1>(0xc0 + toByte(r) * 8 + toByte(r2)); }

	// mov r, [r2]
	Injector& movPtr(REG r, REG r2) { return add<1>(0x8b).add<1>(toByte(r) * 8 + toByte(r2)); }

	// mov r, [r2 + offset]
	Injector& movPtr(REG r, REG r2, int32_t offset);

	// mov [r], r2
	Injector& ptrMov(REG r, REG r2) { return add<1>(0x89).add<1>(toByte(r) + toByte(r2) * 8); }

	// mov [r + offset], r2
	Injector& ptrMov(REG r, int32_t offset, REG r2);

	// mov r, r2;
	Injector& mov(REG r, uint32_t c) { return add<1>(0xb8 + toByte(r)).add<4>(c); }

	// mov r, dword ptr[p]; p为指向T的指针	.
	Injector& movPtr(REG r, iPTR p);

	// mov dword ptr[p], r;
	Injector& ptrMov(iPTR p, REG r);

	// mov ptr[p], c; p指向的T为满足sizeof(T) == N的T*
	template <int N>
	Injector& ptrMov(iPTR p, size_t c);

	// mov ptr[p], c; p为指向T的指针, c为T对象
	template <typename T>
	Injector& ptrMov(iPTR p, T c) { return ptrMov<sizeof(T)>(p, static_cast<size_t>(c)); }

	// mov [r + offset], c
	Injector& ptrMov(REG r, int32_t offset, int32_t c);

	// mov byte ptr [r + offset], c
	Injector& bytePtrMov(REG r, int32_t offset, byte c);

	// mov edi c; call edi
	Injector& movCall(uint32_t addr) { return mov(REG::EDI, addr).add<2>(0xd7ff); }

	// call c;
	Injector& call(uint32_t addr) { return add<1>(0xe8).add<4>(addr - (size + baseAddr + 5)); }

	Injector& add(REG r, REG r2) { return add<1>(0x1), add<1>(0xc0 + toByte(r2) * 8 + toByte(r)); }

	Injector& add(REG r, uint32_t c);

	// add dword ptr[p], c
	Injector& ptrAdd(iPTR p, uint32_t c);

	Injector& sub(REG r, uint32_t c);

	// add [r] r2
	Injector& ptrAdd(REG r, REG r2) { return add<1>(0x01).add<1>(toByte(r) + toByte(r2) * 8); }

	// add [r + offset], r2
	Injector& ptrAdd(REG r, byte offset, REG r2) { return add<1>(0x01).add<1>(0x40 + toByte(r) + toByte(r2) * 8).add<1>(offset); }

	// inc dword ptr[p]
	Injector& inc(iPTR p) { return add<2>(0x05ff).add<4>(p); }

	// dec dword ptr[p]
	Injector& dec(iPTR p) { return add<2>(0x0dff).add<4>(p); }

	// mul r
	Injector& mul(REG r) { return add<1>(0xf7).add<1>(0xe0 + toByte(r)); }

	// div r
	Injector& div(REG r) { return add<1>(0xf7).add<1>(0xf0 + toByte(r)); }

	Injector& cmp(REG r, DWORD c);

	// cmp r, dword ptr[p]
	Injector& cmpPtr(REG r, iPTR p) { return add<1>(0x3b).add<1>(0x05 + toByte(r) * 8).add<4>(p); }

	// cmp ptr[p], c; p指向的T为满足sizeof(T) == N的T*
	template <int N>
	Injector& cmp(iPTR p, size_t c);

	// cmp dword ptr[p], c
	// p所指类型和c所属类型为T
	template <typename T>
	Injector& cmp(iPTR p, T c) { return cmp<sizeof(T)>(p, static_cast<size_t>(c)); }

	Injector& cmp(REG r, int32_t offset, uint32_t c);

	// cmp dword ptr[r], c
	Injector& ptrCmp(REG r, uint32_t c);

	// cmp byte ptr[r+offset], c
	Injector& bytePtrCmp(REG r, int32_t offset, byte c);

	// test r, r2
	Injector& test(REG r, REG r2) { return add<1>(0x85).add<1>(0xc0 + toByte(r) + toByte(r2) * 8); }

	Injector& repe() { return add<1>(0xf3); }

	Injector& stosd() { return add<1>(0xab); }

	uint32_t getOffset(uint32_t addr) { return addr - (size + baseAddr + 6); }

	Injector& je(uint32_t addr) { auto a = getOffset(addr);  return add<2>(0x0f84, true).add<4>(a); }

	Injector& jne(uint32_t addr) { auto a = getOffset(addr);  return add<2>(0x0f85, true).add<4>(a); }

	Injector& jb(uint32_t addr) { auto a = getOffset(addr);  return add<2>(0x0f82, true).add<4>(a); }

	Injector& jnb(uint32_t addr) { auto a = getOffset(addr);  return add<2>(0x0f83, true).add<4>(a); }
	
	Injector& ja(uint32_t addr) { auto a = getOffset(addr);  return add<2>(0x0f87, true).add<4>(a); }

	Injector& jna(uint32_t addr) { auto a = getOffset(addr);  return add<2>(0x0f86, true).add<4>(a); }

	Injector& jg(uint32_t addr) { auto a = getOffset(addr);  return add<2>(0x0f8f, true).add<4>(a); }

	Injector& jmp(uint32_t addr);

	Injector& nop() { return add<1>(0x90); }

	Injector& ret(int16_t c) { return add<1>(0xc2).add<2>(c); }

	// 浮点
	
	// fld dword ptr [r + offset]
	Injector& fld(REG r, int32_t offset);

	// fsub dword ptr [c]
	Injector& fsub(iPTR p) { return add<1>(0xd8).add<1>(0x25).add<4>(p); }

	// fstp dword ptr [r + offset]
	Injector& fstp(REG r, int32_t offset);

	template<typename T>
	Injector& addConst(const T& val);
};

template<size_t SIZE>
Injector& Injector::add(size_t num, bool reverse)
{
	const auto pByte = reinterpret_cast<const byte*>(&num);
	for (size_t i = 0; i < SIZE; i++)
	{
		int idx = reverse ? SIZE - 1 - i : i;
		codes.push_back(pByte[idx]);
	}
	size += SIZE;
	return *this;
}

template<typename T, typename ...ARGS>
T Injector::readMemory(uint32_t ptr, const ARGS & ...args)
{
	std::initializer_list<uint32_t>	addrs = { args... };
	for (auto it : addrs)
	{
		ReadProcessMemory(PVZ::Memory::hProcess, reinterpret_cast<LPCVOID>(ptr), &ptr, sizeof(ptr), NULL);
		ptr += it;
	}
	T ret;
	ReadProcessMemory(PVZ::Memory::hProcess, reinterpret_cast<LPVOID>(ptr), &ret, sizeof(T), NULL);
	return ret;
}

template<typename T, typename ...ARGS>
static void Injector::writeMemory(T value, uint32_t ptr, const ARGS & ...args)
{
	std::initializer_list<uint32_t> addrs = { args };
	for (auto it : addrs)
	{
		ReadProcessMemory(PVZ::Memory::hProcess, reinterpret_cast<LPCVOID>(ptr), &ptr, sizeof(ptr), NULL);
		ptr += it;
	}
	WriteProcessMemory(PVZ::Memory::hProcess, reinterpret_cast<LPVOID>(ptr), &value, sizeof(T), NULL);
}

template<int N>
inline Injector& Injector::ptrMov(iPTR p, size_t c)
{
	if constexpr (N == 4)
	{
		return add<2>(0x06c6).add<4>(p).add<4>(c);
	}
	if constexpr (N == 2)
	{
		return add<1>(0x66).add<2>(0x05c7).add<4>(p).add<2>(c);
	}
	if constexpr (N == 1)
	{
		return add<2>(0x05c6).add<4>(p).add<1>(c);
	}
}

template<int N>
inline Injector& Injector::cmp(iPTR p, size_t c)
{
	if constexpr (N == 4)
	{
		return add<2>(0x3d81).add<4>(p).add<4>(c);
	}
	if constexpr (N == 2)
	{
		return add<1>(0x66).add<2>(0x3d81).add<4>(p).add<2>(c);
	}
	if constexpr (N == 1)
	{
		return add<2>(0x3d80).add<4>(p).add<2>(c);
	}
}

template<typename T>
Injector& Injector::addConst(const T& val)
{
	const auto pByte = reinterpret_cast<const byte*>(&val);
	for (size_t i = 0; i < sizeof(T); i++)
	{
		codes.push_back(pByte[i]);
	}
	size += sizeof(T);
	return *this;
}

inline void Injector::readMemory(std::vector<byte>& vec)
{
	ReadProcessMemory(PVZ::Memory::hProcess, reinterpret_cast<LPCVOID>(baseAddr), vec.data(), vec.size(), NULL);
}

inline void Injector::writeMemory(const std::vector<byte>& vec)
{
	WriteProcessMemory(PVZ::Memory::hProcess, reinterpret_cast<LPVOID>(baseAddr), vec.data(), vec.size(), NULL);
}

Injector::Injector(uint32_t _baseAddr, const std::vector<byte>& _codes) : Injector(_baseAddr)
{
	this->codes = std::vector<byte>(_codes.begin(), _codes.end());
	this->size = _codes.size();
}

bool Injector::effect()
{
	if (isInEffect) return false;
	originalCodes = std::vector<byte>(size, 0);
	readMemory(originalCodes);
	writeMemory(codes);
	isInEffect = true;
	return true;
}

inline void Injector::invalid()
{
	if (!isInEffect) return;
	writeMemory(originalCodes);
	isInEffect = false;
}

inline void Injector::clear()
{
	codes = {};
	originalCodes = {};
	size = 0;
	isInEffect = false;
}

inline Injector& Injector::push(uint32_t c)
{
	if (static_cast<unsigned char>(c) == c) return add<1>(0x6a).add<1>(c); 
	return add<1>(0x68).add<4>(c);
}

Injector& Injector::movPtr(REG r, REG r2, int32_t offset)
{
	if (static_cast<signed char>(offset) == offset) return add<1>(0x8b).add<1>(0x40 + toByte(r) * 8 + toByte(r2)).add<1>(offset);
	else return add<1>(0x8b).add<1>(0x80 + toByte(r) * 8 + toByte(r2)).add<4>(offset);
}

Injector& Injector::ptrMov(REG r, int32_t offset, REG r2)
{
	if (static_cast<signed char>(offset) == offset) return add<1>(0x89).add<1>(toByte(r) + toByte(r2) * 8 + 0x40).add<1>(offset);
	else return add<1>(0x89).add<1>(toByte(r) + toByte(r2) * 8 + 0x80).add<4>(offset);
}

Injector& Injector::movPtr(REG r, iPTR p)
{
	if (r == REG::EAX) add<1>(0xa1);
	else add<1>(0x8b).add<1>(0x05 + toByte(r) * 8);
	return add<4>(p);
}

Injector& Injector::ptrMov(iPTR p, REG r)
{
	if (r == REG::EAX) add<1>(0xa3);
	else add<1>(0x89).add<1>(0x05 + toByte(r) * 8);
	return add<4>(p);
}

inline Injector& Injector::ptrMov(REG r, int32_t offset, int32_t c)
{
	if (static_cast<signed char>(offset) == offset) add<1>(0xc7).add<1>(0x40 + toByte(r)).add<1>(offset);
	else add<1>(0xc7).add<1>(0x80 + toByte(r)).add<4>(offset);
	return add<4>(c);
}

inline Injector& Injector::bytePtrMov(REG r, int32_t offset, byte c)
{
	if (static_cast<signed char>(offset) == offset) add<1>(0xc6).add<1>(0x40 + toByte(r)).add<1>(offset);
	else add<1>(0xc6).add<1>(0x80 + toByte(r)).add<4>(offset);
	return add<1>(c);
}

Injector& Injector::add(REG r, uint32_t c)
{
	if (static_cast<signed char>(c) == c) return add<1>(0x83).add<1>(0xc0 + toByte(r)).add<1>(c);
	if (r == REG::EAX) add<1>(0x05);
	else add<1>(0x81).add<1>(0xc0 + toByte(r));
	return add<4>(c);
}

Injector& Injector::ptrAdd(iPTR p, uint32_t c)
{
	if (static_cast<signed char>(c) == c) return add<2>(0x0583).add<4>(p).add<1>(c);
	else return add<2>(0x0581).add<4>(p).add<4>(c);
}

Injector& Injector::sub(REG r, uint32_t c)
{
	if (static_cast<signed char>(c) == c) return add<1>(0x83).add<1>(0xe8 + toByte(r)).add<1>(c);
	if (r == REG::EAX) add<1>(0x2d);
	else add<1>(0x81).add<1>(0xe8 + toByte(r));
	return add<4>(c);
}

Injector& Injector::cmp(REG r, DWORD c)
{
	if (r == REG::EAX) add<1>(0x3d);
	else add<1>(0x81).add<1>(0xf8 + toByte(r));
	return add<4>(c);
}

inline Injector& Injector::cmp(REG r, int32_t offset, uint32_t c)
{
	if (static_cast<unsigned char>(c) == c)
	{
		if (static_cast<signed char>(offset) == offset) return add<1>(0x83).add<1>(0x78 + toByte(r)).add<1>(offset).add<1>(c);
		return add<1>(0x83).add<1>(0xb8 + toByte(r)).add<4>(offset).add<1>(c);
	}
	else
	{
		if (static_cast<signed char>(offset) == offset) return add<1>(0x81).add<1>(0x78 + toByte(r)).add<1>(offset).add<4>(c);
		return add<1>(0x81).add<1>(0xb8 + toByte(r)).add<4>(offset).add<4>(c);
	}
}

inline Injector& Injector::ptrCmp(REG r, uint32_t c)
{
	if (static_cast<unsigned char>(c) == c) return add<1>(0x83).add<1>(0x38 + toByte(r)).add<1>(c);
	return add<1>(0x81).add<1>(0x38 + toByte(r)).add<4>(c);
}

inline Injector& Injector::bytePtrCmp(REG r, int32_t offset, byte c)
{
	if (static_cast<signed char>(offset) == offset) add<1>(0x80).add<1>(0x78 + toByte(r)).add<1>(offset);
	else add<1>(0x80).add<1>(0xb8 + toByte(r)).add<4>(offset);
	return add<1>(c);
}

Injector& Injector::jmp(uint32_t addr)
{
	uint32_t i = addr - (size + baseAddr + 5);
	if (static_cast<signed char>(addr) == addr)
	{
		addr += 3;
		return add<1>(0xeb).add<1>(addr);
	}
	else return add<1>(0xe9).add<4>(i);
}

Injector& Injector::fld(REG r, int32_t offset)
{
	if (static_cast<signed char>(offset) == offset) return add<1>(0xd9).add<1>(0x40 + toByte(r)).add<1>(offset);
	else return add<1>(0xd9).add<1>(0x80 + toByte(r)).add<4>(offset);
}

Injector& Injector::fstp(REG r, int32_t offset)
{
	if (static_cast<signed char>(offset) == offset) return add<1>(0xd9).add<1>(0x58 + toByte(r)).add<1>(offset);
	else return add<1>(0xd9).add<1>(0x98 + toByte(r)).add<4>(offset);
}