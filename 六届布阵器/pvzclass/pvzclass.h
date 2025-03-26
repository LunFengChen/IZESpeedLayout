#pragma once
#include "ProcessOpener.h"
#include "PVZ.h"
#include "Creators.h"
#include "Extensions.h"
#include "utils.h"
#include "events.h"
#include "iMemory.hpp"
#include <array>
#include <iostream>
#include "events.h"
#include <random>
#include <sstream>
#include <WinUser.h>
#include <functional>
#include <locale.h>

// #define SLEEP_1
// types
class TimeStruct;
class LevelState;
enum class Theme : byte;
enum class SmallFeature : byte;

template<typename T>
using VoidLambda = std::function<void(PVZ*, std::shared_ptr<T>)>;

using SlotType = std::array<ZombieType::ZombieType, 10>;


class TimeStruct
{
public:
	std::size_t minute = 0;
	std::size_t second = 0;
	TimeStruct(std::size_t _minute, std::size_t _second) : minute(_minute), second(_second) {};
	TimeStruct(std::size_t _second);
	std::string cnPrint() const { return std::to_string(minute).append(" 分 ").append(std::to_string(second).append(" 秒")); };
	std::string enPrint() const 
	{ 
		std::string firstStr = minute >= 10 ? std::to_string(minute) : std::string("0").append(std::to_string(minute));
		std::string secondStr = second >= 10 ? std::to_string(second) : std::string("0").append(std::to_string(second));
		return firstStr.append(":").append(secondStr);
	};
	TimeStruct operator-(const TimeStruct& ts) const;
	TimeStruct operator+(const TimeStruct& ts) const;
	bool const operator==(const TimeStruct& ts) { return !(second - ts.second) && !(minute - ts.minute); };
	static TimeStruct getNow() { return TimeStruct(time(nullptr)); };
};

// 通过每小关时候的状态
class LevelState
{
public:
	int sunNo;
	int accomplishedFlagNo;
	SlotType zombieTypes;
	std::vector<SmallFeature> smallFeatures;
	std::array<size_t, 25> seeds;
	std::array<Theme, 25> themes;
	TimeStruct currentTime;

	LevelState(int sunNo, int accomplishedFlagNo, const SlotType& zombieTypes, const std::vector<SmallFeature>& smallFeatures, const std::array<size_t, 25>& seeds, const std::array<Theme, 25>& themes, const TimeStruct& currentTime)
		: sunNo(sunNo), accomplishedFlagNo(accomplishedFlagNo), zombieTypes(zombieTypes), smallFeatures(smallFeatures), seeds(seeds), themes(themes), currentTime(currentTime)
	{
	}

	LevelState(int sunNo, int accomplishedFlagNo, const SlotType& zombieTypes, const std::vector<SmallFeature>& smallFeatures, const std::array<size_t, 25>& seeds, const std::array<Theme, 25>& themes)
		: LevelState(sunNo, accomplishedFlagNo, zombieTypes, smallFeatures, seeds, themes, TimeStruct::getNow()) {};
};


enum class Theme : byte
{
	NIL = 0, // 占位符
	COMPOSITE, // 综合
	CONTROL, // 控制
	INSTANT_KILL, // 即死
	PEAS, // 输出
	STAR_AND_SPIKE, // 倾斜
	EXPLODING, // 爆炸
	MAGNAT_AND_FUME, // 穿刺
	SCARDY	// 胆小
};


constexpr uint32_t L_PLANT_START = 0x42b360;
constexpr uint32_t L_PLANT_FIRST = 0x6b1100;
constexpr uint32_t PLANT_MEMORY = 0x6b1200;
constexpr uint32_t L_PLANT_LOOP = 0x6b1130;
constexpr uint32_t L_PLANT_TEST = 0x6b11C0;
constexpr uint32_t L_PLANT_END = 0x6b11E0;
constexpr uint32_t PLANT_FLAG = 0x6b0905;

enum class SmallFeature : byte
{
	NIL = 0, // 常规模式
	MINI_ZOMBIE, // 小僵尸, 卡片阳光修改, 处理为所有僵尸放下时变小僵尸(while循环内遍历)
	AVOID_FLOWERS, // 避花, 改死亡条件和卡片阳光
	MIXING, // 缝合, 直接修改setLayout中getThemePlantTypes返回值即可, 拿原来对应这关的seed继续当种子生成
	TELEPORT, // 传送, 门的位置同上, 生成在setLayout中同样处理即可
	GARLICS, // 大蒜, 同上, 这个直接改倒数第一第二个小喷当大蒜
	LEFT_MOVE, // 左移, 不懂
	NO_SMALL_FEATURE, // 没有小特性但有大特性
};
// 不需要确保杂七杂八的随机数了, 所有整活, 布阵码只提供大小特性随机结果即可
// 非修改本体特性为1~4, 所有特性为1~6
// 大特性随机结果修改卡槽数组slotZombieTypes

// functions

// 从0开始, 即和右下角数字保持一致
inline int getFlag(PVZ* pvz) { return pvz->GetMiscellaneous()->Round; }

// 生成主题对应的二期植物顺序，顺序和游戏布阵相同
constexpr std::array<PlantType::PlantType, 17> getThemePlantTypes(Theme theme);

// 返回为{row, col}，起点0
std::pair<int, int> intToPos(int i);

// 整数格子坐标到浮点坐标, 返回x-y
inline std::pair<float, float> posToFloat(int row, int col) { return  { 80.0 * col, 40.0 + 100.0 * row }; };

// 整数格子坐标到浮点坐标, 返回x-y
std::pair<float, float> posToFloat(std::pair<int, int> pos) { return posToFloat(pos.first, pos.second); }

// 生成一个随机的, 合规的主题分布
std::array<Theme, 25> generateThemeDistribution();

// 确定植物分布数组
std::array<PlantType::PlantType, 25> getFlagPlantTypes(int flag, Theme theme);

// 生成一个合规的种子
size_t generateSeed(Theme theme);

// 根据种子随机顺序数组
std::array<int, 25> getShuffledArray(size_t seed);

// 种一关的植物
void setLayout(PVZ* pvz, int flag, Theme theme, size_t seed, const std::vector<SmallFeature>& smallFeatures);

// 遍历植物
void iterPlants(PVZ* pvz, VoidLambda<Plant> func, bool reverseOrder = false);

// 遍历僵尸
void iterZombies(PVZ* pvz, VoidLambda<Zombie> func, bool reverseOrder = false);

// 遍历场地物品
void iterGriditems(PVZ* pvz, VoidLambda<PVZ::Griditem> func, bool reverseOrder = false);

// 遍历钱袋
void iterCoins(PVZ* pvz, VoidLambda<PVZ::Coin> func, bool reverseOrder = false);

// 加密布阵码
std::string encodeLayoutString(const std::array<Theme, 25>& themes, const std::array<size_t, 25>& seeds);

// 生成一个带有整活特性的布阵码
bool getFeaturedLayoutString(std::string& originalLayoutString, const SlotType& zombies, const std::vector<SmallFeature>& smallFeatures);

// 解密布阵码
bool decodeLayoutString(std::string& layoutString, std::array<Theme, 25>& themes, std::array<size_t, 25>& seeds);

// 解密整活布阵码, 0常规 1整活 2输入错误
int decodeFeatureString(std::string& layoutString, SlotType& zombies, std::vector<SmallFeature>& smallFeatures);

// 生成一个布阵码
std::string generateLayoutString();

// 启动游戏
int startGame(const std::array<Theme, 25>& themes, const std::array<size_t, 25>& seeds, const std::vector<SmallFeature>& smallFeatures, const bool isCrashed = false);

// 设置剪贴板
void copyToClipBoard(const std::string& str);

// 找出吃了几个脑子
int countEatenBrain(PVZ* pvz);

// 大小整活特性更新函数
void featureUpdate(PVZ* pvz, const std::vector<SmallFeature>& smallFeatures, const SlotType& zombieTypes);

// 生成大特性僵尸
SlotType generateSlotTypes(std::array<std::vector<int>, 4> givenTypes = { { {}, {}, {}, {} } });

// 生成小特性
std::vector<SmallFeature> generateSmallFeatures(int count, bool hasSpecialFeatures, std::vector<SmallFeature> givenFeatures = {});

// 生成缝合特性植物分布
bool setMixingLayout(std::array<PlantType::PlantType, 25>& plantTypes, size_t seed, Theme theme);

// 处理传送特性
bool setTeleport(PVZ* pvz, size_t seed);

// 处理结束游戏时的操作
void exitGame(PVZ* pvz, const SlotType& zombieTypes, const std::vector<SmallFeature>& smallFeatures);

// 设置汇编注入器
void setInjectors(const SlotType& zombieTypes, const std::vector<SmallFeature>& smallFeatures);

// 大特性设置cli
void genSlotTypeCli(SlotType& zombies);

// 根据卡槽修改僵尸.
void setCards(SlotType& zombies, PVZ* pvz);

constexpr int getCardOrder(ZombieType::ZombieType zt);

void setPortal(int yellow1Row, int yellow1Column, int yellow2Row, int yellow2Column, int blue1Row, int blue1Column, int blue2Row, int blue2Column)
{
	PVZ::Memory::WriteMemory<int>(0x426FE9, yellow1Row);
	PVZ::Memory::WriteMemory<int>(0x426FE2, yellow1Column);
	PVZ::Memory::WriteMemory<int>(0x427014, yellow2Row);
	PVZ::Memory::WriteMemory<int>(0x42700D, yellow2Column);
	PVZ::Memory::WriteMemory<int>(0x427044, blue1Row);
	PVZ::Memory::WriteMemory<int>(0x42703D, blue1Column);
	//PVZ::Memory::WriteMemory<int>(0x42706D, blue2Row);
	//PVZ::Memory::WriteMemory<int>(0x427068, blue2Column);
}

// 崩溃处理.
void crashSolution();

// [0, BASE)范围内的数字转换为指定ascii码
char numberToChar(size_t i)
{
	if (i >= 0 && i <= 9) return i + '0';
	if (i <= 35) return i - 10 + 'A';
	return i - 8 + 'A';
}

// 指定ascii码转换为[0, BASE)范围内的数字
size_t charToNumber(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c <= 'Z') return c + 10 - 'A';
	return c + 8 - 'A';
}

// size_t范围内的整数指数幂
size_t stPow(size_t base, size_t power)
{
	size_t ret = 1;
	for (size_t i = 0; i < power; i++) ret *= base;
	return ret;
}

// 删除首地址在pEffect的particleSystem
void __removeEffect(uint32_t pEffect)
{
	SETARG(__asm__Effect__Remove, 1) = pEffect;
	PVZ::Memory::Execute(STRING(__asm__Effect__Remove));
	return;
}

