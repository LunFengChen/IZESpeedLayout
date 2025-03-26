#include "pvzclass.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <iomanip>
#include <numeric>
#include<unordered_set>

#include <intrin.h>
#include <iphlpapi.h>
#include <VersionHelpers.h>
#include <wincrypt.h>
#include <windows.h>

#include <comdef.h>
#include <Wbemidl.h>
#include <fstream>
#include <ctime>
#include <conio.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "wbemuuid.lib")

// 加密类
#include "EncryptUtils.h"
// 汇编类
#include "AsmCode.h"
// 布阵码生成器类
#include "GenerateLayoutCode.h"
// 日志记录类
#include "Logger.h"
// 关卡数据类
#include "LevelData.h"
// 时间类
#include "TimeStruct.h"
// 六届布阵器的汇编类
#include "iMemory.hpp"

// 全局随机数
std::random_device rd;
std::mt19937_64 gen(rd()); // 全局随机数生成器(32位就够用了）

// 布阵器控制
constexpr wchar_t WINDOW_NAME[] = L"IZE竞速布阵器";
constexpr char INIT_WORDS[] = "\
欢迎使用IZE竞速玩法布阵器 = v = \n\
作者: 解不出积分的小风; \n\
github地址:https://github.com/LunFengChen/IZESpeedLayout/ \n\
请在输入功能对应序号后按回车键：\n\
1：布阵\n\
2：生成25关随机阵型代码\n\
3: 限时残局玩法\n\
4：计时但不限时冲关\n\
5: 生成当前电脑机器码【比赛模式】\n\
6：裁判生成随机阵型代码【比赛模式】\n\
7：布阵【比赛模式】\n\
8：娱乐模式\n\
9：弹出工具-磁铁倒计时\n\
a：生成25关随机阵型代码【自定义花数分布】\n\
b：布阵，不限时玩法【可以布1-n个阵】\n\
c：导出本关ize阵型代码\n\
0：使用说明";









// 反作弊检测
class GameCheatCheck {
public:
	bool check_all(Logger& logger) {
		//std::cout << "正在反作弊检测" << std::endl;
		if (check_speed(logger)) {
			logger.log("检测到速度异常", Logger::DEBUG);
			return true;
		}
		else if (check_speed_constant(logger)) {
			logger.log("检测到速度异常", Logger::DEBUG);
			return true;
		}
		else if (check_Kernelpult(logger)) {
			logger.log("检测到玉米异常", Logger::DEBUG);
			return true;
		}
		else if (check_rnd(logger)) {
			logger.log("检测到随机数异常", Logger::DEBUG);
			return true;
		}
	}

	// 检查速度是否异常
	bool check_speed(Logger& logger) {
		// 检测修改帧间隔加速
		int time_ms = PVZ::Memory::ReadMemory<int>(PVZ::Memory::ReadMemory<int>(0x6a9ec0)+0x454);
		if (time_ms != 10) {
			logger.log("检测到速度异常，帧间隔异常，" + std::to_string(time_ms), Logger::DEBUG);
			return true;
		}

		// pvz自带加速: 25px, 0.25px
		if (PVZ::Memory::ReadMemory<bool>(0x6A9EAB) || PVZ::Memory::ReadMemory<bool>(0x6A9EAA)) {
			logger.log("检测到速度异常，可能启用了20px和0.25px", Logger::DEBUG);
			return true;
		}

		// 检测iz布阵器的那种超级加速


		return false;
	}


	// 检测相对速度常量是否被修改
	bool check_speed_constant(Logger& logger) {

		bool data_changed = false; // 记录是否有数据变化
		// 定义速度和预期值的对照表
		struct SpeedCheck {
			float current_value;
			float expected_value;
			const char* name;
		};

		// 初始化每个检查项
		SpeedCheck speed_checks[] = {
			{PVZ::Memory::ReadMemory<float>(0x6793C0), 0.5, "舞王滑步"},
			{PVZ::Memory::ReadMemory<float>(0x67966C), 0.12, "反矿"},
			{PVZ::Memory::ReadMemory<float>(0x679668), 0.9, "小鬼"},
			{PVZ::Memory::ReadMemory<float>(0x67963C), 0.45, "舞王/伴舞前进"},
			{PVZ::Memory::ReadMemory<float>(0x679640), 0.66, "矿工挖掘浮动下界"},
			{PVZ::Memory::ReadMemory<float>(0x679644), 0.68, "矿工挖掘浮动上界"},
			{PVZ::Memory::ReadMemory<float>(0x679648), 0.79, "跑步梯子浮动下界"},
			{PVZ::Memory::ReadMemory<float>(0x67964C), 0.81, "跑步梯子浮动上界"},
			{PVZ::Memory::ReadMemory<float>(0x679670), 0.23, "其他僵尸相对速度浮动下界"},
			{PVZ::Memory::ReadMemory<float>(0x679660), 0.37, "其他僵尸相对速度浮动上界"}
		};

		// 检查每个速度值是否符合预期
		for (const auto& check : speed_checks) {
			if (check.current_value != check.expected_value) {
				logger.log(
					std::string(check.name)
					+ "被锁定！原数据: " +std::to_string(check.expected_value)
					+"修改后: " + std::to_string(check.current_value)
					,Logger::DEBUG
				);
				data_changed = true;  // 只要有一个不匹配就返回true
			}
		}

		return data_changed;  // 如果所有的值都符合预期，返回false
		
	}

	// 检测是否开启免费种植


	// 监测锁玉米或者黄油
	bool check_Kernelpult(Logger& logger) {
		if (PVZ::Memory::ReadMemory<byte>(0x45F1EC) != byte(117)) {
			logger.log("检测到玉米子弹异常!", Logger::DEBUG);
			return true;
		}
		if (PVZ::Memory::ReadMemory<byte>(0x45F1EC) == byte(235)) {
			logger.log("检测到锁玉米粒!", Logger::DEBUG);
			return true;
		}
		if (PVZ::Memory::ReadMemory<byte>(0x45F1EC) == byte(112)) {
			logger.log("检测到锁黄油!", Logger::DEBUG);
			return true;
		}

		return false;
	}

	// TODO: dance检测, 现在这个版本没完成
	bool check_dance(Logger& logger) {
		if (PVZ::Memory::ReadPointer(0x6A9EC0, 0x768, 0x5765)) {
			logger.log("检测到开启dance!", Logger::DEBUG);
				return true;
			}
		return false;
	}

	// TODO: 检测是否铲植物
	bool check_delete_plant() {
		// 怎么写逻辑? 如果是土豆雷的话，可能误伤了...
	}

	// rnd检测
	bool check_rnd(Logger& logger) {
		bool data_changed = false; // 记录是否有数据变化
		// 定义速度和预期值的对照表
		struct RndFloatCheck {
			float current_value;
			float expected_value;
			const char* name;
		};

		// Rnd_3_4锁浮点数是写整数锁某些值，检测不是原数据；锁其他的都是写字节184给某些值，检测不是184就行；
		RndFloatCheck rnd_float_checks[] = {
			//Float
			// 激活比例：锁上下限，数据填入0x004144D8
			{PVZ::Memory::ReadMemory<int>(0x00414073), 6706212, "激活比例 内存数据1"}, // 锁下限
			{PVZ::Memory::ReadMemory<int>(0x00414080), 6788032, "激活比例 内存数据2"}, // 锁下限
			// 小丑速度：锁上下限，数据填入
			{PVZ::Memory::ReadMemory<int>(0x00524C29), 6788676, "小丑速度 内存数据1"}, // 锁下限
			{PVZ::Memory::ReadMemory<int>(0x00524C36), 6788672, "小丑速度 内存数据2"}, // 锁下限
			// 梯子速度：锁上下限，数据填入0x005149E4
			{PVZ::Memory::ReadMemory<int>(0x00524C00), 6788684, "梯子速度 内存数据1"}, // 锁下限
			{PVZ::Memory::ReadMemory<int>(0x00524C0D), 6788680, "梯子速度 内存数据2"}, // 锁下限
			// 海豚速度：锁上下限，数据填入0x005149E8
			{PVZ::Memory::ReadMemory<int>(0x00524BD7), 6788692, "海豚速度 内存数据1"}, // 锁下限
			{PVZ::Memory::ReadMemory<int>(0x00524BE4), 6788688, "海豚速度 内存数据2"}, // 锁下限
			// 普僵速度：锁上下限，数据填入0x005149EC
			{PVZ::Memory::ReadMemory<int>(0x00524B83), 6788704, "普僵速度 内存数据1"}, // 锁下限
			{PVZ::Memory::ReadMemory<int>(0x00524B90), 6788720, "普僵速度 内存数据2"}, // 锁上限
			// 小鬼参数：锁了某个值0x0052707D，0x0052708A，0x00527085，0x00527081【后面3个都是字节数组，懒得写，反正有一个就行】, 数据填入0x004144DC
			{PVZ::Memory::ReadMemory<int>(0x0052707D), 6788248, "小鬼参数 内存数据1"}, // 锁下限

		};

		// 检查浮点数是否异常
		for (const auto& check : rnd_float_checks) {
			if (check.current_value != check.expected_value) {
				logger.log(
					std::string(check.name)
					+ "被锁定！原数据: "
					+ std::to_string(check.expected_value)
					+ "修改后: "
					+ std::to_string(check.current_value)
					, Logger::DEBUG
				);
				data_changed = true;  // 只要有一个不匹配就返回true
			}
		}

		struct RndByteCheck {
			byte current_value;
			const char* name;
		};
		RndByteCheck rnd_byte_checks[] = {
			//Zombies
			{PVZ::Memory::ReadMemory<byte>(0x00522FD2), "小丑倒数"},
			{PVZ::Memory::ReadMemory<byte>(0x00522FE8), "小丑早爆"},
			{PVZ::Memory::ReadMemory<byte>(0x0052259F), "其他出生"},
			{PVZ::Memory::ReadMemory<byte>(0x00522CDB), "撑杆出生"},
			{PVZ::Memory::ReadMemory<byte>(0x00522DEF), "冰车出生"},
			{PVZ::Memory::ReadMemory<byte>(0x00522E91), "投篮出生"},
			{PVZ::Memory::ReadMemory<byte>(0x00523D38), "巨人出生"},
			{PVZ::Memory::ReadMemory<byte>(0x0052B907), "大蒜方向"},
			{PVZ::Memory::ReadMemory<byte>(0x00523A8B), "辣椒倒数"},
			{PVZ::Memory::ReadMemory<byte>(0x00522A26), "小偷高度"},
			{PVZ::Memory::ReadMemory<byte>(0x005234FE), "舞王滑步"},
			{PVZ::Memory::ReadMemory<byte>(0x005232B5), "雪人逃跑"},
			{PVZ::Memory::ReadMemory<byte>(0x0041CEA8), "跳跳初始"},
			//Plants
			{PVZ::Memory::ReadMemory<byte>(0x00532420), "一次冻结"},
			{PVZ::Memory::ReadMemory<byte>(0x0053240F), "二次冻结"},
			{PVZ::Memory::ReadMemory<byte>(0x004140C4), "刷新倒数"},
			{PVZ::Memory::ReadMemory<byte>(0x00413BBE), "天降间隔"},
			{PVZ::Memory::ReadMemory<byte>(0x0045E3CC), "小喷橫移"},
			{PVZ::Memory::ReadMemory<byte>(0x0045E3DC), "小喷纵移"},
			{PVZ::Memory::ReadMemory<byte>(0x0045E66F), "阳光橫移"},
			{PVZ::Memory::ReadMemory<byte>(0x0045E67F), "阳光纵移"},
			{PVZ::Memory::ReadMemory<byte>(0x0045F1E5), "锁定黄油"},
			{PVZ::Memory::ReadMemory<byte>(0x004630F4), "保龄方向"},
			{PVZ::Memory::ReadMemory<byte>(0x0045DED0), "生产初始"},
			{PVZ::Memory::ReadMemory<byte>(0x0045FA91), "生产间隔"},
			{PVZ::Memory::ReadMemory<byte>(0x0045DEE2), "攻击初始"},
			{PVZ::Memory::ReadMemory<byte>(0x0045F8BA), "攻击间隔"},
			{PVZ::Memory::ReadMemory<byte>(0x0042AFA6), "IZE减花"},
		};
		// 检查其他是否异常
		for (const auto& check : rnd_byte_checks) {
			if (check.current_value == byte(184)) {
				logger.log(
					std::string(check.name)
					+ "被锁定! "
					, Logger::DEBUG
				);
				data_changed = true;  // 只要有一个不匹配就返回true
			}
		}

		return data_changed;  // 如果所有的值都符合预期，返回false
		
	}

	// 监测后台进程
	bool monitor_background_process() {
		// 监测后台进程是否有算血器：常规信息中的描述为"IZECalculatorV1.5.10.exe"
		// 监测后台进程是否有IZ布阵器：常规信息中的描述为"IZ_Format_Designer_V2"


		// 监测后台进程是否有Rnd：常规信息中的描述为"Rnd_3_4.exe"
		// 监测后台进程是否有pt：常规信息中的描述为"PvZ Tools"
		// 监测后台进程是否有ptk：常规信息中的描述为"PvZ Toolkit"
		// 监测后台进程是否有终极修改器：常规信息中的描述为"PVZWPF修改器"

	}



};

// 控制游戏
class GameControl {


private:;
		// 检测游戏开启
		bool is_game_on()
		{
			return PVZ::Memory::ReadMemory<int>(0x6a9ec0) != 0;
		}
		// 获取游戏mode
		int get_game_mode() {
			return PVZ::Memory::ReadPointer(0x6a9ec0, 0x7f8);
		}
		// 获取游戏ui
		int get_game_ui()
		{
			return PVZ::Memory::ReadPointer(0x6a9ec0, 0x7fc);
		}


public:
	SPT<PVZ::Board> board;
	SPT<PVZ::PVZApp> pvz;
	
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

	GameControl(DWORD pid) {
		if (pid) {
			PVZ::InitPVZ(pid);
			this->board = PVZ::GetBoard();
			this->pvz = PVZ::GetPVZApp();
		}
	}



	// 打开自动收集
	void auto_collect(bool on) {
		if (on) PVZ::Memory::WriteMemory<byte>(0x0043158f, 0xeb);
		else PVZ::Memory::WriteMemory<byte>(0x0043158f, 0x75);
	}

	// 检测是否在ize中
	bool is_in_ize() { // 进入了ize中
		return (is_game_on() && get_game_mode() == 70 && (get_game_ui() == 2 || get_game_ui() == 3));
	}

	// 布阵
	void set_layout(const std::string& ls, int flower_num) {
		if (is_in_ize()) {
			// ls = "1/2130778634";
			int theme = static_cast<int>(ls[0] - '0');
			size_t seed = std::stoull(ls.substr(2));

			auto plantTypes = GenerateLayoutCode::get_theme_plants(flower_num, static_cast<Theme>(theme)); // 获取不同主题的植物生成顺序
			auto orders = GenerateLayoutCode::get_shuffled_array(seed);

			// 逆序删植物
			auto board = PVZ::GetBoard();
			if (board->PlantsCount > 0) {
				auto pPlants = board->GetAllPlants();
				for (size_t i = 0; i < pPlants.size(); i++) {
					auto idx = pPlants.size() - 1 - i;//逆序删
					if (pPlants[idx] != nullptr) {
						if (!pPlants[idx]->NotExist) pPlants[idx]->Remove();
						//std::cout << "正在删除栈位为" << pPlants[idx]->Index << "的植物" << std::endl;
						Sleep(1);
					}
				}
			}


			//调用游戏刷新函数0x41ca10:(1)先写入字节数组(2)后写入标志位（3）游戏根据标志位，读写入的字节数组
			for (size_t i = 0; i < 25; i++) // 把植物塞到内存中
			{//
				int written[] = { (orders[i]) / 5 , (orders[i]) % 5 ,static_cast<int>(plantTypes[i]) };
				PVZ::Memory::WriteArray<int>(0x6b1200 + 12 * i, written, 12);
			}
			Sleep(20);
			PVZ::Memory::WriteMemory<byte>(0x6b0905, 3); // 调用游戏自身的布阵功能
			//std::cout << "等游戏刷新植物" << std::endl;
			Sleep(20);


			////TODO: 不出图只能这样了, 怎么解决不出图的问题？
			//if (board->PlantsCount == 0) {
			//	for (int i = 0; i < 25; i++) {
			//		GameControl::spawn_plant(plantTypes[i], orders[i] / 5, orders[i] % 5);
			//	}
			//}

			// 设置小喷偏移
			std::mt19937_64 genPuffshroom(seed + 1);
			std::uniform_int_distribution<int> rngPuffshroomX(-5, 4);
			std::uniform_int_distribution<int> rngPuffshroomY(-3, 2);

			for (auto& plant : board->GetAllPlants()) {
				if (plant->Type == PlantType::Puffshroom) {
					float x = 80.0 * plant->Column;
					float y = 40.0 + 100.0 * plant->Row;
					plant->ImageX = static_cast<int>(x) + 40 + rngPuffshroomX(genPuffshroom);
					plant->ImageY = static_cast<int>(y) + 40 + rngPuffshroomX(genPuffshroom);
				};
			};


		};
	}

	// 删除场上还没收集的阳光
	void clear_not_colleted_sun() {
		auto board = PVZ::GetBoard();
		auto all_coins = board->GetAllCoins();

		for (auto& coin : all_coins) {
			if (coin->Type == CoinType::NormalSun) {
				coin->Collected = true; // 设置为收集了
				coin->NotExist = true; // 删了
			}
		}
	}

	// 数脑子
	int countEatenBrain()
	{
		return PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x160, 0x60);
	}

	// 设置脑子数
	void set_EatenBrains(int value) {
		PVZ::Memory::WriteMemory(PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x160) + 0x60, value);
	}

	// 汇编删脑子
	void clear_all_brains() {
		if (is_in_ize())
		{
			// 初始化汇编环境
			AsmCode code;
			code.asm_init();

			// 找到脑子
			unsigned int griditem_struct_size = 0xec;
			auto grid_item_count_max = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x120);
			auto grid_item_offset = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x11c);
			for (size_t i = 0; i < grid_item_count_max; i++)
			{
				auto grid_item_disappeared = PVZ::Memory::ReadMemory<bool>(grid_item_offset + 0x20 + griditem_struct_size * i);
				auto grid_item_type = PVZ::Memory::ReadMemory<int>(grid_item_offset + 0x8 + griditem_struct_size * i);
				if (!grid_item_disappeared && grid_item_type == 12)// 场地物品没空且是脑子，删了
				{
					int addr = grid_item_offset + 0xec * i;
					code.asm_mov_exx(AsmCode::Reg::ESI, addr);
					code.asm_call(0x0044d000);
				}
			}
			// 注入代码
			code.asm_ret();
			code.asm_code_inject(PVZ::Memory::hProcess);
		}
	}

	// pvzclass更新脑子// 初始化，你得先找到游戏
	void update_brains() {
		// 在ize中即可
		if (is_in_ize()) {
			// 1. 汇编先删脑子
			clear_all_brains();
			// 2. 设置关卡进程0/5【吃的脑子数】，进度条0
			PVZ::Memory::WriteMemory<int>(PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x160) + 0x60, 0);
			PVZ::Memory::WriteMemory<int>(PVZ::Memory::ReadPointer(0x6a9ec0, 0x768) + 0x5610, 0);
			// 3. 生成5个脑子
			for (size_t i = 0; i < 5; i++) {
				SPT<PVZ::Griditem> iz_brain_new = Creator::CreateGriditem();
				// 设置基类属性：场地物品属性
				iz_brain_new->Row = i; iz_brain_new->Column = 0;
				iz_brain_new->Layer = 302000 + i * 10000;//图层
				iz_brain_new->NotExist = false;
				iz_brain_new->Type = GriditemType::IZBrain;
				// 设置派生类属性：IZ脑子专有属性：hp 和 Y坐标
				PVZ::Memory::WriteMemory<int>(iz_brain_new->GetBaseAddress() + 0x18, 70);// hp
				PVZ::Memory::WriteMemory<float>(iz_brain_new->GetBaseAddress() + 0x28, 120 + i * 100); // Y
			}
		}
	}

	// 写内存删除全部僵尸
	void clear_all_zombies() {
		if (is_in_ize()) {
			auto zombie_count_max = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x94); // 0x6a9ec0: lawnl; 0x768: board; 0x94:zombie_count_max
			auto zombie_offset = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x90); // 0x90: zombie

			for (size_t i = 0; i < zombie_count_max; i++) { // 0xec: zombie_dead ;0x28: zombie_status
				if (!PVZ::Memory::ReadMemory<bool>(zombie_offset + 0xec + i * 0x15c)) { // 如果僵尸没死就给他们设置死的状态
					PVZ::Memory::WriteMemory<int>(zombie_offset + 0x28 + i * 0x15c, 3); // 3为iz布阵器那种删除， 1为缓慢消失
				}
			}
		}
	}

	// 汇编删全部子弹
	void clear_all_bullets() {
		if (is_in_ize()) {
			unsigned int bullet_struct_size = 0x94; // 每个植物的分块内存大小 332字节
			auto bullet_count_max = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0xcc);
			auto bullet_offset = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0xc8);
			// 初始化汇编环境
			AsmCode code;
			code.asm_init();

			for (size_t i = 0; i < bullet_count_max; i++) {
				auto bullet_disappered = PVZ::Memory::ReadMemory<bool>(bullet_offset + 0x50 + bullet_struct_size * i);
				if (!bullet_disappered) { // 子弹消失则为true, 取反
					uint32_t addr = bullet_offset + bullet_struct_size * i;
					code.asm_mov_exx(AsmCode::Reg::EAX, addr);
					code.asm_call(0x46eb20); // 调用植物删除函数call_delete_plant
				}
			};

			// 执行上述汇编命令
			code.asm_ret();
			code.asm_code_inject(PVZ::Memory::hProcess);
		}
	}

	// 汇编删植物
	void clear_all_plants() {
		if (is_in_ize())
		{
			unsigned int plant_struct_size = 0x14c; // 每个植物的分块内存大小 332字节
			auto plant_count_max = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0xb0); // 植物最大数量
			auto plant_offset = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0xac); // 植物偏移

			// 初始化汇编环境
			AsmCode code;
			code.asm_init();

			// 依次删除场上植物
			for (size_t i = 0; i < plant_count_max; i++) { // 关于指针: 序列第 42 (十进制) 个位置的僵尸的属性倒计时: [[[[0x6A9EC0] +0x768] +0x90] +0x68 +0x15C*42].
				auto plant_dead = PVZ::Memory::ReadMemory<bool>(plant_offset + 0x141 + plant_struct_size * i); // 0表示正常
				if (!plant_dead) {
					uint32_t addr = plant_offset + plant_struct_size * i;
					code.asm_push(addr);
					code.asm_call(0x004679b0); // 调用植物删除函数call_delete_plant
				}
			}

			// 执行上述汇编命令
			code.asm_ret();
			code.asm_code_inject(PVZ::Memory::hProcess);
		}
	}

	// 汇编放一个植物
	void spawn_plant(int type, int row, int col) {
		if (is_in_ize())
		{
			// 初始化
			AsmCode code;
			code.asm_init();

			// 种植物
			code.asm_push(-1);
			code.asm_push(type);
			code.asm_mov_exx(AsmCode::Reg::EAX, row);
			code.asm_push(col);
			code.asm_mov_exx_dword_ptr(AsmCode::Reg::EBP, 0x6a9ec0);
			code.asm_mov_exx_dword_ptr_exx_add(AsmCode::Reg::EBP, 0x768);
			code.asm_push_exx(AsmCode::Reg::EBP);
			code.asm_call(0x0040d120); // call_put_plant

			code.asm_add_list(0x8b, 0xf0); // mov esi, eax
			code.asm_push_exx(AsmCode::Reg::EAX);
			code.asm_mov_exx_dword_ptr(AsmCode::Reg::EAX, 0x6a9ec0);
			code.asm_mov_exx_dword_ptr_exx_add(AsmCode::Reg::EAX, 0x768);
			code.asm_mov_exx_dword_ptr_exx_add(AsmCode::Reg::EAX, 0x160);
			code.asm_call(0x0042a530); // call_put_plant_iz_style
			code.asm_add_list(0x8b, 0xc6); // mov eax, esi


			// 注入
			code.asm_ret();
			code.asm_code_inject(PVZ::Memory::hProcess);
		}

	}

	// 汇编生成植物
	void spawn_all_plants(std::array<PlantType::PlantType, 25> plant_types, std::array<int, 25> orders) {
		// 初始化
		AsmCode code;
		code.asm_init();

		// 逆序种植物
		for (int i = 24; i >= 0; i--) {
			auto type = plant_types[i];
			auto row = orders[i] / 5;
			auto col = orders[i] % 5;

			// 种植物
			code.asm_push(-1);
			code.asm_push(type);
			code.asm_mov_exx(AsmCode::Reg::EAX, row);
			code.asm_push(col);
			code.asm_mov_exx_dword_ptr(AsmCode::Reg::EBP, 0x6a9ec0);
			code.asm_mov_exx_dword_ptr_exx_add(AsmCode::Reg::EBP, 0x768);
			code.asm_push_exx(AsmCode::Reg::EBP);
			code.asm_call(0x0040d120); // call_put_plant

			code.asm_add_list(0x8b, 0xf0); // mov esi, eax
			code.asm_push_exx(AsmCode::Reg::EAX);
			code.asm_mov_exx_dword_ptr(AsmCode::Reg::EAX, 0x6a9ec0);
			code.asm_mov_exx_dword_ptr_exx_add(AsmCode::Reg::EAX, 0x768);
			code.asm_mov_exx_dword_ptr_exx_add(AsmCode::Reg::EAX, 0x160);
			code.asm_call(0x0042a530); // call_put_plant_iz_style
			code.asm_add_list(0x8b, 0xc6); // mov eax, esi
		}
		// 注入
		code.asm_ret();
		code.asm_code_inject(PVZ::Memory::hProcess);

	}

	// izt字符布阵
	void set_layout_iztstr(const std::string& iztStr) {

	}

	void set_speed_0_25x() {
		if (is_in_ize()) {
			PVZ::Memory::WriteMemory<bool>(0x6A9EAA, true);
		}
	}

	void set_speed_20x() {
		if (is_in_ize()) {
			PVZ::Memory::WriteMemory<bool>(0x6A9EAB, true);
		}
	}

	void set_speed_10x() {
		if (is_in_ize()) {
			PVZ::Memory::WriteMemory<int>(PVZ::Memory::ReadMemory<int>(0x6a9ec0) +0x454, 1);
		}
	}

	void reset_speed() {
		if (is_in_ize()) {
			PVZ::Memory::WriteMemory(PVZ::Memory::ReadMemory<int>(0x6a9ec0) + 0x454, 10); // 1帧10cs
			PVZ::Memory::WriteMemory<bool>(0x6A9EAA, false);
			PVZ::Memory::WriteMemory<bool>(0x6A9EAB, false);
		}
	}


	// 禁用女仆: 有bug
	void ban_mj(bool is_ban_mj, Logger& logger) {
		if (is_in_ize() && is_ban_mj) { // 开启禁用女仆功能
			logger.log("已禁用女仆!", Logger::INFO);
			byte disable_MaidCheat_code[] = {0x68, 0x07, 0x00, 0x00, 0x8b, 0x80, 0x68, 0x55, 0x00, 0x00, 0x99, 0xf7, 0xf9, 0x8b, 0xc2, 0x99, 0xf7, 0xfe, 0x5e, 0xc3};
			PVZ::Memory::WriteArray<byte>(0x52dfcb, disable_MaidCheat_code, 20);
		}
		else {
			logger.log("女仆状态: 正常!", Logger::INFO);
			byte enable_MaidCheat_code[] = { 0x38 ,0x08 ,0x00 ,0x00 ,0x99 ,0xF7 ,0xF9 ,0x8B ,0xC2 ,0x99 ,0xF7 ,0xFE ,0x5E ,0xC3 ,0xCC ,0xCC ,0xCC ,0xCC ,0xCC ,0xCC };
			PVZ::Memory::WriteArray<byte>(0x52dfcb, enable_MaidCheat_code, 20);
		}
	}

    // 开启反作弊
    void cheat_check(bool is_cheat_check, Logger& logger) {
		if (is_in_ize() && is_cheat_check) {
			GameCheatCheck game_cheat_checker;
			if (game_cheat_checker.check_all(logger)) {
				logger.log("检测到异常!", Logger::INFO);
			}
		}
    }
};


// 布阵器控制
class LayoutControler {
public:


	void register_LevelRush_hotkey(){
		RegisterHotKey(NULL, 1, MOD_SHIFT, 'D'); // ctrl+d打开加速
		RegisterHotKey(NULL, 2, MOD_SHIFT, 'A'); // 打开自动收集
		RegisterHotKey(NULL, 3, MOD_SHIFT, 'Q'); // 强制退出
		RegisterHotKey(NULL, 4, MOD_SHIFT, 'J'); // 跳关
		RegisterHotKey(NULL, 5, MOD_SHIFT, 'M'); // 开关女仆
	}

	void unregister_LevelRush_hotkey() {
		UnregisterHotKey(NULL, 1);
		UnregisterHotKey(NULL, 2);
		UnregisterHotKey(NULL, 3);
		UnregisterHotKey(NULL, 4);
		UnregisterHotKey(NULL, 5);
	}

	void register_SpeedRun_hotkey() {
		RegisterHotKey(NULL, 1, MOD_SHIFT, 'Q'); // 强制退出
		RegisterHotKey(NULL, 2, MOD_SHIFT, 'R'); // 重开
	}

	void unregister_SpeedRun_hotkey() {
		UnregisterHotKey(NULL, 1);
		UnregisterHotKey(NULL, 2);
	}

	int get_terminal_width() {
		int width = 80; // 默认宽度
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		width = csbi.srWindow.Right - csbi.srWindow.Left;

		return width;
	}

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


	// 统计玉米子弹: 玉米粒 黄油
	void count_Kernelpult() {

	}


	// 冲关循环: 布阵器输入了4
	void LevelRush(const bool is_cheat_check, bool is_ban_mj) {
		// 日志记录
		Logger logger(TimeStruct::getCurrentDateTime() + ".log", Logger::DEBUG); // 默认打印INFO

		// 1. 找到 pvz
		DWORD pid = ProcessOpener::Open();
		if (!pid) {
			logger.log("未找到pvz!", Logger::INFO);
			return; // 结束
		}
		logger.log("已找到pvz!", Logger::INFO);
		EnableBackgroundRunning(true); // 启用pvz后台运行

		// 2. 实例化游戏控制类, 日志记录
		GameControl game_controler(pid);

		// 4. 实例化冲关布阵码生成器
		GenerateLayoutCode code_generator;

		// 5. 一直检测，直到进入ize
		while (!game_controler.is_in_ize()) {
			Sleep(1);
		}
		logger.log("已经进入ize, 现在开始布阵", Logger::INFO);


		// 检测环境是否异常
		GameCheatCheck game_cheat_checker;
		TimeStruct check_time = TimeStruct::getNow();
		int check_interval = 1;
		if (is_cheat_check) {
			if (game_cheat_checker.check_all(logger)) {
				logger.log(std::string(get_terminal_width(), '-'), Logger::INFO);
				logger.log("环境检测结果: 异常!", Logger::INFO);
				logger.log(std::string(get_terminal_width(), '-'), Logger::INFO);
				return;
			}
			else {
				logger.log(std::string(get_terminal_width(), '-'), Logger::INFO);
				logger.log("环境正常，请继续游戏!", Logger::INFO);
				logger.log(std::string(get_terminal_width(), '-'), Logger::INFO);
			}
		}
		// 按照输入的指令选择是否禁用女仆
		game_controler.ban_mj(is_ban_mj, logger);

		// 6. 初始化bool值
		bool is_speed_up = false; // 是否开启了加速
		bool is_auto = false; // 是否开启了自动收集


		game_controler.auto_collect(false); // 关掉自动收集
		game_controler.reset_speed(); // 恢复原速

		// 记录关卡和游戏开始时间
		int current_flag = 0;
		bool has_started = false;
		TimeStruct start_time = TimeStruct::getNow();
		int scardy_theme_count = 0;
		// ?
		auto currentAddress = game_controler.board->GetBaseAddress();


		// 记录已经监测并处理的僵尸
		std::unordered_set<int> processed_zombie_ids;

		// 结束条件
		int lowestSun = 50;

		// 7.初始化第一关数据并布阵
		game_controler.board->GetMiscellaneous()->Round = 0; // 从第一关开始
		game_controler.board->Sun = 150; // 设置初始阳光
		game_controler.update_brains(); // 脑子初始化为0
		game_controler.clear_all_zombies();
		game_controler.clear_all_bullets();
		game_controler.clear_not_colleted_sun();
		logger.log("初始化第一关信息，并进行第一关的布阵", logger.DEBUG);

		// 记录存档
		std::vector<LevelData> save_data;
		std::vector<std::string> all_layout_code;


		// 7. 开始游戏，保存第一关数据，并对第一关进行布阵
		LevelData leveldata; 
		leveldata.initial_sun = 150;
		leveldata.released_zombies_count = 0;
		leveldata.zombie_cost = 0;
		leveldata.kernelpult_butter_rate = 0.00;
		leveldata.score = 0.0;
		leveldata.setlayout_time = TimeStruct::getNow();
		leveldata.brain_eaten_times = {};

		// 拿到第一关阵型代码
		auto ls = code_generator.generate_LevelRush_code(0);
		auto flower_num = code_generator.get_LevelRush_flower_num_distribution(0);
		game_controler.set_layout(ls, flower_num); all_layout_code.push_back(ls);

		// 主循环
		MSG msg = { 0 };
		while (true) {
			Sleep(1);

			// 添加快捷键并处理，全局热键消息
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_HOTKEY) {
					if (msg.wParam == 1) {  // shift+D 切换加速与原速
						is_speed_up = !is_speed_up;
						if (is_speed_up) {
							game_controler.set_speed_10x();
							logger.log("进行了10x加速", logger.DEBUG);

						}
						else {
							game_controler.reset_speed();
							logger.log("关闭加速", logger.DEBUG);

						};
					}
					else if (msg.wParam == 2) { // shift+A 切换自动收集
						is_auto = !is_auto;
						game_controler.auto_collect(is_auto);
                        logger.log(std::string(is_auto ? "打开" : "关闭") +"自动收集", logger.DEBUG);
					}
					else if (msg.wParam == 3) { // shift+q 强制结束
						game_controler.auto_collect(false);
						game_controler.reset_speed();

						auto over_time = TimeStruct::getNow() - start_time;
						logger.log(over_time.enPrint().append(" 提前结束游戏!"), Logger::INFO);
						logger.log(std::string(get_terminal_width(), '-'), Logger::INFO);
						std::ostringstream oss;
						oss << std::fixed << std::setprecision(1) << (std::round(leveldata.score * 10) / 10.0);
						std::string score_str = oss.str();

						logger.log("游戏结束! 最终得分为——  " + score_str, Logger::INFO);
						logger.log("最后吃脑时间为: " + (leveldata.last_brain_eaten_time - start_time).cnPrint(), Logger::INFO);

						auto timeStr = (leveldata.last_brain_eaten_time - start_time).enPrint().append("     ").append(std::to_string(std::round(leveldata.score * 10) / 10.0));
						Creator::CreateCaption(timeStr.c_str(), timeStr.size() - 5, CaptionStyle::Lowermiddle);
						return;
					}
					else if (msg.wParam == 4) { // shift+j 跳关
						game_controler.board->Win();
						logger.log("跳关了", logger.DEBUG);
					}
					else if (msg.wParam == 5) {
						// 切换禁用女仆
						is_ban_mj = !is_ban_mj;
						game_controler.ban_mj(is_ban_mj, logger);
					}
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			// 1. 监测崩溃
			if(!ProcessOpener::Open()){
				logger.log("游戏关闭!", Logger::INFO);
				break;
			}

			// 开启反作弊
			//game_controler.cheat_check(is_cheat_check, logger);

			// 检测重开:?

			// 2. 检测作弊
			if (is_cheat_check)
			{
				do {// 先检测一次
					if ((TimeStruct::getNow() - check_time).second > check_interval) {
						game_controler.cheat_check(true, logger);
						check_time = TimeStruct::getNow();
						//logger.log("当前正在检测是否作弊！", logger.DEBUG);
					}
				} while (0);
			}

			// 2. 跨关更新
			if (has_started && game_controler.board->GetBaseAddress() &&
				game_controler.board->GetMiscellaneous()->Round != current_flag &&
				game_controler.pvz->GameState == PVZGameState::Playing) {

				// 2.1 存档
				save_data.push_back(leveldata);
				// 2.2 记录跨关数据
				logger.log((TimeStruct::getNow() - start_time).enPrint()
					+ " 已经通过" + std::to_string(game_controler.board->GetMiscellaneous()->Round)
					+ "关, 阳光" + std::to_string(game_controler.board->Sun)
					+ "，花费" + std::to_string(leveldata.zombie_cost),
					Logger::INFO);
				// 2.3 记录胆小
				if (ls[0] == '8')
				{
					logger.log("遇到了一次胆小, 目前胆小次数为: "+std::to_string(scardy_theme_count), Logger::DEBUG);
					scardy_theme_count += 1;
				}

				// 2.3 更新关数，初始化下一关记录的数据
				current_flag = game_controler.board->GetMiscellaneous()->Round;

				leveldata.initial_sun = game_controler.board->Sun;
				leveldata.released_zombies_count = 0;
				leveldata.zombie_cost = 0;
				leveldata.kernelpult_butter_rate = 0.00;
				leveldata.score = current_flag;
				leveldata.setlayout_time = TimeStruct::getNow();
				leveldata.brain_eaten_times = {};
				// 2.5 关闭加速
				game_controler.reset_speed();

				// 2.6 布阵
				ls = code_generator.generate_LevelRush_code(current_flag);
				flower_num = code_generator.get_LevelRush_flower_num_distribution(current_flag);
				game_controler.set_layout(ls, flower_num); all_layout_code.push_back(ls);
				logger.log("现在对第" + std::to_string(current_flag) + "关进行布阵,花数:"+std::to_string(flower_num) + ", 布阵码:" + ls, logger.DEBUG);
			}

			// 3. 检测是否开始游戏
			if (!has_started) {
				if (!game_controler.pvz->GetBaseAddress() ||
					game_controler.pvz->LevelId != PVZLevel::I_Zombie_Endless ||
					game_controler.pvz->GameState != PVZGameState::Playing)
					continue;

				// 判断释放僵尸条件
				if (game_controler.board->ZombiesCount != 1) continue;

				has_started = true;
				start_time = TimeStruct::getNow();
				// 开始时间
				leveldata.first_zombie_release_time = start_time;
				leveldata.reaction_time = leveldata.setlayout_time - leveldata.first_zombie_release_time;
				leveldata.last_brain_eaten_time = start_time;
					
				logger.log("开始游戏!  现实时间为: " + start_time.getCurrentTime(), Logger::INFO);
				logger.log(std::string(get_terminal_width(), '-'), Logger::INFO);
			}

			// 3. 监测放置的僵尸以及计算花费
			std::vector<SPT<PVZ::Zombie>> zombies = game_controler.board->GetAllZombies();
			for (auto& zombie : zombies) {
				// 如果僵尸死了，跳过
				if (zombie->NotExist) {
					processed_zombie_ids.erase(zombie->Id);
					continue;
				}
				// 如果不是刚放置的僵尸, 跳过
				if (zombie->ExistedTime < 0 || zombie->ExistedTime > 500) continue; // 只检查新生成的【冲关的话得放宽】
				// 如果这个僵尸已经处理过了, 跳过
				if (processed_zombie_ids.count(zombie->Id)) continue; // 已处理则跳过

				processed_zombie_ids.insert(zombie->Id); // 记录已处理

				// 如果不是ize中的僵尸，而且现在又不是开局的话, 记录下来
				if (!game_controler.ZombieSunCost.count(zombie->Type)) {
					// 重开的话需要先清除选卡界面的僵尸
					logger.log("检测到在第" + std::to_string(zombie->Row + 1) + "行放置了非ize关卡的僵尸,僵尸类型为" + ZombieType::ToString(zombie->Type), logger.DEBUG);
					continue;
				}
				// 如果是ize中的僵尸，但又不是伴舞僵尸，日志记录并且计算花费【每一关结束赋值】
				if (zombie->Type == ZombieType::BackupDancer) continue;

				auto zombie_info = game_controler.ZombieSunCost[zombie->Type];
				leveldata.zombie_cost += zombie_info.first;
				leveldata.released_zombies_count += 1;
				logger.log("在第" + std::to_string(zombie->Row + 1) + "行放置了" + std::string(zombie_info.second) + ",目前一共放了" + std::to_string(leveldata.released_zombies_count) + "个僵尸", logger.DEBUG);

				// 记录反应时间
				if (leveldata.released_zombies_count == 1) {
					leveldata.first_zombie_release_time = TimeStruct::getNow();
					leveldata.reaction_time = leveldata.first_zombie_release_time - leveldata.setlayout_time;
					logger.log("第" + std::to_string(current_flag + 1) + "关，第一个僵尸释放时间: " + (leveldata.first_zombie_release_time-start_time).enPrint(), logger.DEBUG);
					logger.log("第" + std::to_string(current_flag + 1) + "关，反应时间: " + leveldata.reaction_time.enPrint(), logger.DEBUG);
				}
			}

			// 4. 统计玉米黄油率
			for (auto& projectile : game_controler.board->GetAllProjectile()) {
				if (projectile->Type == ProjectileType::Butter) {
					logger.log("丢了一个黄油", Logger::DEBUG);
				}
				else if (projectile->Type == ProjectileType::Kernel) {
					logger.log("丢了一个玉米", Logger::DEBUG);
				}
			}

			// 5. 监测脑子变化
			if (std::abs(leveldata.score - (current_flag + game_controler.countEatenBrain() * 0.2))>1e-1) { // 脑子被吃
 				leveldata.score = game_controler.board->GetMiscellaneous()->Round + game_controler.countEatenBrain() * 0.2;
				leveldata.last_brain_eaten_time = TimeStruct::getNow();
				leveldata.brain_eaten_times.push_back(leveldata.last_brain_eaten_time);

				logger.log((leveldata.last_brain_eaten_time - start_time).enPrint() + "吃了第" + std::to_string(game_controler.countEatenBrain()) + "个脑子", logger.DEBUG);
			}

			// 6. lose， 打印所有数据
			if (game_controler.board->ZombiesCount == 0 && game_controler.board->Sun < lowestSun) {
				bool is_dead = true;

				for (auto& coin : game_controler.board->GetAllCoins()) {
					if (coin->Type == CoinType::NormalSun) is_dead = false;
				}
				if (!is_dead) continue; // 还没死就跳过
				// 游戏结束
				game_controler.auto_collect(false);
				game_controler.reset_speed();

				std::ostringstream oss;
				oss << std::fixed << std::setprecision(1) << (std::round(leveldata.score * 10) / 10.0);
				std::string score_str = oss.str();
				auto game_over_str = std::string("GameOver..").append("     ").append(std::to_string(scardy_theme_count) + "-") + score_str;
				logger.log(std::string(get_terminal_width(), '-'), Logger::INFO);
				logger.log((leveldata.last_brain_eaten_time - start_time).enPrint() + " 游戏结束--  得分: " + score_str, Logger::INFO);
				Creator::CreateCaption(game_over_str.c_str(), game_over_str.size() - 5, CaptionStyle::Lowermiddle); // 去掉浮点数的两位小数

				// 关卡结束后打印本次冲关数据
				std::string layout_codes;
				for (const auto& ls : all_layout_code) {
					layout_codes += ls + ".";
				}
				layout_codes = layout_codes.substr(0, layout_codes.size() - 1); // 去掉最后的'.'
				logger.log("本次冲关关卡布阵码为(已复制到剪切板): \n" + layout_codes, Logger::INFO);
				copyToClipBoard(layout_codes);

				logger.log("所有关卡数据如下:", logger.DEBUG);
				int count = 0;
				for (auto& leveldata : save_data) {
					logger.log("第" + std::to_string(++count) + "关数据如下:", Logger::DEBUG);
					logger.log("    放置的僵尸数: " + std::to_string(leveldata.released_zombies_count), Logger::DEBUG);
					logger.log("    僵尸花费: " + std::to_string(leveldata.zombie_cost), Logger::DEBUG);
					logger.log("    反应时间: " + leveldata.reaction_time.enPrint(), Logger::DEBUG);
					logger.log("    最后一个脑子吃的时间: " + (leveldata.last_brain_eaten_time - leveldata.setlayout_time).enPrint(), Logger::DEBUG);
					logger.log("    所有脑子吃的时间列表: ", Logger::DEBUG);

					for (auto& eat_brain_time : leveldata.brain_eaten_times) {
						logger.log((eat_brain_time - leveldata.setlayout_time).enPrint(), logger.DEBUG);
					}
				};
				// 退出本次冲关
				return;
					
			}


		}

			
			

	}

	// 30min限时循环
	void SpeedRun30min(std::string ls, const bool is_cheat_check) {

		// 0. 日志记录
		Logger logger(TimeStruct::getCurrentDateTime() + ".log", Logger::DEBUG); // 默认打印INFO, 但是记录的话全部记录

		


		// 1. 拿到所有关卡的布阵代码
		auto ss = std::stringstream(ls);
		auto str = std::string();
		auto vec = std::vector<std::string>();

		while (getline(ss, str, '.')) vec.push_back(str);

		if (vec.size() != 25) {
			logger.log("输入不合法", Logger::INFO);
			return;
		}


		// 2. 找到 pvz
		DWORD pid = ProcessOpener::Open();
		if (!pid) {
			logger.log("未找到pvz!", Logger::INFO);
			return; // 结束
		}
		logger.log("已找到pvz!", Logger::INFO);
		EnableBackgroundRunning(true); // 启用pvz后台运行

		// 3. 实例化游戏控制器,实例化布阵码生成器【为了拿花数】
		GameControl game_controler(pid);
		GenerateLayoutCode code_generator;
			

		// 4. 一直检测，直到进入ize
		while (!game_controler.is_in_ize()) {
			Sleep(1);
		}
		logger.log("已经进入ize, 现在开始布阵", Logger::INFO);


		// 检测环境是否异常
		GameCheatCheck game_cheat_checker;
		TimeStruct check_time = TimeStruct::getNow();
		int check_interval = 1;
		if (is_cheat_check) {
			if (game_cheat_checker.check_all(logger)) {
				logger.log(std::string(get_terminal_width(), '-'), Logger::INFO);
				logger.log("环境检测结果: 异常!", Logger::INFO);
				logger.log(std::string(get_terminal_width(), '-'), Logger::INFO);
				return;
			}
			else {
				logger.log(std::string(get_terminal_width(), '-'), Logger::INFO);
				logger.log("环境正常，请继续游戏!", Logger::INFO);
				logger.log(std::string(get_terminal_width(), '-'), Logger::INFO);
			}

		}
		// 禁mj
		game_controler.ban_mj(true, logger);

		// 5. 初始化游戏信息
		int current_flag = 0;
		bool has_started = false;
		TimeStruct start_time = TimeStruct::getNow();
		//?
		auto currentAddress = game_controler.board->GetBaseAddress();



		// 记录已经监测并处理的僵尸
		std::unordered_set<int> processed_zombie_ids;

		// 结束条件
		int lowestSun = 50;


		// 6. 记录存档
		std::vector<LevelData> save_data; 
		// 6.1. 初始化每关要记载的数据
		LevelData leveldata;
		leveldata.initial_sun = 150;
		leveldata.released_zombies_count = 0;
		leveldata.zombie_cost = 0;
		leveldata.kernelpult_butter_rate = 0.00;
		leveldata.score = 0.0;
		leveldata.setlayout_time = TimeStruct::getNow();
		leveldata.brain_eaten_times = {};


		// 7. 先对第一关进行布阵
		// 7.1 初始化第一关数据
		game_controler.board->GetMiscellaneous()->Round = 0; // 从第一关开始
		game_controler.board->Sun = 150; // 设置初始阳光
		game_controler.update_brains(); // 脑子初始化为0
		game_controler.clear_all_zombies(); // 删僵尸
		game_controler.clear_all_bullets(); // 删子弹
		game_controler.clear_not_colleted_sun(); // 删掉没收集的阳光
		logger.log("初始化第一关信息，并进行第一关的布阵", logger.DEBUG);


		// 按照 '/' 切分字符串
		std::vector<std::string> parts;
		std::istringstream iss(ls);
		std::string token;
		while (getline(iss, token, '.')) {
			parts.push_back(token);
		}
		// 提示主题
		logger.log("25个主题序号为：", Logger::INFO);
		int count = 0;
		std::ostringstream oss;
		for (const auto& part : parts) {
			count++;
			oss << part[0] << " ";
			if (count % 10 == 0) {
				oss << std::endl;
			}
		}
		logger.log(oss.str(), Logger::INFO);

		//game_controler.setInjectors();

		// 7.2 开始布阵
		auto layout_code = vec[0];
		int flower_num = code_generator.getSsb6FlowerNumDistribution()[0];
		game_controler.set_layout(layout_code, flower_num);



		// 8. 游戏主循环
		MSG msg = { 0 };
		while (true) {
			Sleep(1);

			// 添加快捷键并处理，全局热键消息
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_HOTKEY) {
					if (msg.wParam == 1) { // shift+q 强制结束
						auto over_time = TimeStruct::getNow() - start_time;
						logger.log(over_time.enPrint().append(" 提前结束游戏!"), Logger::INFO);
						logger.log(std::string(get_terminal_width(), '-'), Logger::INFO);

						std::ostringstream oss;
						oss << std::fixed << std::setprecision(1) << (std::round(leveldata.score * 10) / 10.0);
						std::string score_str = oss.str();
						logger.log("游戏结束! 最终得分为——  " + score_str, Logger::INFO);
						logger.log("最后吃脑时间为: " + (leveldata.last_brain_eaten_time - start_time).cnPrint(), Logger::INFO);

						auto timeStr = (leveldata.last_brain_eaten_time - start_time).enPrint().append("     ") + score_str;
						Creator::CreateCaption(timeStr.c_str(), timeStr.size() - 5, CaptionStyle::Lowermiddle);
						return;
					}
					if (msg.wParam == 2) { // shift+R重开
						game_controler.clear_not_colleted_sun();
						game_controler.update_brains();
						game_controler.clear_all_zombies();
						game_controler.clear_all_bullets();
						// 恢复阳光
						game_controler.board->Sun = leveldata.initial_sun;
						leveldata.released_zombies_count = 0;
						leveldata.zombie_cost = 0;

						TimeStruct restart_time = TimeStruct::getNow() - start_time;
						std::cout << restart_time.enPrint() << " 重开" << current_flag + 1 << "关!" << std::endl;
						std::string restart_str = restart_time.enPrint().append("     ").append(std::string("Restart"));
						Creator::CreateCaption(restart_str.c_str(), restart_str.size(), CaptionStyle::Lowermiddle); // 游戏白字，处于靠下居中位置

							
						// 重新布阵一下
						layout_code = vec[current_flag];
						std::cout << layout_code << std::endl;
						flower_num = code_generator.getSsb6FlowerNumDistribution()[current_flag];
						game_controler.set_layout(layout_code, flower_num);
					}
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}


			// 1. 监测崩溃

			// 2. 检测作弊
			if(is_cheat_check){
				do {// 先检测一次
					if ((TimeStruct::getNow() - check_time).second > check_interval) {
						game_controler.cheat_check(true, logger);
						check_time = TimeStruct::getNow();
						//logger.log("当前正在检测是否作弊！", logger.DEBUG);
					}
				} while (0);
			}


			// 2. 跨关更新
			if (game_controler.board->GetBaseAddress() &&
				game_controler.board->GetMiscellaneous()->Round != current_flag &&
				game_controler.pvz->GameState == PVZGameState::Playing) {

				if (game_controler.board->GetMiscellaneous()->Round >= 25) {
					logger.log(std::string(get_terminal_width(), '-'), Logger::INFO);
					logger.log(std::string("恭喜打通!!!!"), Logger::INFO);
					logger.log("最后吃脑时间为: " + (leveldata.last_brain_eaten_time - start_time).cnPrint(), Logger::INFO);

					auto timeStr = (leveldata.last_brain_eaten_time - start_time).enPrint().append("     ").append(std::string("Congrats!"));
					Creator::CreateCaption(timeStr.c_str(), timeStr.size(), CaptionStyle::Lowermiddle);

					logger.log("所有关卡数据如下:", logger.DEBUG);
					int count = 0;
					for (auto& leveldata : save_data) {
						logger.log("第" + std::to_string(++count) + "关数据如下:", Logger::DEBUG);
						logger.log("    放置的僵尸数: " + std::to_string(leveldata.released_zombies_count), Logger::DEBUG);
						logger.log("    僵尸花费: " + std::to_string(leveldata.zombie_cost), Logger::DEBUG);
						logger.log("    反应时间: " + leveldata.reaction_time.enPrint(), Logger::DEBUG);
						logger.log("    最后一个脑子吃的时间: " + (leveldata.last_brain_eaten_time - leveldata.setlayout_time).enPrint(), Logger::DEBUG);
						logger.log("    所有脑子吃的时间列表: ", Logger::DEBUG);

						for (auto& eat_brain_time : leveldata.brain_eaten_times) {
							logger.log((eat_brain_time - leveldata.setlayout_time).enPrint(), logger.DEBUG);
						}
					};
					return;
				}

				// 2.1 存档
				save_data.push_back(leveldata);
				// 2.2 打印数据
				logger.log((TimeStruct::getNow() - start_time).enPrint()
					+ " 已经通过" + std::to_string(game_controler.board->GetMiscellaneous()->Round)
					+ "关, 阳光" + std::to_string(game_controler.board->Sun)
					+ "，花费" + std::to_string(leveldata.zombie_cost),
					Logger::LogLevel::INFO);

				// 2.3 更新数据
				current_flag = game_controler.board->GetMiscellaneous()->Round;

				// 2.4 初始化下一关的数据
				leveldata.initial_sun = game_controler.board->Sun;
				leveldata.released_zombies_count = 0;
				leveldata.zombie_cost = 0;
				leveldata.kernelpult_butter_rate = 0.00;
				leveldata.score = current_flag;
				leveldata.setlayout_time = TimeStruct::getNow();
				leveldata.brain_eaten_times = {};

				// 布阵
				layout_code = vec[current_flag];
				flower_num = code_generator.getSsb6FlowerNumDistribution()[current_flag];
				game_controler.set_layout(layout_code, flower_num);
				logger.log("现在对第" + std::to_string(current_flag) + "关进行布阵,花数:" + std::to_string(flower_num) + ", 布阵码:" + ls, logger.DEBUG);

			}

			// 3. 检测是否开始游戏
			if (!has_started) {
				if (!game_controler.pvz->GetBaseAddress() ||
					game_controler.pvz->LevelId != PVZLevel::I_Zombie_Endless ||
					game_controler.pvz->GameState != PVZGameState::Playing)
					continue;

				// 判断释放僵尸条件
				if (game_controler.board->ZombiesCount != 1) continue;

				has_started = true;
				start_time = TimeStruct::getNow();
				// 开始时间
				leveldata.first_zombie_release_time = start_time;
				leveldata.reaction_time = leveldata.setlayout_time - leveldata.first_zombie_release_time;
				leveldata.last_brain_eaten_time = start_time;

				logger.log("开始游戏!  现实时间为: " + start_time.getCurrentTime(), Logger::LogLevel::INFO);
				logger.log(std::string(get_terminal_width(), '-'), Logger::LogLevel::INFO);
			}



			// 4. 监测放置的僵尸、释放时间以及计算花费、反应时间
			std::vector<SPT<PVZ::Zombie>> zombies = game_controler.board->GetAllZombies();
			for (auto& zombie : zombies) {
				// 如果僵尸死了，跳过
				if (zombie->NotExist) {
					processed_zombie_ids.erase(zombie->Id);
					continue;
				}
				// 如果不是刚放置的僵尸, 跳过
				if (zombie->ExistedTime < 0 || zombie->ExistedTime > 5) continue; // 只检查新生成的
				// 如果这个僵尸已经处理过了, 跳过
				if (processed_zombie_ids.count(zombie->Id)) continue; // 已处理则跳过

				processed_zombie_ids.insert(zombie->Id); // 记录已处理

				// 如果不是ize中的僵尸，而且现在又不是开局的话, 记录下来
				if (!game_controler.ZombieSunCost.count(zombie->Type)) {
					// 重开的话需要先清除选卡界面的僵尸
					logger.log("检测到在第" + std::to_string(zombie->Row + 1) + "行放置了非ize关卡的僵尸,僵尸类型为" + ZombieType::ToString(zombie->Type), logger.DEBUG);					
					continue;
				}
				// 如果是ize中的僵尸，但又不是伴舞僵尸，日志记录并且计算花费【每一关结束赋值】
				if (zombie->Type == ZombieType::BackupDancer) continue;

				auto zombie_info = game_controler.ZombieSunCost[zombie->Type];
				leveldata.zombie_cost += zombie_info.first;
				leveldata.released_zombies_count += 1;
				logger.log("在第" + std::to_string(zombie->Row + 1) + "行放置了" + std::string(zombie_info.second) + ",目前一共放了" + std::to_string(leveldata.released_zombies_count) + "个僵尸", logger.DEBUG);

				// 记录反应时间
				if (leveldata.released_zombies_count == 1) {
					leveldata.first_zombie_release_time = TimeStruct::getNow();
					leveldata.reaction_time = leveldata.first_zombie_release_time - leveldata.setlayout_time;
					logger.log("第" + std::to_string(current_flag + 1) + "关，第一个僵尸释放时间: " + (leveldata.first_zombie_release_time - start_time).enPrint(), logger.DEBUG);
					logger.log("第" + std::to_string(current_flag + 1) + "关，反应时间: " + leveldata.reaction_time.enPrint(), logger.DEBUG);
				}
			}

			// 5. 监测脑子变化
			if (std::abs(leveldata.score - (current_flag + game_controler.countEatenBrain() * 0.2)) > 1e-2) {
				leveldata.score = game_controler.board->GetMiscellaneous()->Round + game_controler.countEatenBrain() * 0.2;
				leveldata.last_brain_eaten_time = TimeStruct::getNow();
				leveldata.brain_eaten_times.push_back(leveldata.last_brain_eaten_time);

				logger.log((leveldata.last_brain_eaten_time - start_time).enPrint() + "吃了第" + std::to_string(game_controler.countEatenBrain()) + "个脑子", logger.DEBUG);
			}

			// 6. 超时
			if ((TimeStruct::getNow() - start_time).minute >= 30)
			{
				auto over_time = TimeStruct::getNow() - start_time;
				logger.log(std::string(get_terminal_width(), '-'), Logger::INFO);

				logger.log("超时，游戏结束!", Logger::DEBUG);

				std::ostringstream oss;
				oss << std::fixed << std::setprecision(1) << (std::round(leveldata.score * 10) / 10.0);
				std::string score_str = oss.str();
				logger.log("游戏结束! 最终得分为——  " + score_str, Logger::INFO);
				logger.log("最后吃脑时间为: " + (leveldata.last_brain_eaten_time - start_time).cnPrint(), Logger::INFO);

				logger.log("所有关卡数据如下:", logger.DEBUG);
				int count = 0;
				for (auto& leveldata : save_data) {
					logger.log("第" + std::to_string(++count) + "关数据如下:", Logger::DEBUG);
					logger.log("    放置的僵尸数: " + std::to_string(leveldata.released_zombies_count), Logger::DEBUG);
					logger.log("    僵尸花费: " + std::to_string(leveldata.zombie_cost), Logger::DEBUG);
					logger.log("    反应时间: " + leveldata.reaction_time.enPrint(), Logger::DEBUG);
					logger.log("    最后一个脑子吃的时间: " + (leveldata.last_brain_eaten_time - leveldata.setlayout_time).enPrint(), Logger::DEBUG);
					logger.log("    所有脑子吃的时间列表: ", Logger::DEBUG);

					for (auto& eat_brain_time : leveldata.brain_eaten_times) {
						logger.log((eat_brain_time - leveldata.setlayout_time).enPrint(), logger.DEBUG);
					}
				};
				return;
			}

			// 6. 还有时间，阳光用完了
			if (game_controler.board->ZombiesCount == 0 && game_controler.board->Sun < lowestSun) {
				bool is_dead = true;

				for (auto& coin : game_controler.board->GetAllCoins()) {
					if (coin->Type == CoinType::NormalSun) is_dead = false;
				}
				if (is_dead) {
					auto time_str = (leveldata.last_brain_eaten_time - start_time).enPrint().append("     ").append(std::to_string(leveldata.score));
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << (std::round(leveldata.score * 10) / 10.0);
					std::string score_str = oss.str();

					logger.log(std::string(get_terminal_width(), '-'), Logger::INFO);
					logger.log((leveldata.last_brain_eaten_time - start_time).enPrint() + " 游戏结束--  得分: " + score_str, Logger::INFO);

					Creator::CreateCaption(time_str.c_str(), time_str.size() - 5, CaptionStyle::Lowermiddle); // 去掉浮点数的两位小数

					logger.log("所有关卡数据如下:", logger.DEBUG);
					int count = 0;
					for (auto& leveldata : save_data) {
						logger.log("第" + std::to_string(++count) + "关数据如下:", Logger::DEBUG);
						logger.log("    放置的僵尸数: " + std::to_string(leveldata.released_zombies_count), Logger::DEBUG);
						logger.log("    僵尸花费: " + std::to_string(leveldata.zombie_cost), Logger::DEBUG);
						logger.log("    反应时间: " + leveldata.reaction_time.enPrint(), Logger::DEBUG);
						logger.log("    最后一个脑子吃的时间: " + (leveldata.last_brain_eaten_time - leveldata.setlayout_time).enPrint(), Logger::DEBUG);
						logger.log("    所有脑子吃的时间列表: ", Logger::DEBUG);

						for (auto& eat_brain_time : leveldata.brain_eaten_times) {
							logger.log((eat_brain_time - leveldata.setlayout_time).enPrint(), logger.DEBUG);
						}
					};
					return;
				}
			}
		}


		
	}

	// 布阵器循环
	void main() {
		//布阵器实现
		setlocale(LC_ALL, ".936"); // 设置编码格式
		SetConsoleTitle(WINDOW_NAME);


		while (true) {
			std::cout << std::string(get_terminal_width(), '*') << std::endl;
			std::cout << INIT_WORDS << std::endl;
			std::cout << std::string(get_terminal_width(), '*') << std::endl;


			std::string s;
			std::cin >> s;
			/*1：【练习模式】布阵，30min限时玩法\n\
			5: 【比赛模式】生成当前电脑机器码\n\
			6：【比赛模式】裁判生成随机阵型代码\n\
			7：【比赛模式】布阵\n\
			8：娱乐模式\n\
			9：弹出工具-磁铁倒计时\n\
			a：生成25关随机阵型代码【自定义花数分布】\n\
			b：布阵，不限时玩法【可以布1-n个阵】
			c：导出本关ize阵型代码\n\
			0：使用说明*/
			if (!s.compare("0")) {// 使用说明
				std::cout << "1.布阵: 根据拿到的25关布阵码进行布阵，限时30min，最高25关；提供日志记录，log文件, 在exe文件目录下IZESpeedLayoutDatas/\n\
	(1)输入合法后会提示主题序号；\n\
	(2)快捷键：a)shift+Q/q 强制结束 (2)shift+R/r重开当前关卡\n\
2.生成随机的25关布阵码:  \n\
	(1)除去B类阵外分布外，其余与原版完全一致；\n\
	(2)1-8花A类与1-7花B类均有可能出现（六届不会出现1花胆小）\n\
	(4)对于B类，每5关出现一个，前3个B类出现一个胆小【调整后现在可出现1花胆小】；\n\
	(4)花数分布与六届手速杯规则一致,为876554433211223+；\n\
3.【还没写好】残局玩法:\n\
4.【勉强能用，还得补反作弊和禁女仆】不限时冲关：与正常冲关无异，旨在提供一些数据便于玩家后续复盘；\n\
	(1)快捷键：a)shift+A 切换自动收集 2)shift+D 切换加速，每关开始会恢复原速 3)shift+Q 强制退出 4)shift+J 跳关，用于卡礼物然后无法过关的情况\n\
5.生成本台电脑机器码: 把机器码提交给裁判；两台电脑不可能出现一样的；但不保证本电脑会不会变（，不过正常玩家也不会随便变动啊\n\
6.生成仅供两台电脑使用的布阵码：使用流程如下\n\
	(1)裁判从玩家除拿到两个机器码 \n\
	(2)生成加密布阵码后，粘贴发给玩家\n\
	(3)再拿到解密密钥后，粘贴发给玩家\n\
7.比赛模式布阵：额外开启反作弊检测；也会提示主题序号；使用流程如下\n\
	(1)拿到加密布阵码输入布阵器；\n\
	(2)输入密钥解密布阵码；\n\
	后正常游戏\n\
8.【还没写好】娱乐模式：什么消消乐模式（，考验眼力和反应力\n\
9.【还没写好】弹出磁铁倒计时: 类似雪线那种\n\
a.【还没写好】生成自定义花数分布随机码：\n\
b.【还没写好】复盘模式布阵：与正常布阵无异，但是加了很多方便的快捷键\n\
c.【还没写好】导出本关ize阵型代码：方便捏码"
<< std::endl;
			}
			else if (!s.compare("1")) { // 30min限时玩法
				std::cout << "请输入布阵码: " << std::endl;
				std::string ls;
				std::cin >> ls;


				// 添加快捷键
				register_SpeedRun_hotkey();
				// 30min限时玩法
				SpeedRun30min(ls, false);//默认不开启反作弊
				// 注销全局热键
				unregister_SpeedRun_hotkey();

			}
			else if (!s.compare("2")) { // 2：生成25关随机阵型代码
				GenerateLayoutCode code_generator;
				auto ls = code_generator.generate_ssb6_code();
				std::cout << ls << std::endl;
				copyToClipBoard(ls);
				std::cout << "已复制到剪贴板,可直接粘贴使用" << std::endl;
				continue;
			}
			else if (!s.compare("3")) { // 残局玩法

			}

			else if (!s.compare("4")) {
				// 添加快捷键shift+指令【自动收集A；加速D；强制退出Q；跳关J；女仆M】
				register_LevelRush_hotkey();
				// 设置要求
				std::cout << "是否开启反作弊(1为开启, 0为关闭): " << std::endl;
				std::string cmd1;
				std::cin >> cmd1;
				bool is_cheat_check = !cmd1.compare("1") ? true : false;
				std::cout << "有女仆输入1(1为开启, 0为关闭): " << std::endl;
				std::string cmd2;
				std::cin >> cmd2;
				bool is_ban_mj = !cmd2.compare("1") ? false : true;

				// 开始冲关
				LevelRush(is_cheat_check, is_ban_mj);
				// 注销全局热键
				unregister_LevelRush_hotkey();
			}

			/*1. 玩家AB分别生成自己电脑的机器码。【已实现】
			2. 加密布阵码过程（裁判执行）：
			2.1 生成密钥：裁判利用两个机器码利用特定密钥如“xiaofeng”进行加密得到K作为加密密钥。
			2.2 加密消息：使用K对目标字符串进行对称加密，得到密文C。

			3. 玩家后续操作
			3.1 玩家A和B拿到K和C后，根据特定密钥进行解密，拿到两个机器码组合的内容
			3.2 玩家电脑根据解密后的内容判断是否包含本机机器码，如果有的话，使用K对C进行解密拿到C的明文
			*/
			else if (!s.compare("5")) { // 生成机器码A和B
				auto machine_code = EncryptUtils::generate_machine_code();
				std::cout << machine_code << std::endl;
				copyToClipBoard(machine_code);
				std::cout << "本机机器码已复制到剪贴板,请私发给裁判" << std::endl;
				continue;
			}
			else if (!s.compare("6")) {
				std::cout << "请输入机器码A: " << std::endl;
				std::string machine_codeA;
				std::cin >> machine_codeA;
				//std::cout << "已经拿到机器码A" << machine_codeA << std::endl;

				std::cout << "请输入机器码B: " << std::endl;
				std::string machine_codeB;
				std::cin >> machine_codeB;
				//std::cout << "已经拿到机器码A" << machine_codeB << std::endl;


				std::string key = EncryptUtils::sha256("xiaofeng");
				auto combie_machine_code = machine_codeA + "/" + machine_codeB;
				//std::cout << combie_machine_code << std::endl;

				std::string layout_code_key = EncryptUtils::aes128ECBEncrypt(combie_machine_code, key);
				//std::cout << layout_code_key << std::endl;

				GenerateLayoutCode code_geneator;
				auto ls = code_geneator.generate_ssb6_code();
				//std::cout << ls << std::endl;
				auto enc_ls = EncryptUtils::aes128ECBEncrypt(ls, layout_code_key);

				copyToClipBoard(enc_ls);
				std::cout << enc_ls << std::endl;
				std::cout << "加密布阵码已复制到剪贴板,请私发给选手" << std::endl;

				std::cout << "输入1拿解密密钥: " << std::endl;
				std::string cmd;
				std::cin >> cmd;

				copyToClipBoard(layout_code_key);
				std::cout << layout_code_key << std::endl;
				std::cout << "用于解密的密钥已复制到剪贴板,请私发给选手" << std::endl;
			}
			else if (!s.compare("7")) {
				std::cout << "请输入被加密的布阵码: " << std::endl;
				std::string enc_ls;
				std::cin >> enc_ls;

				std::cout << "请输入解密密钥:" << std::endl;
				std::string layout_code_key;
				std::cin >> layout_code_key;

				std::string key = EncryptUtils::sha256("xiaofeng");
				auto machine_code = EncryptUtils::generate_machine_code();
				std::string combie_machine_code;
				try { // 尝试解密密钥
					combie_machine_code = EncryptUtils::aes128ECBDecrypt(layout_code_key, key);
				}
				catch (...) {  // 捕获异常，异常类型可以根据实际情况设定
					std::cout << "解密密钥出错!" << std::endl;
					continue;
				}


				size_t pos = combie_machine_code.find('/');
				if (pos == std::string::npos) {
					std::cout << "解密密钥出错!" << std::endl;
					continue;
				}
				std::string machine_code_1 = combie_machine_code.substr(0, pos);
				std::string machine_code_2 = combie_machine_code.substr(pos + 1);
				if (machine_code_1.find(machine_code) == std::string::npos && (machine_code_2.find(machine_code) == std::string::npos)) {
					std::cout << "机器码异常!" << std::endl;
					continue;
				}

				// 玩家解密
				std::string ls;
				try { // 尝试解密布阵码
					ls = EncryptUtils::aes128ECBDecrypt(enc_ls, layout_code_key);
				}
				catch (...) {  // 捕获异常，异常类型可以根据实际情况设定
					std::cout << "解密密钥出错!" << std::endl;
					continue;
				}

				// 添加快捷键
				register_SpeedRun_hotkey();
				// 30min限时玩法
				SpeedRun30min(ls, true);
				// 注销全局热键
				unregister_SpeedRun_hotkey();


			}
			else if (!s.compare("8")) {

			}
			else if (!s.compare("9")) { // 3. 【每关计时】不限时冲关


			}
			else if (!s.compare("a")) {

			}

			else if (!s.compare("b")) {

			}

			else if (!s.compare("c")) {

			}


		}
	}


	
};








int main() {


	//LayoutControler layout_controler;
	//layout_controler.main();

	DWORD pid = ProcessOpener::Open();
	GameControl game_controler(pid);


	return 0;
};
