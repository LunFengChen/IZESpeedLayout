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
#include <ctime>
#include <iomanip>
#include <direct.h>
#include <thread>
#include <chrono>
#include <fstream>

#include <unordered_set>
#include <unordered_map>


#include "Code.h"
#include "Encrypt.h"
//#include "TimeStruct.hpp"



constexpr std::array<const char*, 7> SMALL_FEATURE_NAMES = { "【迷你】", "【避花】", "【缝合】", "【传送】", "【大蒜】", "【左移】", "【无特性】" };
constexpr char SMALL_FEATURE_WORDS[] = "小特性列表: \n\
1. 【迷你】变为小僵尸,花费变为1/3向下取整 \n\
2. 【避花】放僵尸增加等额阳光,阳光达到25000时本局结束 \n\
3. 【缝合】每个非恢复关有一额外主题(除了胆小以外，7个主题概率一样) \n\
4. 【传送】每关在12列交界和56列交界随机位置开启一对传送门\n\
5. 【大蒜】每关将1-2个小喷替换为血量较低（100）的大蒜，其变向固定并显示在右侧 \n\
6. 【左移】左行矿工与其他僵尸会缓慢左移 \n\
*****************************************************";

constexpr char BIG_FEATURE_WORDS[] = "大特性:抽卡\n\
【小鬼】【普僵】2抽1\n\
【路障】【撑杆】【雪人】【旗帜】【报纸】5抽3\n\
【铁桶】【蹦极】【铁门】【小丑】4抽2\n\
【矿工】【扶梯】【橄榄】【跳跳】4抽3\n\
舞王不动\n\
新僵尸数据(未提及部分保持原版性质) :\n\
普通50阳光\n\
雪人75阳光，会且只会在食脑前转为右行\n\
摇旗75阳光，在放置时会在上下路额外放置一个普通(若在场地边缘则放置在本行)\n\
读报75阳光，狂暴时无视减速且啃食加快4倍\n\
铁门100阳光，饰品血量55->40\n\
小丑75阳光，只在12列交界(或更左)啃食植物或断手时爆炸\n\
跳跳125阳光，在01列交界弃杆";

constexpr char USING_INSTRUCTIONS[] = "请尽量使用shift+q关闭布阵器而非直接关闭布阵器!!!! \n\
请在布阵前重启游戏, 并且在进入ize后先重开关卡再输入布阵码! \n\
游戏中快捷键，在“开始游戏”后，最上方窗口为布阵器时使用：\n\
shift+r：重新开始本轮\n\
shift+p：暂停计时/继续计时\n\
shift+q：退出布阵模式\n\
崩溃回档，在“开始游戏”后游戏崩溃或误操作导致游戏关闭可使用：\n\
关闭游戏及Fatal Error窗口后，重新进入游戏、进入IZE关卡并回到布阵器按下s即可重新开始游戏，其中暂停计时。";

constexpr char INIT_WORDS[] = "\
欢迎使用手速杯布阵器 = v = \n\
作者: 小风 碳酸 天盟琉璃; 手速杯交流群：1157197563\n\
请在输入功能对应序号后按回车键：\n\
1：使用阵型代码生成阵型\n\
2：生成常规代码\n\
3：生成整活代码\n\
4：整活特性介绍\n\
5：使用说明\n\
0：退出";
constexpr bool extraFeatures = true;

constexpr wchar_t WINDOW_NAME[] = L"手速杯布阵器";





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
	static std::string getCurrentTime() {
		auto t = std::time(nullptr);
		std::tm tm;
		localtime_s(&tm, &t);  // Windows 平台下使用 localtime_s
		std::ostringstream oss;
		oss << std::put_time(&tm, "%H:%M:%S");
		return oss.str();
	}
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
	TimeStruct currentTime = TimeStruct::getNow();

	LevelState(int sunNo, int accomplishedFlagNo, const SlotType& zombieTypes, const std::vector<SmallFeature>& smallFeatures, const std::array<size_t, 25>& seeds, const std::array<Theme, 25>& themes, const TimeStruct& currentTime)
		: sunNo(sunNo), accomplishedFlagNo(accomplishedFlagNo), zombieTypes(zombieTypes), smallFeatures(smallFeatures), seeds(seeds), themes(themes), currentTime(currentTime)
	{
	}

	LevelState(int sunNo, int accomplishedFlagNo, const SlotType& zombieTypes, const std::vector<SmallFeature>& smallFeatures, const std::array<size_t, 25>& seeds, const std::array<Theme, 25>& themes)
		: LevelState(sunNo, accomplishedFlagNo, zombieTypes, smallFeatures, seeds, themes, TimeStruct::getNow()) {
	};
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


class ConsoleControl {
public:
	ConsoleControl();
	~ConsoleControl();

	static int get_terminal_width();
	static void copyToClipBoard(const std::string& str);
	void main();
};
class LayoutStringGenerator {
public:
	int BASE = 66;  // 假定定义，按实际修改
	// gen()、stPow()、pool、poolLimit、poolMax、slotZombieTypes、gen 等相关函数/变量需要在其它地方定义

	// 获取指定主题的植物类型列表，返回长度为 17 的数组
	static std::array<PlantType::PlantType, 17> getThemePlantTypes(Theme theme);

	// 设置第一阶段植物，flowerNumber 表示向数组前 flowerNumber 个位置设置为 Sunflower，其余位置设置为 Puffshroom（0～7位），返回 0
	static int setFirstPhasePlants(std::array<PlantType::PlantType, 25>& arr, int flowerNumber);

	// 根据 flag 值和主题获取旗帜关的植物类型，返回长度为 25 的数组
	static std::array<PlantType::PlantType, 25> getFlagPlantTypes(int flag, Theme theme);

	// 生成带有小特性布局字符串，传入僵尸卡槽和小特性列表，将生成的字符串追加到 originalLayoutString 上，返回 true 表示成功
	static bool getFeaturedLayoutString(std::string& originalLayoutString, const SlotType& zombies, const std::vector<SmallFeature>& smallFeatures);

	// 解析布局字符串，解析后的结果存入 zombies 和 smallFeatures，返回 1 表示解析成功，0 表示失败
	static int decodeFeatureString(std::string& layoutString, SlotType& zombies, std::vector<SmallFeature>& smallFeatures);

	// 将指定的 ascii 码转换为 [0, BASE) 范围内的数字
	static size_t charToNumber(char c);

	// 将 [0, BASE) 范围内的数字转换为对应的 ascii 字符
	static char numberToChar(size_t i);

	// 对每个 seed 进行 BASE 进制转换，将主题和 seed 编码为字符串，用 '.' 分隔
	static std::string encodeLayoutString(const std::array<Theme, 25>& themes, const std::array<size_t, 25>& seeds);

	// 获取打乱顺序的数组，数组大小为 25，使用传入 seed 作为随机种子
	static std::array<int, 25> getShuffledArray(size_t seed);

	// 将整型数 i 转换为棋盘位置（行、列），返回 pair{row, col}
	static inline std::pair<int, int> intToPos(int i);

	// 根据主题生成 seed，部分主题生成满足特定要求的 seed
	static size_t generateSeed(Theme theme);

	// 生成主题分布，返回长度为 25 的 Theme 数组
	static std::array<Theme, 25> generateThemeDistribution();

	// 生成最终的布局字符串
	static std::string generateLayoutString();

	// 解析布局字符串，返回解析后的主题分布和种子数组，成功时返回 true，失败返回 false
	static bool decode_layout_string(std::string& layoutString, std::array<Theme, 25>& themes, std::array<size_t, 25>& seeds);

	// 根据给定类型生成大特性僵尸列表，givenTypes 为每个池子指定的索引（+1），0 表示无限制，返回僵尸卡槽（SlotType）
	static SlotType generateSlotTypes(std::array<std::vector<int>, 4> givenTypes);

	// 生成小特性列表，根据 count 指定生成数量，hasSpecialFeatures 指定是否允许特殊特性，给定的小特性列表 givenFeatures 优先使用
	static std::vector<SmallFeature> generateSmallFeatures(int count, bool hasSpecialFeatures, std::vector<SmallFeature> givenFeatures);

	// 命令行生成大特性僵尸卡槽，结果存入 zombies 中
	static void genSlotTypeCli(SlotType& zombies);

	// 根据 ZombieType 返回其排序的权重，排序值较小表示优先级较高（constexpr 函数）
	static constexpr int getCardOrder(ZombieType::ZombieType zt);

};
class GameControl;



// 阳光花费字典
std::unordered_map < ZombieType::ZombieType, std::pair<int, std::string >> ZombieSunCost = {
	{ZombieType::Imp,                 {50,  "小鬼僵尸"}},
	{ZombieType::ConeheadZombie,      {75,  "路障僵尸"}},
	{ZombieType::PoleVaultingZombie,  {75,  "撑杆僵尸"}},
	{ZombieType::BucketheadZombie,    {125, "铁桶僵尸"}}, // 修正名称
	{ZombieType::BungeeZombie,        {125, "蹦极僵尸"}},
	{ZombieType::DiggerZombie,        {125, "矿工僵尸"}},
	{ZombieType::LadderZombie,        {150, "梯子僵尸"}},
	{ZombieType::FootballZombie,      {175, "橄榄球僵尸"}},
	{ZombieType::DancingZombie,       {350, "舞王僵尸"}},
	{ZombieType::BackupDancer,       {0, "伴舞僵尸"}},
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

