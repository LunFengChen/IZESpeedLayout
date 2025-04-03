#include "pvzclass.h"

// 加密类：不公布
#include "EncryptUtils.h"
// 汇编类【参考izc和pt】
#include "AsmCode.h"
// 还是汇编类【参考六届布阵器】
#include "iMemory.hpp"
// 布阵码生成器类
#include "GenerateLayoutCode.h"
// 日志记录类
#include "Logger.h"
// 关卡数据类
#include "LevelData.h"
// 时间类【参考六届布阵器】
#include "TimeStruct.h"

// 全局随机数
std::random_device rd;
std::mt19937_64 gen(rd()); // 全局随机数生成器


// 反作弊检测
class GameCheatCheck {
private:

	struct WindowInfo {
		DWORD pid;
		std::wstring title;
	};
	std::vector<WindowInfo> windows;

	// 用于窗口遍历的回调函数
	static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
		std::vector<WindowInfo>* pWindows = reinterpret_cast<std::vector<WindowInfo>*>(lParam);

		// 过滤不可见窗口
		if (!IsWindowVisible(hwnd)) return TRUE;

		// 获取窗口标题
		wchar_t titleBuffer[256];
		if (GetWindowTextW(hwnd, titleBuffer, 256) == 0) return TRUE; // 无标题跳过

		// 获取进程ID
		DWORD pid;
		GetWindowThreadProcessId(hwnd, &pid);

		// 添加到列表
		pWindows->push_back({ pid, titleBuffer });

		return TRUE; // 继续遍历
	}

	// 将宽字符串转换为 UTF-8 编码的窄字符串
	std::string WideToUTF8(const std::wstring& wstr) {
		if (wstr.empty()) return "";

		int size_needed = WideCharToMultiByte(
			CP_UTF8, 0,
			wstr.c_str(), (int)wstr.size(),
			nullptr, 0, nullptr, nullptr
		);

		std::string str(size_needed, 0);
		WideCharToMultiByte(
			CP_UTF8, 0,
			wstr.c_str(), (int)wstr.size(),
			&str[0], size_needed, nullptr, nullptr
		);

		return str;
	}

	// 将宽字符串转换为系统本地编码的窄字符串
	std::string WideToANSI(const std::wstring& wstr) {
		if (wstr.empty()) return "";

		int size_needed = WideCharToMultiByte(
			CP_ACP, 0,
			wstr.c_str(), (int)wstr.size(),
			nullptr, 0, nullptr, nullptr
		);

		std::string str(size_needed, 0);
		WideCharToMultiByte(
			CP_ACP, 0,
			wstr.c_str(), (int)wstr.size(),
			&str[0], size_needed, nullptr, nullptr
		);

		return str;
	}

	// 检查游戏速度是否异常
	bool check_speed() {
		// 检测修改帧间隔加速
		int time_ms = PVZ::Memory::ReadMemory<int>(PVZ::Memory::ReadMemory<int>(0x6a9ec0) + 0x454);
		if (time_ms != 10) {
			logger->log("检测到速度异常，帧间隔异常，" + std::to_string(time_ms), Logger::DEBUG);
			return true;
		}

		// pvz自带加速: 25px, 0.25px
		if (PVZ::Memory::ReadMemory<bool>(0x6A9EAB) || PVZ::Memory::ReadMemory<bool>(0x6A9EAA)) {
			logger->log("检测到速度异常，可能启用了20px和0.25px", Logger::DEBUG);
			return true;
		}

		// 检测iz布阵器的那种超级加速


		return false;
	}

	// 检测相对速度常量是否被修改
	bool check_speed_constant() {

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
				logger->log(
					std::string(check.name)
					+ "被锁定！原数据: " + std::to_string(check.expected_value)
					+ "修改后: " + std::to_string(check.current_value)
					, Logger::DEBUG
				);
				data_changed = true;  // 只要有一个不匹配就返回true
			}
		}

		return data_changed;  // 如果所有的值都符合预期，返回false

	}

	// 检测是否开启免费种植
	bool check_free_plant() {
		if (PVZ::Memory::ReadPointer(0x6a9ec0, 0x814)) {
			logger->log("检测到开启了免费种植!", Logger::DEBUG);
			return true;
		}

		return false;
	}
	
	// 检测是否锁玉米或者黄油
	bool check_Kernelpult() {
		if (PVZ::Memory::ReadMemory<byte>(0x45F1EC) == byte(235)) {
			logger->log("检测到锁玉米粒!", Logger::DEBUG);
			return true;
		}
		if (PVZ::Memory::ReadMemory<byte>(0x45F1EC) == byte(112)) {
			logger->log("检测到锁黄油!", Logger::DEBUG);
			return true;
		}
		if (PVZ::Memory::ReadMemory<byte>(0x45F1EC) != byte(117)) {
			logger->log("检测到玉米子弹异常!", Logger::DEBUG);
			return true;
		}
		

		return false;
	}

	// 检测是否开启dance
	bool check_dance() {
		auto board = PVZ::GetBoard();
		if (board->Dance) { 
			logger->log("检测到开启dance!, "+std::to_string(PVZ::Memory::ReadMemory<bool>(PVZ::Memory::ReadPointer(0x6A9EC0, 0x768) + 0x5765)), Logger::DEBUG);
			return true;
		}
		return false;
	}

	// rnd检测
	bool check_rnd() {
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
				logger->log(
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
				logger->log(
					std::string(check.name)
					+ "被锁定! "
					, Logger::DEBUG
				);
				data_changed = true;  // 只要有一个不匹配就返回true
			}
		}

		return data_changed;  // 如果所有的值都符合预期，返回false

	}

	// 检测是否使用过铲子
	bool check_shovel() {
		if (PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x579c) != 0) {
			logger->log("铲过植物数量: " + std::to_string(PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x579c)), Logger::DEBUG);
			return true;
		}
		return false;
	}

	// 检测僵尸速度分布
	bool check_zombie_speed() {
		if (PVZ::Memory::ReadMemory<byte>(0x52b215) != byte(117)) {
			logger->log("开启僵尸速度加快", Logger::DEBUG);
			return true;
		}
		if (PVZ::Memory::ReadMemory<short int>(0x0052F103) != 21620) {
			logger->log("开启僵尸速度更快", Logger::DEBUG);
			return true;
		}
		if (PVZ::Memory::ReadMemory<byte>(0x52aaad) != byte(116)) {
			logger->log("开启僵尸匀速前进", Logger::DEBUG);
			return true;
		}
		if (PVZ::Memory::ReadMemory<byte>(0x00531045) != byte(200)) {
			logger->log("僵尸状态异常: 疑似开了僵尸无敌", Logger::DEBUG);
			return true;
		}
		return false;
	}

	// 将测僵尸状态
	bool check_zombie_status() {
		if (PVZ::Memory::ReadMemory<byte>(0x0053095C) != byte(132)) {
			logger->log("开启僵尸免疫减速", Logger::DEBUG);
			return true;
		}
		if (PVZ::Memory::ReadMemory<byte>(0x00531A1A) != byte(116)) {
			logger->log("开启僵尸免疫黄油", Logger::DEBUG);
			return true;
		}
		if (PVZ::Memory::ReadMemory<byte>(0x004620C5) != byte(2)
			|| PVZ::Memory::ReadMemory<byte>(0x004620CA) != byte(3)
			|| PVZ::Memory::ReadMemory<byte>(0x004620D5) != byte(1)
			|| PVZ::Memory::ReadMemory<byte>(0x004620DA) != byte(3)
			|| PVZ::Memory::ReadMemory<byte>(0x004620DF) != byte(15)
			|| PVZ::Memory::ReadMemory<byte>(0x004620ED) != byte(0)
			) {
			logger->log("开启僵尸免疫磁力菇", Logger::DEBUG);
			return true;
		}
		if (PVZ::Memory::ReadMemory<byte>(0x004248AA) != byte(117)) {
			logger->log("开启僵尸快跑", Logger::DEBUG);
			return true;
		}



		return false;
	}

	// 检测植物状态
	bool check_plant_status() {
		if (PVZ::Memory::ReadMemory<byte>(0x0045EE0A) != byte(117)
			|| PVZ::Memory::ReadMemory<byte>(0x0052FCF3) != byte(252)
			|| PVZ::Memory::ReadMemory<byte>(0x0052FCF1) != byte(70)
			) {
			logger->log("开启植物虚弱", Logger::DEBUG);
			return true;
		}
		return false;
	}

	// 检测是否开启自动收集
	bool check_auto_collected() {
		if (PVZ::Memory::ReadMemory<byte>(0x0043158f) != 0x75) {
			logger->log("检测到开启自动收集", Logger::DEBUG);
			PVZ::Memory::WriteMemory<byte>(0x0043158f, 0x75);
			return true;
		}
		return false;
	}

	// 扫描内存中常修改的数据
	bool check_memory() {
		// 游戏速度
		if (check_speed()) {
			logger->log("检测到速度异常", Logger::DEBUG);
			return true;
		}
		// 自动收集
		else if (check_auto_collected()) {
			logger->log("检测到开启自动收集", Logger::DEBUG);
			return true;
		}
		// 速度常量
		else if (check_speed_constant()) {
			logger->log("检测到速度常量异常", Logger::DEBUG);
			return true;
		}
		// 锁玉米黄油
		else if (check_Kernelpult()) {
			logger->log("检测到玉米异常", Logger::DEBUG);
			return true;
		}
		// 是否使用rnd修改随机数
		else if (check_rnd()) {
			logger->log("检测到随机数异常", Logger::DEBUG);
			return true;
		}
		// 是否使用过铲子
		else if (check_shovel()) {
			logger->log("检测使用过铲子", Logger::DEBUG);
			PVZ::Memory::WriteMemory(PVZ::Memory::ReadPointer(0x6a9ec0, 0x768) + 0x579c, 0);//报过一次作弊后就改回0
			return true;
		}
		// 是否开启dance
		else if (check_dance()) {
			logger->log("检测出dance", Logger::DEBUG);
			return true;
		}
		// 是否开启免费种植
		else if (check_free_plant()) {
			logger->log("检测出免费种植", Logger::DEBUG);
			return true;
		}
		// 僵尸速度状态是否异常：匀速，变速什么的
		else if (check_zombie_speed()) {
			logger->log("检测出僵尸速度异常", Logger::DEBUG);
			return true;
		}
		// 僵尸状态是否异常：无敌，免疫
		else if (check_zombie_status()) {
			logger->log("检测出僵尸状态异常", Logger::DEBUG);
			return true;
		}
		// 植物状态是否异常：被秒吃，虚弱
		else if (check_plant_status()) {
			logger->log("检测出僵尸状态异常", Logger::DEBUG);
			return true;
		}
		return false;
	}

	// 扫描后台进程: 带窗口的, 只要是看信息就会有显示信息的
	void log_background_process() {
		// 监测后台进程是否有算血器：常规信息中的描述为"IZECalculatorV1.5.10.exe"
		// 监测后台进程是否有IZ布阵器：常规信息中的描述为"IZ_Format_Designer_V2"
		// 监测后台进程是否有Rnd：常规信息中的描述为"Rnd_3_4.exe"
		// 监测后台进程是否有pt：常规信息中的描述为"PvZ Tools"
		// 监测后台进程是否有ptk：常规信息中的描述为"PvZ Toolkit"
		// 监测后台进程是否有终极修改器：常规信息中的描述为"PVZWPF修改器"

		EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&windows));

		// 去重（按进程ID和标题）
		std::unordered_set<std::wstring> uniqueTitles;
		for (const auto& win : windows) {
			if (!win.title.empty() && uniqueTitles.insert(win.title).second) {
				logger->log("应用标题: " + WideToANSI(win.title) + " (PID: " + std::to_string(win.pid) + ")", Logger::DEBUG);
			}
		}
	}

	// 记录植物和僵尸数据
	void log_zombie_plant() {
		// 记录所有僵尸移速和血量
		for (auto zombie : PVZ::GetBoard()->GetAllZombies()) {
			if (!zombie->NotExist) {
				int bodyhp, max_bodyhp;
				zombie->GetBodyHp(&bodyhp, &max_bodyhp);
				PVZ::Zombie::AccessoriesType1 acctype1 = zombie->GetAccessoriesType1();
				PVZ::Zombie::AccessoriesType2 acctype2 = zombie->GetAccessoriesType2();

				logger->log(
					"第" + std::to_string(zombie->Row + 1) + "行, 坐标为:" + std::to_string(zombie->ImageX) + ", 栈位为:" + std::to_string(zombie->Index) + "的" + ZombieType::ToString(zombie->Type)
					+ "速度为: " + std::to_string(zombie->Speed) + "僵尸本体血量: " + std::to_string(bodyhp)
					+ ", 一类防具: " + std::to_string(acctype1.Hp) + "/" + std::to_string(acctype1.MaxHp)
					+ ", 二类防具: " + std::to_string(acctype2.Hp) + "/" + std::to_string(acctype2.MaxHp)
					+ ", 存在时间: " + std::to_string(zombie->ExistedTime)
					, Logger::DEBUG
				);
			}
		}

		// 记录植物血量: 用于检测血量是否异常
		for (auto plant : PVZ::GetBoard()->GetAllPlants()) {
			if (!plant->NotExist) {
				logger->log(
					"第" + std::to_string(plant->Row + 1) + "行, 坐标为:" + std::to_string(plant->ImageX) + ", 栈位为:" + std::to_string(plant->Index) + "的" + PlantType::ToString(plant->Type)
					+ "hp为: " + std::to_string(plant->Hp) + ", 最大血量为: " + std::to_string(plant->MaxHp) + ", 属性倒计时(一般是磁力菇): " + std::to_string(plant->AttributeCountdown)
					, Logger::DEBUG
				);
			}
		}
	}

	// 记录鼠标数据
	void log_mouse() {
		logger->log(PVZ::Memory::ReadPointer(0x6a9ec0, 0x320, 0xdc) ? "鼠标移出屏幕" : "鼠标还在屏幕内", Logger::DEBUG);
		logger->log("鼠标坐标: " + std::to_string(PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x5558)) + "-" + std::to_string(PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x555c)), Logger::DEBUG);
	}
public:
	int check_interval;
	Logger* logger;

	// 类实例化：检测间隔(s)，日志记录类引用
	GameCheatCheck(int interval, Logger* logger_ref)
		: check_interval(interval), logger(logger_ref) // 初始化列表
	{
	}

	// 检测和扫描所有可疑项目：游戏速度，内存数据，植物和僵尸数据，后台进程
	bool check_all() {
		// 扫描鼠标
		log_mouse();
		// 扫描植物
		log_zombie_plant(); 
		
		// 扫描后台进程
		log_background_process();

		if (check_memory()) {
			logger->log("检测出内存异常!", Logger::DEBUG);
			return true;
		}

		return false;
	}




	void check_envirnoment() {
		if (check_all()) {
			logger->log("当前pvz环境检测结果: 异常!", Logger::INFO);
			return;
		}
		else {
			logger->log("当前pvz环境检测结果: 正常，请继续游戏!", Logger::INFO);
		}
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

	// 清除重开时的僵尸
	void remove_cutscene_zombie() {
		if (is_in_ize()) {
			AsmCode code;
			code.asm_init();

			code.asm_push_exx(AsmCode::Reg::EBX);
			code.asm_mov_exx_dword_ptr(AsmCode::Reg::EBX, PVZ::Memory::ReadPointer(0x6a9ec0, 0x768));
			code.asm_call(0x40df70);
			code.asm_pop_exx(AsmCode::Reg::EBX);
			code.asm_ret();

			code.asm_code_inject(PVZ::Memory::hProcess);
		}
	}

	// 拿到玩家名字
	std::string get_player_name() {
		int name_length = PVZ::Memory::ReadPointer(0x6a9ec0, 0x82c, 0x14);
		// 按照地址依次读name_length个字节，每个字节转十六进制再-'0'
		std::stringstream ss;
		for (int i = 0; i < name_length; i++) {
			int value = PVZ::Memory::ReadMemory<byte>(PVZ::Memory::ReadPointer(0x6a9ec0, 0x82c) + 0x4 + i );
			ss << static_cast<char>(value);
			
		}
		return ss.str();
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

	// 种子流布阵
	void set_layout(const std::string& ls, int flower_num) {
		if (is_in_ize()) {
			// ls = "1/2130778634";
			int theme = static_cast<int>(ls[0] - '0');
			size_t seed = std::stoull(ls.substr(2));

			auto plantTypes = GenerateLayoutCode::get_theme_plants(flower_num, static_cast<Theme>(theme)); // 获取不同主题的植物生成顺序
			auto orders = GenerateLayoutCode::get_shuffled_array(seed);
			//for (auto it : orders) std::cout << it << " ";


			////调用游戏刷新函数0x41ca10:(1)先写入字节数组(2)后写入标志位（3）游戏根据标志位，读写入的字节数组
			//for (size_t i = 0; i < 25; i++) // 把植物塞到内存中
			//{//
			//	int written[] = { (orders[i]) / 5 , (orders[i]) % 5 ,static_cast<int>(plantTypes[i]) };
			//	PVZ::Memory::WriteArray<int>(0x6b1200 + 12 * i, written, 12);
			//}
			//Sleep(20);
			//PVZ::Memory::WriteMemory<byte>(0x6b0905, 3); // 调用游戏自身的布阵功能
			////std::cout << "等游戏刷新植物" << std::endl;
			//Sleep(20);


			// 汇编一次种
			spawn_all_plants(plantTypes, orders);


			// 设置小喷偏移
			std::mt19937 genPuffshroom(seed + 1);
			std::uniform_int_distribution<int> rngPuffshroomX(-5, 4);
			std::uniform_int_distribution<int> rngPuffshroomY(-3, 2);

			for (auto& plant : board->GetAllPlants()) {
				if (plant->Type == PlantType::Puffshroom) {
					plant->ImageX = 40 + 80 * plant->Column + rngPuffshroomX(genPuffshroom);
					plant->ImageY = 80 + 100 * plant->Row + rngPuffshroomX(genPuffshroom);
					//std::cout << "修改了小喷偏移" << plant->ImageX << "/" << plant->ImageY;
				};
			};


			// TODO: 种完了截图

		};
	}

	// 植物种植位置列表布阵
	void set_layout_test(const std::string& ls, const int theme_index, const int flower_num, const int sun, const std::array<int, 25>orders) {
		if (is_in_ize()) {
			// 获得种植顺序
			// 获得种植位置
			auto plantTypes = GenerateLayoutCode::get_theme_plants(flower_num, static_cast<Theme>(theme_index)); // 获取不同主题的植物生成顺序
			if (sun != 0) {
				PVZ::GetBoard()->Sun = sun;
			}
			// 汇编一次种
			spawn_all_plants(plantTypes, orders);

			// 设置小喷偏移
			std::mt19937 genPuffshroom(theme_index * 66 + flower_num * 88);
			std::uniform_int_distribution<int> rngPuffshroomX(-5, 4);
			std::uniform_int_distribution<int> rngPuffshroomY(-3, 2);

			for (auto& plant : board->GetAllPlants()) {
				if (plant->Type == PlantType::Puffshroom) {
					plant->ImageX = 40 + 80 * plant->Column + rngPuffshroomX(genPuffshroom);
					plant->ImageY = 80 + 100 * plant->Row + rngPuffshroomX(genPuffshroom);
				};
			};

			// TOdo: 截图


		}



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
		if (is_in_ize()) return PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x160, 0x60);
	}

	// 设置脑子数
	void set_EatenBrains(int value) {
		if (is_in_ize()) PVZ::Memory::WriteMemory(PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x160) + 0x60, value);
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

	//
	void clear_reverse_all_plants() {
		// 逆序删植物
		if (is_in_ize()) {
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
		}
	}

	// 禁原版种植
	void stop_game_plant() {
		if (is_game_on()) {
			int written[] = { 0xc2, 0x0c, 0x00 };
			PVZ::Memory::WriteArray<int>(0x42A6C0, written, 3);
		}
	}

	// 去掉种植物音效
	void disable_plant_effect() {
		if (is_game_on()) {
			int written[] = { 0xc2, 0x04, 0x00 };
			PVZ::Memory::WriteArray<int>(0x40ce60, written, 3);

			AsmCode code;
			code.asm_init();

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
		for (int i = 0; i < 25; i++) {
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
	void ban_maidCheat(bool is_maidCheat, Logger& logger) {
		if (is_in_ize()) {

		//if(is_maidCheat) { // 开启禁用女仆功能
		//		logger.log("已禁用女仆!", Logger::INFO);
		//		byte disable_MaidCheat_code[] = { 0x68, 0x07, 0x00, 0x00, 0x8b, 0x80, 0x68, 0x55, 0x00, 0x00, 0x99, 0xf7, 0xf9, 0x8b, 0xc2, 0x99, 0xf7, 0xfe, 0x5e, 0xc3 };
		//		PVZ::Memory::WriteArray<byte>(0x52dfcb, disable_MaidCheat_code, 20);
		//		
		//}
		//else {
		//	logger.log("女仆状态: 正常!", Logger::INFO);
		//	byte enable_MaidCheat_code[] = { 0x38 ,0x08 ,0x00 ,0x00 ,0x99 ,0xF7 ,0xF9 ,0x8B ,0xC2 ,0x99 ,0xF7 ,0xFE ,0x5E ,0xC3 ,0xCC ,0xCC ,0xCC ,0xCC ,0xCC ,0xCC };
		//	PVZ::Memory::WriteArray<byte>(0x52dfcb, enable_MaidCheat_code, 20);
		//}
		}
		}

	// 汇编恢复女仆
	void enable_maidCheat() {
		std::vector<Injector*> injectors = {};

		//禁用女仆： 汇编修改时钟增加判定（游戏不暂停才增加）【勺12138提供】
		Injector* disable_maidCheat2 = new Injector(0x4526E6, { 0x01, 0xaf, 0x38, 0x08, 0x00, 0x00 });
		injectors.push_back(disable_maidCheat2);

		for (size_t i = 0; i < injectors.size(); i++)
		{
			if (injectors[injectors.size() - i - 1] != nullptr)
			{
				injectors[injectors.size() - i - 1]->effect();
				Sleep(1);
			}
			else
			{
				// 处理空指针的情况，例如记录日志或抛出异常
				//std::cerr << "Error: Null pointer in injectors vector at index " << injectors.size() - i - 1 << std::endl;
			}
		}
	}

	// 汇编禁用女仆
	void disable_maidCheat() {
		std::vector<Injector*> injectors = {};

		//禁用女仆： 汇编修改时钟增加判定（游戏不暂停才增加）【勺12138提供】
		Injector* disable_maidCheat2 = new Injector(0x4526E6, { 0xE8, 0x48, 0x00, 0x00, 0x00, 0x90 });
		injectors.push_back(disable_maidCheat2);
		Injector* disable_maidCheat1 = new Injector(0x452733, { 0x80, 0xBE, 0x64, 0x01, 0x00, 0x00, 0x00, 0x75, 0x06, 0x01, 0xAF, 0x38, 0x08, 0x00, 0x00, 0x90, 0xC3 });
		injectors.push_back(disable_maidCheat1);

		for (size_t i = 0; i < injectors.size(); i++)
		{
			if (injectors[injectors.size() - i - 1] != nullptr)
			{
				injectors[injectors.size() - i - 1]->effect();
				Sleep(1);
			}
			else
			{
				// 处理空指针的情况，例如记录日志或抛出异常
				//std::cerr << "Error: Null pointer in injectors vector at index " << injectors.size() - i - 1 << std::endl;
			}
		}
	}

	// 使用游戏内的植物刷新函数
	void setInjectors() {
		std::vector<Injector*> injectors = {};

		// 禁用女仆：六届布阵器逻辑： 会导致过率降低
		//Injector* disable_maidCheat = new Injector(0x4526E6, { 0x80, 0xBF, 0x64, 0x01, 0x00, 0x00, 0x00, 0x75, 0x06, 0x01, 0xAF, 0x38, 0x08, 0x00, 0x00, 0x85, 0xF6, 0x85, 0xF6, 0x74, 0x05, 0xE8, 0xDB, 0x93, 0xFC, 0xFF, 0x8B, 0xCF, 0xE8, 0xA4, 0x32, 0x18, 0x00, 0x8B, 0x87, 0x3C, 0x08, 0x00, 0x00, 0xE8, 0x69, 0x8F, 0x00, 0x00, 0x80, 0xBF, 0xFA, 0x04, 0x00, 0x00, 0x00, 0x74, 0x10, 0x8B, 0x87, 0x20, 0x08, 0x00, 0x00, 0x85, 0xC0, 0x74, 0x06, 0x50, 0xE8, 0x60, 0x2F, 0xFF, 0xFF, 0x8B, 0xC7, 0xE8, 0xC9, 0xFD, 0xFF, 0xFF, 0x2B, 0xDD, 0x75, 0xB5, 0x5F, 0x5E, 0x5D, 0x5B, 0x8B, 0xE5, 0x5D, 0xC3 });		injectors.push_back(disable_maidCheat);

		// 禁掉种植音效
		Injector* disablePlantingEffect = new Injector{ 0x40ce60 };
		disablePlantingEffect->ret(0x0004);
		injectors.push_back(disablePlantingEffect);

		// 停掉游戏自带的种植物
		Injector* lStopGamePlanting = new Injector(0x42A6C0, { 0xc2, 0x0c, 0x00 }); // ret 000c
		injectors.push_back(lStopGamePlanting);

		for (size_t i = 0; i < injectors.size(); i++)
		{
			if (injectors[injectors.size() - i - 1] != nullptr)
			{
				injectors[injectors.size() - i - 1]->effect();
				Sleep(1);
			}
			else
			{
				// 处理空指针的情况，例如记录日志或抛出异常
				//std::cerr << "Error: Null pointer in injectors vector at index " << injectors.size() - i - 1 << std::endl;
			}
		}
	}

	// 检测当前阵型是否合规，合规则转为布阵码
	bool export_layout_string(std::string& result) {
		if (is_in_ize()) {
			std::array<int, 25> positions = {};               // 对应位置

			// 1.判断当前主题
			std::unordered_map<PlantType::PlantType, int> plantCount;
			for (auto plant : board->GetAllPlants()) {
				if (plant->NotExist) continue;
				plantCount[plant->Type]++;
			}

			int theme_index = 0;
			// 1.1 获取主题
			{
				if (
					plantCount[PlantType::SnowPea] == 9 && plantCount[PlantType::Peashooter] == 4 && plantCount[PlantType::SplitPea] == 4
					&& (plantCount[PlantType::Sunflower] + plantCount[PlantType::Puffshroom] == 8)
					&& plantCount[PlantType::Sunflower] >= 1 && plantCount[PlantType::Puffshroom] >= 1
					) theme_index = 4;
				else if (
					plantCount[PlantType::Starfruit] == 8 && plantCount[PlantType::Spickweed] == 9
					&& (plantCount[PlantType::Sunflower] + plantCount[PlantType::Puffshroom] == 8)
					&& plantCount[PlantType::Sunflower] >= 1 && plantCount[PlantType::Puffshroom] >= 1
					) theme_index = 5;
				else if (
					plantCount[PlantType::Chomper] == 8 && plantCount[PlantType::PotatoMine] == 9
					&& (plantCount[PlantType::Sunflower] + plantCount[PlantType::Puffshroom] == 8)
					&& plantCount[PlantType::Sunflower] >= 1 && plantCount[PlantType::Puffshroom] >= 1
					) theme_index = 6;
				else if (
					plantCount[PlantType::Fumeshroom] == 9 && plantCount[PlantType::Magnetshroom] == 8
					&& (plantCount[PlantType::Sunflower] + plantCount[PlantType::Puffshroom] == 8)
					&& plantCount[PlantType::Sunflower] >= 1 && plantCount[PlantType::Puffshroom] >= 1
					) theme_index = 7;
				else if (
					plantCount[PlantType::Scaredyshroom] == 12
					&& (plantCount[PlantType::Sunflower] + plantCount[PlantType::Puffshroom] == 13)
					&& plantCount[PlantType::Sunflower] >= 1 + 5 && plantCount[PlantType::Puffshroom] >= 1
					) theme_index = 8;

				// 判断A : 虽然下面的条件效率不高，但是简单啊（
				if (
					(plantCount[PlantType::Sunflower] + plantCount[PlantType::Puffshroom] == 8)
					&& plantCount[PlantType::Sunflower] >= 1 && plantCount[PlantType::Puffshroom] >= 0
					&& plantCount[PlantType::Wallnut] == 1
					&& plantCount[PlantType::Torchwood] == 1
					&& plantCount[PlantType::PotatoMine] == 1
					&& plantCount[PlantType::Chomper] == 2
					&& plantCount[PlantType::Peashooter] == 1
					&& plantCount[PlantType::SplitPea] == 1
					&& plantCount[PlantType::Kernelpult] == 1
					&& plantCount[PlantType::Threepeater] == 1
					&& plantCount[PlantType::SnowPea] == 1
					&& plantCount[PlantType::Squash] == 1
					&& plantCount[PlantType::Fumeshroom] == 1
					&& plantCount[PlantType::UmbrellaLeaf] == 1
					&& plantCount[PlantType::Starfruit] == 1
					&& plantCount[PlantType::Magnetshroom] == 1
					&& plantCount[PlantType::Spickweed] == 2
					) theme_index = 1;
				else if (
					(plantCount[PlantType::Sunflower] + plantCount[PlantType::Puffshroom] == 8)
					&& plantCount[PlantType::Sunflower] >= 1 && plantCount[PlantType::Puffshroom] >= 0
					&& plantCount[PlantType::Torchwood] == 1
					&& plantCount[PlantType::SplitPea] == 3
					&& plantCount[PlantType::Repeater] == 1
					&& plantCount[PlantType::Kernelpult] == 3
					&& plantCount[PlantType::Threepeater] == 1
					&& plantCount[PlantType::SnowPea] == 3
					&& plantCount[PlantType::UmbrellaLeaf] == 1
					&& plantCount[PlantType::Magnetshroom] == 1
					&& plantCount[PlantType::Spickweed] == 3
					) theme_index = 2;
				else if (
					(plantCount[PlantType::Sunflower] + plantCount[PlantType::Puffshroom] == 8)
					&& plantCount[PlantType::Sunflower] >= 1 && plantCount[PlantType::Puffshroom] >= 0
					&& plantCount[PlantType::PotatoMine] == 4
					&& plantCount[PlantType::Chomper] == 3
					&& plantCount[PlantType::Squash] == 3
					&& plantCount[PlantType::Fumeshroom] == 4
					&& plantCount[PlantType::Spickweed] == 3
					) theme_index = 3;
			}

			// 1.2 检测主题是否合法
			if (theme_index > 9 || theme_index < 1) {
				std::cout << "主题不合规, 请检查!" << std::endl;
				return false; // 返回失败
			}


			// 胆小特殊处理
			int flower_num = theme_index == 8 ? plantCount[PlantType::Sunflower] - 5 : plantCount[PlantType::Sunflower]; 
			
			// 2. 依次获取植物顺序
			std::array<PlantType::PlantType, 25> plant_types =
				GenerateLayoutCode::get_theme_plants(flower_num, static_cast<Theme>(theme_index));

			// 使用vector存储植物类型及其位置
			std::vector<std::pair<PlantType::PlantType, std::vector<int>>> plants_position;

			// 收集植物位置
			for (auto plant : board->GetAllPlants()) {
				if (plant->NotExist) continue;
				bool found = false;
				for (auto& plant_pair : plants_position) {
					if (plant_pair.first == plant->Type) {
						if ((plant->Type == PlantType::Wallnut || plant->Type == PlantType::Torchwood) && plant->Column < 2) {
							std::cout << "坚果火炬不合规!" << plant->Column + 1 << std::endl;
							return false; // 返回失败
						}

						plant_pair.second.push_back(plant->Row * 5 + plant->Column);
						found = true;
						break;
					}
				}
				if (!found) {
					plants_position.push_back({ plant->Type, {plant->Row * 5 + plant->Column} });
				}
			}

			// 3. 随机打乱每个类型的位置
			std::random_device rd;
			std::mt19937 rng(rd());
			for (auto& pair : plants_position) {
				std::shuffle(pair.second.begin(), pair.second.end(), rng);
			}
			// 如果是胆小，则需要从plant_position[0].second中取出5个，并且删除原有的，最后塞到positions后面5个
			 // 3. 胆小模式特殊处理：从plant_position[0].second中取出5个植物并将其删除，最后添加到positions后面5个
			if (theme_index == 8) {
				std::vector<int> shy_plants = {};

				// 获取前5个植物的位置
				shy_plants = { plants_position[0].second.begin(), plants_position[0].second.begin() + 5 };
				// 删除这5个植物的位置
				plants_position[0].second.erase(plants_position[0].second.begin(), plants_position[0].second.begin() + 5);
				
				// 将这5个植物的位置添加到positions的后5个位置
				for (int i = 0; i < 5; ++i) {
					positions[20 + i] = shy_plants[i];
				}
			}

			// 4. 按主题顺序填充位置
			int order = 0;

			for (int i = 0; i < 25; ++i) {
				PlantType::PlantType required_type = plant_types[i];
				bool found = false;

				// 在plants_position中查找该类型
				for (auto& pair : plants_position) {
					if (pair.first == required_type && !pair.second.empty()) {
						// 取出最后一个位置（已随机打乱）
						positions[order++] = pair.second.back();
						pair.second.pop_back();
						found = true;
						break;
					}
				}

				// 处理未找到的情况
				if (!found && theme_index !=8 ) {
					std::cerr << "错误：类型 " << PlantType::ToString(required_type)
						<< " 在位置 " << i << " 没有可用植物" << std::endl;
					positions[order++] = -1; // 用-1标记错误
				}
			}

			// 5. 返回布阵码
			result = std::to_string(theme_index) 
				+ std::to_string(flower_num)
				+ "00"
				+ GenerateLayoutCode::encrypt_to_base25(positions);

			return true; // 返回成功
		}

		return false; // 如果不在ize模式下
	}
};


// 布阵器控制
class ConsoleControler {
private:
	std::atomic<bool> stop_flag;       // 全局停止标志
	std::thread worker_thread;         // 线程对象
	std::mutex mtx;                    // 互斥锁
	std::condition_variable cv;        // 条件变量
	std::queue<int> result_queue; // 结果队列

	// 启用冲关快捷键：加速 自收 强制退出 跳关 切换女仆
	void register_RushMode_hotkey() {
		RegisterHotKey(NULL, 1, MOD_SHIFT, 'D'); // 切换加速
		RegisterHotKey(NULL, 2, MOD_SHIFT, 'A'); // 切换自动收集
		RegisterHotKey(NULL, 3, MOD_SHIFT, 'Q'); // 强制退出
		RegisterHotKey(NULL, 4, MOD_SHIFT, 'J'); // 跳关
		RegisterHotKey(NULL, 5, MOD_SHIFT, 'M'); // 切换女仆
		RegisterHotKey(NULL, 6, MOD_SHIFT, 's'); // 切换女仆
	}
	// 销毁冲关快捷键
	void unregister_RushMode_hotkey() {
		UnregisterHotKey(NULL, 1);
		UnregisterHotKey(NULL, 2);
		UnregisterHotKey(NULL, 3);
		UnregisterHotKey(NULL, 4);
		UnregisterHotKey(NULL, 5);
		UnregisterHotKey(NULL, 6);
	}
	// 比赛模式：强制退出，重开
	void register_RaceMode_hotkey() {
		RegisterHotKey(NULL, 1, MOD_SHIFT, 'Q'); // 强制退出
		RegisterHotKey(NULL, 2, MOD_SHIFT, 'R'); // 重开
		RegisterHotKey(NULL, 3, MOD_SHIFT, 'S'); // 切换女仆

	}
	void unregister_RaceMode_hotkey() {
		UnregisterHotKey(NULL, 1);
		UnregisterHotKey(NULL, 2);
		UnregisterHotKey(NULL, 3);
	}
	// 复盘模式快捷键
	void register_reviewMode_hotkey() {
		RegisterHotKey(NULL, 1, MOD_SHIFT, 'D'); // ctrl+d打开加速
		RegisterHotKey(NULL, 2, MOD_SHIFT, 'A'); // 打开自动收集
		RegisterHotKey(NULL, 3, MOD_SHIFT, 'Q'); // 强制退出
		RegisterHotKey(NULL, 4, MOD_SHIFT, 'J'); // 跳关
		RegisterHotKey(NULL, 5, MOD_SHIFT, 'R'); // 重开本局
		RegisterHotKey(NULL, 6, MOD_SHIFT, 's'); // 重开本局

	}

	void unregister_reviewMode_hotkey() {
		UnregisterHotKey(NULL, 1);
		UnregisterHotKey(NULL, 2);
		UnregisterHotKey(NULL, 3);
		UnregisterHotKey(NULL, 4);
		UnregisterHotKey(NULL, 5);
		UnregisterHotKey(NULL, 6);

	}

	// 禁掉快速编辑模式，但是坏消息是没法复制文本内容
	void disable_quick_edit_mode() {
		HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
		DWORD mode;
		GetConsoleMode(hStdin, &mode);
		mode &= ~ENABLE_QUICK_EDIT_MODE; // 禁用快速编辑
		mode &= ~ENABLE_INSERT_MODE;     // 禁用插入模式
		SetConsoleMode(hStdin, mode);
	}

	// 启用快速编辑模式（允许鼠标选择文本）
	void enable_quick_edit_mode() {
		HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
		DWORD mode;
		GetConsoleMode(hStdin, &mode);
		mode |= ENABLE_QUICK_EDIT_MODE;  // 启用快速编辑
		mode |= ENABLE_INSERT_MODE;      // 启用插入模式
		SetConsoleMode(hStdin, mode);
	}

	// 获取控制台当前宽度
	int get_terminal_width() {
		int width = 80; // 默认宽度
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		width = csbi.srWindow.Right - csbi.srWindow.Left;

		return width;
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

	// 游戏结束统一输出日志
	void log_game_end(Logger& logger, std::vector<LevelData> save_data) {
		logger.log("所有关卡数据如下:", Logger::DEBUG);
		int count = 0;
		float kernel_count = 0; float butter_count = 0;
		for (auto& leveldata : save_data) {
			logger.log("第" + std::to_string(++count) + "关数据如下:", Logger::DEBUG);
			logger.log("    放置的僵尸数: " + std::to_string(leveldata.released_zombies_count), Logger::DEBUG);
			logger.log("    僵尸花费: " + std::to_string(leveldata.zombie_cost), Logger::DEBUG);
			logger.log("    反应时间: " + leveldata.reaction_time.enPrint(), Logger::DEBUG);
			logger.log("    玉米粒数量: " + std::to_string(leveldata.kernel_count) + ", 黄油数量: " + std::to_string(leveldata.butter_count) + " 黄油率为: " + std::to_string(leveldata.kernelpult_butter_rate), Logger::DEBUG);
			kernel_count += leveldata.kernel_count;
			butter_count += leveldata.butter_count;

			logger.log("    最后一个脑子吃的时间: " + (leveldata.last_brain_eaten_time - leveldata.setlayout_time).enPrint(), Logger::DEBUG);
			logger.log("    所有脑子吃的时间列表: ", Logger::DEBUG);

			for (auto& eat_brain_time : leveldata.brain_eaten_times) {
				logger.log((eat_brain_time - leveldata.setlayout_time).enPrint(), Logger::DEBUG);
			}
		};
		if (butter_count + kernel_count == 0) {
			logger.log("没有黄油", Logger::DEBUG);
			return;
		}
		logger.log("黄油率为: " + std::to_string(butter_count / (butter_count + kernel_count)), Logger::DEBUG);
	}

	// 锁主题锁花数
	void lock_theme_flower_num(const int theme_index, const int flower_num) {

		// 1. 找到 pvz
		DWORD pid = ProcessOpener::Open();
		if (!pid) {
			std::cout << "未找到pvz!" << std::endl;
			return; // 结束
		}
		std::cout << "已找到pvz!" << std::endl;
		EnableBackgroundRunning(true); // 启用pvz后台运行

		// 2. 实例化游戏控制类, 实例化冲关布阵码生成器
		GameControl game_controler(pid);
		GenerateLayoutCode code_generator;
		// 3. 一直检测，直到进入ize
		while (!game_controler.is_in_ize()) {
			Sleep(1);
		}
		std::cout << "已经进入ize, 现在开始布阵" << std::endl;

		// 记录存档
		std::vector<LevelData> save_data;
		std::vector<std::string> all_layout_code;

		// 记录僵尸
		std::unordered_set<int> processed_zombie_ids;

		// 标志位
		int current_flag = -1; std::string ls;
		bool is_speed_up = false; bool is_auto = false; bool has_started = false;
		auto current_adress = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768);
		std::array<int, 25> orders;
		TimeStruct start_time = TimeStruct::getNow(); LevelData leveldata;


		// 1. 删掉植物
		game_controler.update_brains();
		game_controler.clear_all_bullets();
		game_controler.clear_all_zombies();
		game_controler.clear_all_plants();
		game_controler.board->GetMiscellaneous()->Round = 0;

		// 禁植物禁音效
		game_controler.setInjectors();
		game_controler.disable_maidCheat();

		MSG msg = { 0 };
		while (true) {
			// 1. 监测快捷键并处理，全局热键消息
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_HOTKEY) {
					if (msg.wParam == 1) {  // shift+D 切换加速与原速
						is_speed_up = !is_speed_up;
						if (is_speed_up) {
							game_controler.set_speed_10x();
							std::cout << "进行了10x加速" << std::endl;

						}
						else {
							game_controler.reset_speed();
							std::cout << "关闭加速" << std::endl;

						};
					}
					else if (msg.wParam == 2) { // shift+A 切换自动收集
						is_auto = !is_auto;
						game_controler.auto_collect(is_auto);
						std::cout << std::string(is_auto ? "打开" : "关闭") + "自动收集" << std::endl;
					}
					else if (msg.wParam == 3) { // shift+q 强制结束
						game_controler.auto_collect(false);
						game_controler.reset_speed();
						game_controler.enable_maidCheat();

						std::cout << "结束复盘模式!" << std::endl;
						return;
					}
					else if (msg.wParam == 4) { // shift+j 跳关
						game_controler.board->Win();
						std::cout << "跳过第" << game_controler.board->GetMiscellaneous()->Round + 1 << "关!" << std::endl;
					}
					else if (msg.wParam == 5) { // shift+r
						game_controler.clear_not_colleted_sun();
						game_controler.update_brains();
						game_controler.clear_all_zombies();
						game_controler.clear_all_bullets();
						game_controler.clear_all_plants();
						// 恢复阳光
						game_controler.board->Sun = 2000;
						leveldata.released_zombies_count = 0;
						leveldata.zombie_cost = 0;
						has_started = false;
						leveldata.brain_eaten_times.clear(); //清空吃脑记录

						TimeStruct restart_time = TimeStruct::getNow() - start_time;
						std::string restart_str = restart_time.enPrint().append("     ").append(std::string("Restart"));
						Creator::CreateCaption(restart_str.c_str(), restart_str.size(), CaptionStyle::Lowermiddle); // 游戏白字，处于靠下居中位置

						// 重新布阵一下
						leveldata.setlayout_time = TimeStruct::getNow();
						game_controler.set_layout_test(ls, theme_index, flower_num, 0, orders);

					}
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			// 检测重开
			do {
				if (PVZ::Memory::ReadPointer(0x6a9ec0, 0x768) == 0
					|| current_adress == PVZ::Memory::ReadPointer(0x6a9ec0, 0x768)
					|| game_controler.pvz->GameState != PVZGameState::Playing) continue;

				else { // 重开了
					game_controler.board = PVZ::GetBoard(); // 重新拿一下board 
					Sleep(5);

					current_adress = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768);
					current_flag = PVZ::GetBoard()->GetMiscellaneous()->Round;

					game_controler.clear_not_colleted_sun();
					game_controler.update_brains();
					game_controler.clear_all_zombies();
					game_controler.clear_all_bullets();
					game_controler.clear_all_plants();
					//game_controler.remove_cutscene_zombie();
					// 恢复阳光
					game_controler.board->Sun = 2000;
					leveldata.released_zombies_count = 0;
					leveldata.zombie_cost = 0;
					has_started = false;
					leveldata.brain_eaten_times.clear(); //清空吃脑记录

					auto result = code_generator.generate_arr_seed(static_cast<Theme>(theme_index));
					auto seed = result.second;
					orders = result.first;
					ls = std::to_string(theme_index)
						+ std::to_string(flower_num)
						+ "00"
						+ code_generator.encode_seed(result.second);
					all_layout_code.clear(); all_layout_code.push_back(ls);
					game_controler.set_layout_test(ls, theme_index, flower_num, 0, orders);
				}
			} while (0);



			// 2. 跨关
			if (game_controler.board->GetBaseAddress() &&
				game_controler.board->GetMiscellaneous()->Round != current_flag &&
				game_controler.pvz->GameState == PVZGameState::Playing) {

				if (has_started) // 如果已经开始游戏了，说明是正常跨关
				{
					// 0. 先打印上一关数据
					std::cout << (TimeStruct::getNow() - start_time).enPrint()
						<< " 已经通过" << std::to_string(game_controler.board->GetMiscellaneous()->Round)
						<< "关, 花费" << std::to_string(leveldata.zombie_cost)
						<< std::endl;

					std::cout << "反应时间:" << (leveldata.first_zombie_release_time - leveldata.setlayout_time).enPrint()
						<< " 过关耗时: " << (leveldata.last_brain_eaten_time - start_time).enPrint()
						<< std::endl;

					std::cout << "吃脑时间依次为: ";
					for (auto eaten_brain_time : leveldata.brain_eaten_times) {
						std::cout << (eaten_brain_time - start_time).enPrint() << " ";
					}
					std::cout << std::endl;

					// 1. 更新数据，初始化下一关要记录的数据
					leveldata.zombie_cost = 0;
					leveldata.score = current_flag;
					leveldata.setlayout_time = TimeStruct::getNow();
					leveldata.brain_eaten_times = {};
					leveldata.eaten_brain_count = 0;

					// 2.5 关闭加速
					game_controler.reset_speed();

					has_started = false;
				}

				// 2.1 先布阵
				game_controler.board->Sun = 2000;
				current_flag = game_controler.board->GetMiscellaneous()->Round; //更新关数并且进行布阵

				auto result = code_generator.generate_arr_seed(static_cast<Theme>(theme_index));
				auto seed = result.second;
				orders = result.first;

				ls = std::to_string(theme_index)
					+ std::to_string(flower_num)
					+ "00"
					+ code_generator.encode_seed(result.second);
				all_layout_code.push_back(ls);
				game_controler.set_layout_test(ls, theme_index, flower_num, 0, orders);


				leveldata.setlayout_time = TimeStruct::getNow();
				std::cout << std::string(get_terminal_width(), '-') << std::endl;
				std::cout << "第" << current_flag + 1 << "关布阵码为: " << ls << std::endl;

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
				leveldata.last_brain_eaten_time = start_time;
			}

			// 4. 记录僵尸花费
			std::vector<SPT<PVZ::Zombie>> zombies = game_controler.board->GetAllZombies();
			for (auto& zombie : zombies) {
				if (game_controler.board->GetMiscellaneous()->Round == 0)  Sleep(50);
				// 如果僵尸死了，跳过
				if (zombie->NotExist) {
					processed_zombie_ids.erase(zombie->Id);
					continue;
				}
				// 如果不是刚放置的僵尸, 跳过
				if (zombie->ExistedTime < 0 || zombie->ExistedTime > 2000) continue; // 只检查新生成的【冲关的话得放宽】
				// 如果这个僵尸已经处理过了, 跳过
				if (processed_zombie_ids.count(zombie->Id)) continue; // 已处理则跳过

				processed_zombie_ids.insert(zombie->Id); // 记录已处理

				// 如果不是ize中的僵尸，而且现在又不是开局的话, 记录下来
				if (!game_controler.ZombieSunCost.count(zombie->Type)) {
					// 重开的话需要先清除选卡界面的僵尸
					//std::cout << "检测到在第" + std::to_string(zombie->Row + 1) + "行放置了非ize关卡的僵尸,僵尸类型为" + ZombieType::ToString(zombie->Type) << std::endl;
					continue;
				}
				// 如果是ize中的僵尸，但又不是伴舞僵尸，日志记录并且计算花费【每一关结束赋值】
				if (zombie->Type == ZombieType::BackupDancer) continue;

				auto zombie_info = game_controler.ZombieSunCost[zombie->Type];
				leveldata.zombie_cost += zombie_info.first;

				// 记录反应时间
				if (leveldata.released_zombies_count == 1) {
					leveldata.first_zombie_release_time = TimeStruct::getNow();
				}
			}


			// 5. 监测脑子变化
			if (leveldata.eaten_brain_count != game_controler.countEatenBrain() && leveldata.eaten_brain_count != 5) {
				leveldata.last_brain_eaten_time = TimeStruct::getNow();
				leveldata.brain_eaten_times.push_back(leveldata.last_brain_eaten_time);
				leveldata.eaten_brain_count = game_controler.countEatenBrain();
			}
		}


	}

	// 冲关循环: 布阵器输入了4
	void LevelRush(bool is_ban_maidCheat) {

		// 1. 找到 pvz
		DWORD pid = ProcessOpener::Open();
		if (!pid) {
			std::cout << "未找到pvz!" << std::endl;
			return; // 结束
		}
		std::cout << "已找到pvz!" << std::endl;
		EnableBackgroundRunning(true); // 启用pvz后台运行

		// 2. 实例化游戏控制类, 实例化冲关布阵码生成器
		GameControl game_controler(pid);
		GenerateLayoutCode code_generator;

		// 3. 一直检测，直到进入ize
		while (!game_controler.is_in_ize()) {
			Sleep(1);
		}
		std::cout << "已经进入ize, 现在开始布阵" << std::endl;

		// 4. 准备开始日志记录
		std::string current_time = TimeStruct::getCurrentDateTime();
		Logger logger_data(current_time + ".log", Logger::DEBUG); // 默认打印INFO, 但是记录的话全部记录

		// 5. 按照输入的指令选择是否禁用女仆
		if (is_ban_maidCheat) {
			game_controler.disable_maidCheat();
			logger_data.log("当前女仆状态: 禁用!", Logger::INFO);
		}
		else {
			game_controler.enable_maidCheat();
			logger_data.log("当前女仆状态: 不禁用!", Logger::INFO);
		}

		// 6. 初始化bool值
		bool is_speed_up = false; // 是否开启了加速
		bool is_auto = false; // 是否开启了自动收集
		game_controler.auto_collect(false); // 关掉自动收集
		game_controler.reset_speed(); // 恢复原速

		// 记录关卡和游戏开始时间
		int current_flag = -1;
		bool has_started = false;
		TimeStruct start_time = TimeStruct::getNow();
		auto current_adress = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768);
		// 记录胆小数量
		int scardy_theme_count = 0;


		// 记录已经监测并处理的僵尸，子弹
		std::unordered_set<int> processed_zombie_ids;

		// 结束条件： 阳光用完
		int lowestSun = 50;


		// 记录存档数据与布阵码数据
		std::vector<LevelData> save_data;
		std::vector<std::string> all_layout_code;

		// 7. 开始游戏，保存第一关数据，并对第一关进行布阵
		LevelData leveldata;

		std::string ls;
		int flower_num;

		// 删掉第一关的植物
		game_controler.clear_all_plants();
		// 禁掉游戏种植物和种植音效
		game_controler.setInjectors();

		// 7.初始化第一关数据并布阵
		game_controler.board->GetMiscellaneous()->Round = 0; // 从第一关开始
		game_controler.board->Sun = 150; // 设置初始阳光
		leveldata.initial_sun = 150;
		game_controler.update_brains(); // 脑子初始化为0
		game_controler.clear_all_zombies();
		game_controler.clear_all_bullets();
		game_controler.clear_not_colleted_sun();

		// 布阵器主循环
		MSG msg = { 0 };
		while (true) {
			// 添加快捷键并处理，全局热键消息
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_HOTKEY) {
					if (msg.wParam == 1) {  // shift+D 切换加速与原速
						is_speed_up = !is_speed_up;
						if (is_speed_up) {
							game_controler.set_speed_10x();
							logger_data.log("进行了10x加速", Logger::DEBUG);
						}
						else {
							game_controler.reset_speed();
							logger_data.log("关闭加速", Logger::DEBUG);
						};
					}
					else if (msg.wParam == 2) { // shift+A 切换自动收集
						is_auto = !is_auto;
						game_controler.auto_collect(is_auto);
						logger_data.log(std::string(is_auto ? "打开" : "关闭") + "自动收集", Logger::DEBUG);
					}
					else if (msg.wParam == 3) { // shift+q 强制结束
						game_controler.auto_collect(false);
						game_controler.reset_speed();

						auto over_time = TimeStruct::getNow() - start_time;
						logger_data.log(over_time.enPrint().append(" 提前结束游戏!"), Logger::INFO);
						logger_data.log(std::string(get_terminal_width(), '-'), Logger::INFO);
						//得分取一位小数
						std::ostringstream oss;
						oss << std::fixed << std::setprecision(1) << (std::round(leveldata.score * 10) / 10.0);
						std::string score_str = oss.str();

						logger_data.log("游戏结束! 最终得分为——  " + score_str, Logger::INFO);
						logger_data.log("最后吃脑时间为: " + (leveldata.last_brain_eaten_time - start_time).cnPrint(), Logger::INFO);

						auto timeStr = (leveldata.last_brain_eaten_time - start_time).enPrint().append("     ") + score_str;
						Creator::CreateCaption(timeStr.c_str(), timeStr.size(), CaptionStyle::Lowermiddle);

						log_game_end(logger_data, save_data);

						if (is_ban_maidCheat) { // 切换回不禁女仆
							game_controler.enable_maidCheat();
						}
						return;
					}
					else if (msg.wParam == 4) { // shift+j 跳关
						game_controler.board->Win();
						logger_data.log("跳过第"+std::to_string(game_controler.board->GetMiscellaneous()->Round) +"关!", Logger::DEBUG);
					}
					else if (msg.wParam == 5) {
						is_ban_maidCheat = !is_ban_maidCheat;
						if (is_ban_maidCheat) {
							game_controler.disable_maidCheat();
							logger_data.log("切换女仆: 禁用!", Logger::INFO);
						}
						else
						{
							game_controler.enable_maidCheat();
							logger_data.log("切换女仆: 不禁用!", Logger::INFO);
						}

					}
					
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			// 检测重开
			do {
				if (PVZ::Memory::ReadPointer(0x6a9ec0, 0x768) == 0
					|| current_adress == PVZ::Memory::ReadPointer(0x6a9ec0, 0x768)
					|| game_controler.pvz->GameState != PVZGameState::Playing) continue;

				else { // 重开了
					game_controler.board = PVZ::GetBoard(); // 重新拿一下board
					game_controler.pvz = game_controler.board->GetPVZApp();

					current_adress = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768);
					Sleep(5);
					// 初始化数据
					current_flag = game_controler.board->GetMiscellaneous()->Round;
					game_controler.clear_not_colleted_sun();
					game_controler.update_brains();
					game_controler.clear_all_zombies();
					game_controler.clear_all_bullets();
					game_controler.clear_all_plants();
					processed_zombie_ids.clear();
					
					// 恢复阳光
					game_controler.board->Sun = 150;
					leveldata.released_zombies_count = 0;
					leveldata.zombie_cost = 0;
					has_started = false;
					leveldata.brain_eaten_times.clear(); //清空吃脑记录
					leveldata.score = 0.00;
					leveldata.butter_count = 0;
					leveldata.kernel_count = 0;
					leveldata.collected_sun = 0;

					// 重新刷个码然后布阵
					ls = code_generator.generate_LevelRush_code(current_flag); 
					all_layout_code.clear(); all_layout_code.push_back(ls);
					int theme_index = 0; int flower_num = 0; int sun = 0;
					std::array<int, 25> orders = {};
					// 检测布阵码是否合法
					if (!GenerateLayoutCode::decode_layout_string(ls, theme_index, flower_num, sun, orders)) {
						std::cout << "布阵码不合法" << std::endl;
						return;
					}
					game_controler.set_layout_test(ls, theme_index, flower_num, 0, orders);
				}
			} while (0);

			// 2. 跨关更新
			if (game_controler.board->GetBaseAddress() &&
				game_controler.board->GetMiscellaneous()->Round != current_flag &&
				game_controler.pvz->GameState == PVZGameState::Playing) {

				// 2.0 先布阵【包括第一关的布阵】
				// 1. 确保没植物: 有一个植物就删一次然后退出去
				for (auto plant: game_controler.board->GetAllPlants()) {
					if (!plant->NotExist) {
						game_controler.clear_all_plants();
						break;
					}
				}
				// 准备布阵
				current_flag = game_controler.board->GetMiscellaneous()->Round; //更新关数并且进行布阵
				ls = code_generator.generate_LevelRush_code(current_flag); all_layout_code.push_back(ls);
				int theme_index = 0; int flower_num = 0; int sun = 0;
				std::array<int, 25> orders = {};
				// 检测布阵码是否合法
				if (!GenerateLayoutCode::decode_layout_string(ls, theme_index, flower_num, sun, orders)) {
					std::cout << "布阵码不合法" << std::endl;
					return;
				}
				game_controler.set_layout_test(ls, theme_index, flower_num, 0, orders);

				// 游戏还没开始就不存档了
				if (!has_started) continue;
				// 2.1 存档
				if (leveldata.kernel_count == 0) {
					leveldata.kernelpult_butter_rate = 0.00;
				}
				else {
					leveldata.kernelpult_butter_rate = leveldata.butter_count / (leveldata.butter_count + leveldata.kernel_count);
				}
				save_data.push_back(leveldata);

				// 2.2 记录跨关数据
				logger_data.log((TimeStruct::getNow() - start_time).enPrint()
					+ " 已经通过" + std::to_string(game_controler.board->GetMiscellaneous()->Round)
					+ "关, 阳光" + std::to_string(game_controler.board->Sun)
					+ "，花费" + std::to_string(leveldata.zombie_cost),
					Logger::INFO);
				// 2.3 记录胆小
				if (ls[0] == '8')
				{
					logger_data.log("遇到了一次胆小, 目前胆小次数为: " + std::to_string(scardy_theme_count), Logger::DEBUG);
					scardy_theme_count += 1;
				}

				// 2.3 更新关数，初始化下一关需要记录的数据
				leveldata.initial_sun = game_controler.board->Sun;
				leveldata.released_zombies_count = 0;
				leveldata.zombie_cost = 0;
				leveldata.collected_sun = 0;
				leveldata.kernel_count = 0;
				leveldata.butter_count = 0;
				leveldata.kernelpult_butter_rate = 0.00;
				leveldata.score = current_flag;
				leveldata.setlayout_time = TimeStruct::getNow();

				leveldata.brain_eaten_times = {};
				leveldata.eaten_brain_count = 0;

				// 清掉上一关
				processed_zombie_ids.clear();

				// 2.5 每关开始，关闭加速
				game_controler.reset_speed();
				is_speed_up = false;
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

				logger_data.log("开始游戏!  现实时间为: " + start_time.getCurrentTime(), Logger::INFO);
				logger_data.log(std::string(get_terminal_width(), '-'), Logger::INFO);
			}

			// 5. 计算每关花费
			std::vector<SPT<PVZ::Zombie>> zombies = game_controler.board->GetAllZombies();
			for (auto& zombie : zombies) {
				// 如果是第一关的话，晚点计算，等僵尸清干净
				if (game_controler.board->GetMiscellaneous()->Round == 0)  Sleep(100); // 第一关的话就先等游戏自己先把僵尸清除
				// 如果僵尸死了，跳过
				if (zombie->NotExist) {
					processed_zombie_ids.erase(zombie->Id);
					continue;
				}
				// 如果不是刚放置的僵尸, 跳过
				if (zombie->ExistedTime < 0 || zombie->ExistedTime > 2000) continue; // 只检查新生成的【冲关的话得放宽】
				// 如果这个僵尸已经处理过了, 跳过
				if (processed_zombie_ids.count(zombie->Id)) continue; // 已处理则跳过

				processed_zombie_ids.insert(zombie->Id); // 记录已处理

				// 如果不是ize中的僵尸，跳过
				if (!game_controler.ZombieSunCost.count(zombie->Type)) {
					// 重开的话需要先清除选卡界面的僵尸
					continue;
				}
				// 如果是ize中的僵尸，但又不是伴舞僵尸，日志记录并且计算花费【每一关结束赋值】
				if (zombie->Type == ZombieType::BackupDancer) continue;

				auto zombie_info = game_controler.ZombieSunCost[zombie->Type];
				leveldata.zombie_cost += zombie_info.first;
				leveldata.released_zombies_count += 1;

				// 记录反应时间
				if (leveldata.released_zombies_count == 1) {
					leveldata.first_zombie_release_time = TimeStruct::getNow();
					leveldata.reaction_time = leveldata.first_zombie_release_time - leveldata.setlayout_time;
				}
			}


			// 5. 监测脑子变化：win过程中会有一个误判，特判一下
			if (leveldata.eaten_brain_count != game_controler.countEatenBrain() 
				&& leveldata.eaten_brain_count != 5) {
				// 计算得分
				leveldata.score = game_controler.board->GetMiscellaneous()->Round + game_controler.countEatenBrain() * 0.2;
				leveldata.last_brain_eaten_time = TimeStruct::getNow();
				leveldata.brain_eaten_times.push_back(leveldata.last_brain_eaten_time);
				leveldata.eaten_brain_count = game_controler.countEatenBrain();

				logger_data.log((leveldata.last_brain_eaten_time - start_time).enPrint() + "吃了第" + std::to_string(game_controler.countEatenBrain()) + "个脑子", Logger::DEBUG);
			}


			// 6. 还有时间，阳光用完了
			if (game_controler.board->ZombiesCount == 0 && game_controler.board->Sun < lowestSun) {
				bool is_dead = true;

				for (auto& coin : game_controler.board->GetAllCoins()) {
					if (coin->Type == CoinType::NormalSun) is_dead = false;
				}
				if (is_dead) {

					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << (std::round(leveldata.score * 10) / 10.0);
					std::string score_str = oss.str();
					auto game_over_str = std::string("GameOver..").append("     ").append(std::to_string(scardy_theme_count) + "-") + score_str;
					logger_data.log(std::string(get_terminal_width(), '-'), Logger::INFO);
					logger_data.log((leveldata.last_brain_eaten_time - start_time).enPrint() + " 游戏结束--  得分: " + score_str, Logger::INFO);
					Creator::CreateCaption(game_over_str.c_str(), game_over_str.size(), CaptionStyle::Lowermiddle); // 去掉浮点数的两位小数


					// 关卡结束后打印本次冲关数据
					std::string layout_codes;
					for (const auto& ls : all_layout_code) {
						layout_codes += ls + ".";
					}
					layout_codes = layout_codes.substr(0, layout_codes.size() - 1); // 去掉最后的'.'
					logger_data.log("本次冲关关卡布阵码为(已复制到剪切板): \n" + layout_codes, Logger::INFO);
					copyToClipBoard(layout_codes);


					log_game_end(logger_data, save_data);


					// 把加速和自动收集关了
					game_controler.auto_collect(false);
					game_controler.reset_speed();
					if (is_ban_maidCheat) {
						game_controler.enable_maidCheat();
					}

					return;
				}
			}
		}

	}

	// 30min限时循环
	void SpeedRun30min(std::string ls, const bool is_cheat_check) {
		// 0. 拿到所有关卡的布阵代码, 检测长度是否合法，单关布阵码是否合法
		auto ss = std::stringstream(ls);
		auto str = std::string();
		auto vec = std::vector<std::string>();
		while (getline(ss, str, '.')) vec.push_back(str);
		if (vec.size() != 25) {
			std::cout << "输入不合法" << std::endl;
			return;
		};

		for (auto it : vec) {
			int theme_index = static_cast<int>(it[0] - '0');
			int flower_num = static_cast<int>(it[1] - '0');
			int sun = std::stoi(it.substr(2, 2)) * 25;
			std::string order_str = it.substr(4);
			if (theme_index > 8 || theme_index < 1) {
				std::cout << "输入不合法" << std::endl;
			}
			if (flower_num < 1 || flower_num > 8) {
				std::cout << "输入不合法" << std::endl;
			}
			if (sun != 0) { // 常规模式不可设置阳光
				std::cout << "输入不合法" << std::endl;
			}
		};

		// 1. 找到 pvz
		DWORD pid = ProcessOpener::Open();
		if (!pid) {
			std::cout << "未找到pvz!" << std::endl;
			return; // 结束
		}
		std::cout << "已找到pvz!" << std::endl;
		EnableBackgroundRunning(true); // 启用pvz后台运行
		
		// 2. 实例化游戏控制器,实例化布阵码生成器【为了拿花数】
		GameControl game_controler(pid);
		GenerateLayoutCode code_generator;

		// 3. 一直检测，直到进入ize
		while (!game_controler.is_in_ize()) {
			Sleep(1);
		}

		// 4. 准备开始开始记录日志
		std::string current_time = TimeStruct::getCurrentDateTime();
		Logger logger_data(current_time + ".log", Logger::DEBUG); // 默认打印INFO, 但是记录的话全部记录


		logger_data.log(std::string(get_terminal_width(), '-'), Logger::DEBUG);
		logger_data.log("已经进入ize, 现在开始布阵", Logger::INFO);


		// 反作弊日志：在外部作用域声明指针和引用别名
		Logger* logger_cheat = nullptr;  // 原始指针
		// 先检测环境是否异常
		GameCheatCheck game_cheat_checker(2, logger_cheat); // 每2s检测一次
		TimeStruct check_time = TimeStruct::getNow();
		if (is_cheat_check) {
			// 动态创建对象，生命周期由手动管理（或智能指针）
			logger_cheat = new Logger(
				current_time + "_cheatCheck.log",
				Logger::DEBUG,
				true
			);
			game_cheat_checker.logger = logger_cheat;
			SetWindowTextA(PVZ::Memory::mainwindowhandle, "Plants vs. Zombies(cheatCheck: open, maidCheat: disable)"); // 禁掉一些寻找游戏是通过窗口名的：如算血器
			game_cheat_checker.check_envirnoment();
		}
		else { // 还原回标题
			SetWindowTextA(PVZ::Memory::mainwindowhandle, "Plants vs. Zombies"); // 禁掉一些寻找游戏是通过窗口名的：如算血器
		}

		if (is_cheat_check) logger_cheat->log(
			std::string("玩家已经进入ize, ")
			+ "当前日期与时间为: " + TimeStruct::getCurrentDateTime() + "\n"
			+ "本次布阵码为: " + ls + "\n"
			+ "执行本次计时的线程号: " + std::to_string(GetCurrentThreadId()) + " 游戏进程号: " + std::to_string(pid) + "\n"
			+ "游戏宽高: " + std::to_string(PVZ::Memory::ReadPointer(0x6a9ec0, 0xc0)) + "-" + std::to_string(PVZ::Memory::ReadPointer(0x6a9ec0, 0xc4)) + "\n"
			+ "游戏窗口坐标: " + std::to_string(PVZ::Memory::ReadPointer(0x6a9ec0, 0x320, 0x94, 0x30)) + "-" + std::to_string(PVZ::Memory::ReadPointer(0x6a9ec0, 0x320, 0x94, 0x34)) + "\n"
			+ "游戏内玩家名字为: " + game_controler.get_player_name()
			, Logger::DEBUG
		);


		// 5. 初始化游戏信息
		int current_flag = -1;
		bool has_started = false; 
		bool is_crashed = false;
		TimeStruct start_time = TimeStruct::getNow();
		auto current_adress = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768);
		// 记录已经监测并处理的僵尸
		std::unordered_set<int> processed_zombie_ids;
		std::unordered_set<int> processed_projectile_ids;
		std::unordered_set<int> processed_coin_ids;

		// 结束条件
		int lowestSun = 50;

		// 6. 记录存档
		std::vector<LevelData> save_data;
		// 6.1. 初始化每关要记载的数据
		LevelData leveldata;


		// 提示主题
		std::vector<std::string> parts;
		std::istringstream iss(ls);
		std::string token;
		while (getline(iss, token, '.')) {
			parts.push_back(token);
		}

		logger_data.log(std::string(get_terminal_width(), '-'), Logger::INFO);
		logger_data.log("25个主题序号为：", Logger::INFO);
		int count = 0;
		std::ostringstream oss;
		for (const auto& part : parts) {
			count++;
			oss << part[0] << " ";
			if (count % 10 == 0) {
				oss << std::endl;
			}
		}
		logger_data.log(oss.str(), Logger::INFO);



		// 删掉第一关的植物
		game_controler.clear_all_plants();
		// 禁女仆, 禁掉游戏生成植物，禁掉植物种植音效
		game_controler.disable_maidCheat();
		game_controler.setInjectors();


		// 7. 初始化第一关数据
		game_controler.board->GetMiscellaneous()->Round = 0; // 从第一关开始
		game_controler.board->Sun = 150; // 设置初始阳光
		leveldata.initial_sun = 150;
		game_controler.update_brains(); // 恢复脑子
		game_controler.clear_all_zombies(); // 删僵尸
		game_controler.clear_all_bullets(); // 删子弹
		game_controler.clear_not_colleted_sun(); // 删掉没收集的阳光
		if (is_cheat_check) logger_cheat->log("初始化第一关信息，并进行第一关的布阵", Logger::DEBUG);


		// 8. 游戏主循环
		MSG msg = { 0 };
		while (true) {

			// 添加快捷键并处理，全局热键消息
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_HOTKEY) {
					if (msg.wParam == 1) { // shift+q 强制结束
						auto over_time = TimeStruct::getNow() - start_time;
						logger_data.log(over_time.enPrint().append(" 提前结束游戏!"), Logger::INFO);
						logger_data.log(std::string(get_terminal_width(), '-'), Logger::INFO);

						std::ostringstream oss;
						oss << std::fixed << std::setprecision(1) << (std::round(leveldata.score * 10) / 10.0);
						std::string score_str = oss.str();

						if (is_cheat_check) logger_cheat->log("玩家主动提前结束游戏", Logger::DEBUG);


						logger_data.log("游戏结束! 最终得分为——  " + score_str, Logger::INFO);
						logger_data.log("最后吃脑时间为: " + (leveldata.last_brain_eaten_time - start_time).cnPrint(), Logger::INFO);
						auto timeStr = (leveldata.last_brain_eaten_time - start_time).enPrint().append("     ") + score_str;
						Creator::CreateCaption(timeStr.c_str(), timeStr.size(), CaptionStyle::Lowermiddle);
						log_game_end(logger_data, save_data);

						return;
					}
					if (msg.wParam == 2) { // shift+R重开
						game_controler.clear_not_colleted_sun();
						game_controler.update_brains();
						game_controler.clear_all_zombies();
						game_controler.clear_all_bullets();
						game_controler.clear_all_plants();

						processed_zombie_ids.clear();
						processed_coin_ids.clear();
						processed_projectile_ids.clear();
						// 恢复阳光
						game_controler.board->Sun = current_flag == 0 ? 150 : leveldata.initial_sun;
						leveldata.released_zombies_count = 0;
						leveldata.zombie_cost = 0;
						leveldata.brain_eaten_times.clear(); //清空吃脑记录
						leveldata.butter_count = 0;
						leveldata.kernel_count = 0;
						leveldata.score = current_flag;

						if (current_flag == 0) has_started = false;

						TimeStruct restart_time = TimeStruct::getNow() - start_time;
						logger_data.log(restart_time.enPrint() + " 重开第" + std::to_string(current_flag + 1) + "关!", Logger::INFO);
						if (is_cheat_check) logger_cheat->log("玩家在" + restart_time.enPrint() + "时进行重开，关数为" + std::to_string(current_flag + 1), Logger::DEBUG);

						std::string restart_str = restart_time.enPrint().append("     ").append(std::string("Restart"));
						Creator::CreateCaption(restart_str.c_str(), restart_str.size(), CaptionStyle::Lowermiddle); // 游戏白字，处于靠下居中位置

						// 重新布阵一下
						current_flag = game_controler.board->GetMiscellaneous()->Round;
						std::string layout_code = vec[current_flag];
						int theme_index = 0; int flower_num = 0; int sun = 0;
						std::array<int, 25> orders = {};
						// 检测布阵码是否合法
						if (!GenerateLayoutCode::decode_layout_string(layout_code, theme_index, flower_num, sun, orders)) {
							std::cout << "布阵码不合法" << std::endl;
							return;
						}
						game_controler.set_layout_test(layout_code, theme_index, flower_num, 0, orders);
						leveldata.setlayout_time = TimeStruct::getNow();
					}
					if (msg.wParam == 3) { // shift+s崩溃
						if (!is_crashed) continue; // 没崩溃按了没反应
						// 找游戏
						DWORD pid = ProcessOpener::Open();
						if (!pid) continue;
						logger_data.log("现已恢复存档,请继续游戏...", Logger::INFO);

						// 2. 重新拿一下board
						current_adress = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768);
						GameControl game_controler2(pid);
						game_controler = game_controler2;
						if (!game_controler.is_in_ize()) continue;

						// 禁女仆, 禁掉游戏生成植物，禁掉植物种植音效
						game_controler.clear_all_plants();
						game_controler.disable_maidCheat();
						game_controler.setInjectors();

						game_controler.board->GetMiscellaneous()->Round = current_flag;
						std::string layout_code = vec[current_flag];
						int theme_index = 0; int flower_num = 0; int sun = 0;
						std::array<int, 25> orders = {};
						// 检测布阵码是否合法
						if (!GenerateLayoutCode::decode_layout_string(layout_code, theme_index, flower_num, sun, orders)) {
							std::cout << "布阵码不合法" << std::endl;
							return;
						}
						game_controler.set_layout_test(layout_code, theme_index, flower_num, 0, orders);
						if (is_cheat_check) logger_cheat->log("现在对第" + std::to_string(current_flag) + "关进行布阵,花数:" + std::to_string(flower_num) + ", 布阵码:" + layout_code, Logger::DEBUG);

						// 计算还拥有的时间: 使用的时间, 第一个僵尸释放的时间也就是start_time，上一次存档的时间
						start_time = TimeStruct::getNow() - leveldata.current_time;
						leveldata.last_brain_eaten_time = TimeStruct::getNow() - leveldata.current_time;

						if (is_cheat_check) logger_cheat->log("游戏崩溃了, 现在对第" + std::to_string(current_flag) + "关重新进行布阵,花数:" + std::to_string(flower_num) + ", 布阵码:" + ls, Logger::DEBUG);
						
						// 2. 恢复阳光，关数, 黄油数，吃脑数
						game_controler.board->Sun = leveldata.initial_sun;
						leveldata.score = game_controler.board->GetMiscellaneous()->Round;
						leveldata.butter_count = 0;
						leveldata.collected_sun = 0;
						leveldata.eaten_brain_count = 0;
						leveldata.kernel_count = 0;
						leveldata.released_zombies_count = 0;
						leveldata.zombie_cost = 0;

						processed_projectile_ids.clear();
						processed_coin_ids.clear();
						processed_zombie_ids.clear();

						// 3. 修改计时：
						//清空吃脑时间, 最后吃脑时间，布阵时间
						leveldata.brain_eaten_times.clear();
						leveldata.setlayout_time = TimeStruct::getNow();

						is_crashed = false;
					}
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		
			if (is_crashed) continue;
		
			// 监测重开
			do {
				if (PVZ::Memory::ReadPointer(0x6a9ec0, 0x768) == 0
					|| current_adress == PVZ::Memory::ReadPointer(0x6a9ec0, 0x768)
					|| game_controler.pvz->GameState != PVZGameState::Playing) continue;

				game_controler.board = PVZ::GetBoard();
				game_controler.pvz = game_controler.board->GetPVZApp();

				current_adress = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768);
				if (is_cheat_check) logger_cheat->log("使用了游戏内的restart", Logger::DEBUG);
				//game_controler.remove_cutscene_zombie();

				current_flag = game_controler.board->GetMiscellaneous()->Round;

				game_controler.clear_not_colleted_sun();
				game_controler.update_brains();
				game_controler.clear_all_zombies();
				game_controler.clear_all_bullets();
				game_controler.clear_all_plants();

				processed_zombie_ids.clear();
				processed_coin_ids.clear();
				processed_projectile_ids.clear();
				// 恢复阳光
				game_controler.board->Sun = current_flag == 0 ? 150 : leveldata.initial_sun;
				leveldata.released_zombies_count = 0;
				leveldata.zombie_cost = 0;
				if(int(leveldata.score)==0) has_started = false;
				leveldata.brain_eaten_times.clear(); //清空吃脑记录
				leveldata.score = 0.00;
				leveldata.butter_count = 0;
				leveldata.kernel_count = 0;

				current_flag = game_controler.board->GetMiscellaneous()->Round;
				std::string layout_code = vec[current_flag];
				int theme_index = 0; int flower_num = 0; int sun = 0;
				std::array<int, 25> orders = {};
				// 检测布阵码是否合法
				if (!GenerateLayoutCode::decode_layout_string(layout_code, theme_index, flower_num, sun, orders)) {
					std::cout << "布阵码不合法" << std::endl;
					return;
				}
				game_controler.set_layout_test(layout_code, theme_index, flower_num, 0, orders);
				if (is_cheat_check) logger_cheat->log("现在对第" + std::to_string(current_flag) + "关重新进行布阵,花数:" + std::to_string(flower_num) + ", 布阵码:" + layout_code, Logger::DEBUG);

			} while (0);

			// 2. 跨关更新
			if (game_controler.board && 
				!is_crashed &&
				game_controler.board->GetBaseAddress() &&
				game_controler.board->GetMiscellaneous()->Round != current_flag &&
				game_controler.pvz->GameState == PVZGameState::Playing) {

				if (game_controler.board->GetMiscellaneous()->Round >= 25) {
					logger_data.log(std::string(get_terminal_width(), '-'), Logger::INFO);
					logger_data.log(std::string("恭喜打通!!!!"), Logger::INFO);
					logger_data.log("最后吃脑时间为: " + (leveldata.last_brain_eaten_time - start_time).cnPrint(), Logger::INFO);

					auto timeStr = (leveldata.last_brain_eaten_time - start_time).enPrint().append("     ").append(std::string("Congrats!"));
					Creator::CreateCaption(timeStr.c_str(), timeStr.size(), CaptionStyle::Lowermiddle);

					log_game_end(logger_data, save_data);
					return;
				}


				// 2.0 先布阵
				current_flag = game_controler.board->GetMiscellaneous()->Round;
				std::string layout_code = vec[current_flag];
				int theme_index = 0; int flower_num = 0; int sun = 0;
				std::array<int, 25> orders = {};
				// 检测布阵码是否合法
				if (!GenerateLayoutCode::decode_layout_string(layout_code, theme_index, flower_num, sun, orders)) {
					std::cout << "布阵码不合法" << std::endl;
					return;
				}
				game_controler.set_layout_test(layout_code, theme_index, flower_num, 0, orders);
				if (is_cheat_check) logger_cheat->log("现在对第" + std::to_string(current_flag) + "关进行布阵,花数:" + std::to_string(flower_num) + ", 布阵码:" + layout_code, Logger::DEBUG);
				leveldata.setlayout_time = TimeStruct::getNow();


				if (!has_started) continue;
				// 2.1 存档
				if (leveldata.kernel_count == 0) {
					leveldata.kernelpult_butter_rate = 0.00;
				}
				else {
					leveldata.kernelpult_butter_rate = leveldata.butter_count / (leveldata.butter_count + leveldata.kernel_count);
				}
				save_data.push_back(leveldata);

				// 2.2 打印数据
				logger_data.log((TimeStruct::getNow() - start_time).enPrint()
					+ " 已经通过" + std::to_string(game_controler.board->GetMiscellaneous()->Round)
					+ "关, 阳光" + std::to_string(game_controler.board->Sun)
					+ "，花费" + std::to_string(leveldata.zombie_cost),
					Logger::INFO);

				// 2.1 初始化数据
				// 检测跳关或者改了阳光
				if (is_cheat_check && leveldata.eaten_brain_count != 5) logger_cheat->log("检测到跳关!", Logger::INFO);
				if (is_cheat_check && game_controler.board->Sun != (leveldata.initial_sun + leveldata.collected_sun - leveldata.zombie_cost)) {
					logger_cheat->log("检测到可能修改阳光,需裁判赛后查看详细数据!", Logger::DEBUG);
					logger_data.log("阳光异常,需要裁判赛后裁定!", Logger::DEBUG);
					logger_cheat->log("现在阳光:" + std::to_string(game_controler.board->Sun)
						+ " ? "
						+ "上一关阳光: " + std::to_string(leveldata.initial_sun)
						+ " "
						+ "上一关手动点击的" + std::to_string(leveldata.collected_sun)
						+ " " 
						+ "上一关僵尸花费" + std::to_string(leveldata.zombie_cost)
						, Logger::DEBUG
					);
				}
				
				processed_projectile_ids.clear();
				processed_coin_ids.clear();
				processed_zombie_ids.clear();

				leveldata.initial_sun = game_controler.board->Sun;
				leveldata.released_zombies_count = 0;
				leveldata.zombie_cost = 0;
				leveldata.collected_sun = 0;
				leveldata.kernel_count = 0;
				leveldata.butter_count = 0;
				leveldata.kernelpult_butter_rate = 0.00;

				leveldata.brain_eaten_times = {};
				leveldata.current_time = TimeStruct::getNow() - start_time;
				leveldata.eaten_brain_count = 0;
			}

			// 3. 检测是否开始游戏
			if (!has_started) {
				// 并没开始游戏
				if (!leveldata.released_zombies_count && 
					!game_controler.pvz->GetBaseAddress() ||
					game_controler.pvz->LevelId != PVZLevel::I_Zombie_Endless ||
					game_controler.pvz->GameState != PVZGameState::Playing)
					continue;

				// 开始游戏了
				// 判断释放僵尸条件
				if (game_controler.board->ZombiesCount != 1) continue;

				has_started = true;
				start_time = TimeStruct::getNow();
				// 开始时间
				leveldata.first_zombie_release_time = start_time;
				leveldata.reaction_time = leveldata.setlayout_time - leveldata.first_zombie_release_time;
				leveldata.last_brain_eaten_time = start_time;
				logger_data.log("开始游戏!  现实时间为: " + start_time.getCurrentTime(), Logger::INFO);
				logger_data.log(std::string(get_terminal_width(), '-'), Logger::INFO);
			}

			// 4. 定时反作弊检测
			if (is_cheat_check) {
				if ((TimeStruct::getNow() - check_time).second >= game_cheat_checker.check_interval) {
					if (game_cheat_checker.check_all()) {
						logger_cheat->log("检测到作弊!", Logger::INFO);
					}
					check_time = TimeStruct::getNow(); // 这一次的检测时间
				}
			}

			// 4. 监测放置的僵尸、释放时间以及计算花费、反应时间
			std::vector<SPT<PVZ::Zombie>> zombies = game_controler.board->GetAllZombies();
			for (auto& zombie : zombies) {
				// 第一关的话多等一会儿游戏自动清除选卡界面的僵尸
				if (game_controler.board->GetMiscellaneous()->Round == 0)  Sleep(100);
				// 如果僵尸死了，跳过
				if (zombie->NotExist) {
					processed_zombie_ids.erase(zombie->Id);
					continue;
				}
				// 如果不是刚放置的僵尸, 跳过
				if (zombie->ExistedTime < 0 || zombie->ExistedTime > 2000) continue; // 只检查新生成的
				// 如果这个僵尸已经处理过了, 跳过
				if (processed_zombie_ids.count(zombie->Id)) continue; // 已处理则跳过

				processed_zombie_ids.insert(zombie->Id); // 记录已处理

				// 如果不是ize中的僵尸，而且现在又不是开局的话, 记录下来
				if (!game_controler.ZombieSunCost.count(zombie->Type)) {
					// 重开的话需要先清除选卡界面的僵尸
					if (is_cheat_check) logger_cheat->log("检测到在第" + std::to_string(zombie->Row + 1) + "行放置了非ize关卡的僵尸,僵尸类型为" + ZombieType::ToString(zombie->Type), Logger::INFO);
					continue;
				}
				// 如果是ize中的僵尸，但又不是伴舞僵尸，日志记录并且计算花费【每一关结束赋值】
				if (zombie->Type == ZombieType::BackupDancer) continue;

				auto zombie_info = game_controler.ZombieSunCost[zombie->Type];
				leveldata.zombie_cost += zombie_info.first;
				leveldata.released_zombies_count += 1;
				if (is_cheat_check) logger_cheat->log("在第" + std::to_string(zombie->Row + 1) + "行放置了" + std::string(zombie_info.second) + ",目前一共放了" + std::to_string(leveldata.released_zombies_count) + "个僵尸", Logger::DEBUG);

				// 记录反应时间
				if (leveldata.released_zombies_count == 1) {
					leveldata.first_zombie_release_time = TimeStruct::getNow();
					leveldata.reaction_time = leveldata.first_zombie_release_time - leveldata.setlayout_time;
					if (is_cheat_check) logger_cheat->log("第" + std::to_string(current_flag + 1) + "关，第一个僵尸释放时间: " + (leveldata.first_zombie_release_time - start_time).enPrint(), Logger::DEBUG);
					if (is_cheat_check) logger_cheat->log("第" + std::to_string(current_flag + 1) + "关，反应时间: " + leveldata.reaction_time.enPrint(), Logger::DEBUG);
				}
			}

			// 5. 检测玉米粒和黄油
			for (auto& projectile : game_controler.board->GetAllProjectile()) {

				if (projectile->NotExist) {
					processed_projectile_ids.erase(projectile->Id);
					continue;
				}

				if (projectile->ExistedTime < 0 || projectile->ExistedTime > 2000) continue; // 只检查新生成的

				if (processed_projectile_ids.count(projectile->Id)) continue; // 已处理则跳过
				processed_projectile_ids.insert(projectile->Id); // 记录已处理

				// 
				if (projectile->Type == ProjectileType::Kernel) {
					leveldata.kernel_count += 1;
					if (is_cheat_check) logger_cheat->log("第" + std::to_string(projectile->Row + 1) + "行" + std::to_string(projectile->ImageX) + "坐标处出现了一个玉米粒", Logger::DEBUG);
				}
				else if (projectile->Type == ProjectileType::Butter) {
					leveldata.butter_count += 1;
					if (is_cheat_check) logger_cheat->log("第" + std::to_string(projectile->Row + 1) + "行" + std::to_string(projectile->ImageX) + "坐标处出现了一个黄油", Logger::DEBUG);
				}
			}

			// 5. 检测收集的阳光
			for (auto& coin : game_controler.board->GetAllCoins()) {
				if (processed_coin_ids.count(coin->Id)) continue; // 已处理则跳过
				if (!coin->NotExist && coin->Type == CoinType::NormalSun && coin->Collected) {
					if (is_cheat_check) logger_cheat->log("点了阳光", Logger::DEBUG);
					leveldata.collected_sun += 25; // 确实是自己手动点的，但是也有自己收集的
					processed_coin_ids.insert(coin->Id); // 记录已处理
				}
				if (coin->NotExist) {
					processed_coin_ids.erase(coin->Id);
					continue;
				}
			}

			// 5. 监测脑子变化
			if (leveldata.eaten_brain_count != game_controler.countEatenBrain()
				&& leveldata.eaten_brain_count != 5) {
				leveldata.score = game_controler.board->GetMiscellaneous()->Round + game_controler.countEatenBrain() * 0.2;
				leveldata.last_brain_eaten_time = TimeStruct::getNow();
				leveldata.brain_eaten_times.push_back(leveldata.last_brain_eaten_time);
				leveldata.eaten_brain_count = game_controler.countEatenBrain();

				logger_data.log((leveldata.last_brain_eaten_time - start_time).enPrint() + "吃了第" + std::to_string(game_controler.countEatenBrain()) + "个脑子", Logger::DEBUG);
			}

			// 6. 超时: 正常是30，如果崩溃的话
			if ((TimeStruct::getNow() - start_time).minute >= 30)
			{
				auto over_time = TimeStruct::getNow() - start_time; // 用时
				logger_data.log(std::string(get_terminal_width(), '-'), Logger::INFO);

				logger_data.log("超时，游戏结束!", Logger::DEBUG); logger_cheat->log("玩家由于超时结束游戏!", Logger::DEBUG);

				std::ostringstream oss;
				oss << std::fixed << std::setprecision(1) << (std::round(leveldata.score * 10) / 10.0);
				std::string score_str = oss.str();
				logger_data.log("游戏结束! 最终得分为——  " + score_str, Logger::INFO);
				logger_data.log("最后吃脑时间为: " + (leveldata.last_brain_eaten_time - start_time).cnPrint(), Logger::INFO);


				auto time_str = (leveldata.last_brain_eaten_time - start_time).enPrint().append("     ") + score_str;
				Creator::CreateCaption(time_str.c_str(), time_str.size(), CaptionStyle::Lowermiddle); // 去掉浮点数的两位小数

				// 游戏结束，记录本局信息
				log_game_end(logger_data, save_data);

				return;
			}
			
			// 1. 监测崩溃
			if (!ProcessOpener::Open()) {//找不到游戏
				if (!is_crashed) { // 崩溃时打印一次
					logger_data.log("游戏崩溃! 进入ize后按shift+s恢复, 等待中...", Logger::DEBUG);
					logger_data.log(std::string(get_terminal_width(), '*'), Logger::INFO);
					logger_data.log("检测到游戏异常关闭", Logger::INFO);
					//std::cout << leveldata.score << std::endl;
					logger_data.log(
						"存档数据为: 通过" + std::to_string(int(leveldata.score)) + "关, 用时 " + leveldata.current_time.enPrint() + ", 阳光 " + std::to_string(leveldata.initial_sun)
						, Logger::INFO
					);
					logger_data.log("请进入pvz并进入ize后，按shift+s继续游戏", Logger::INFO);
					logger_data.log(std::string(get_terminal_width(), '*'), Logger::INFO);
					is_crashed = true;
					game_controler.board = nullptr;
					game_controler.pvz = nullptr;
					PVZ::QuitPVZ();
				}
				continue;
			}

			// 6. 还有时间，阳光用完了 
			if (game_controler.board && game_controler.board->GetBaseAddress() && game_controler.board->ZombiesCount == 0 && game_controler.board->Sun < lowestSun) {
				bool is_dead = true;
				for (auto& coin : game_controler.board->GetAllCoins()) {
					if (coin->Type == CoinType::NormalSun) is_dead = false;
				}
				Sleep(500);
				if (is_dead && ProcessOpener::Open()) {

					logger_cheat->log("玩家由于阳光用完结束游戏!", Logger::DEBUG);

					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << (std::round(leveldata.score * 10) / 10.0);
					std::string score_str = oss.str();


					auto time_str = (leveldata.last_brain_eaten_time - start_time).enPrint().append("     ") + score_str;

					logger_data.log(std::string(get_terminal_width(), '-'), Logger::INFO);
					logger_data.log((leveldata.last_brain_eaten_time - start_time).enPrint() + " 游戏结束--  得分: " + score_str, Logger::INFO);
					Creator::CreateCaption(time_str.c_str(), time_str.size(), CaptionStyle::Lowermiddle); // 去掉浮点数的两位小数

					log_game_end(logger_data, save_data);
					
					return;
				}
			}
		}

	};

	// 弹出30min倒计时
	void pop_timer() {
		// 倒计时1: 获取还剩的时间: 
	}

public:
	ConsoleControler() {
		//布阵器实现
		setlocale(LC_ALL, ".936"); // 设置编码格式
		SetConsoleTitle(WINDOW_NAME);
	}
	

	// 布阵器循环
	void main() {
		while (true) {
			// 打印功能卡
			std::cout << std::string(get_terminal_width(), '*') << std::endl;
			std::cout << INIT_WORDS << std::endl;
			std::cout << std::string(get_terminal_width(), '*') << std::endl;
			// 读指令
			std::string s;
			std::cin >> s;
			// 指令实现
			{ 
				// 使用说明
				if (!s.compare("0")) {// 使用说明
					std::cout << USE_GUIDES << std::endl;
				}
				// 30min布阵
				else if (!s.compare("1")) { // 30min限时玩法
					std::cout << "请先重开游戏, 并输入布阵码: " << std::endl;
					std::string ls;
					std::cin >> ls;

					std::cout << "是否开启反作弊(1为开启, 0为关闭): " << std::endl;
					std::string cmd1;
					std::cin >> cmd1;
					bool is_cheat_check = !cmd1.compare("1") ? true : false;

					// 创建并启动新线程（立即执行）
					disable_quick_edit_mode();
					std::thread worker([this, ls, is_cheat_check] {
						register_RaceMode_hotkey();
						SpeedRun30min(ls, is_cheat_check);
						register_RaceMode_hotkey();
						});

					// 主线程在此等待子线程完成
					worker.join();  // 阻塞直到线程结束
					enable_quick_edit_mode();
				}
				// 生成25关随机阵型代码
				else if (!s.compare("2")) { 
					GenerateLayoutCode code_generator;
					auto ls = code_generator.generate_ssb6_code();
					std::cout << ls << std::endl;
					copyToClipBoard(ls);
					std::cout << "已复制到剪贴板,可直接粘贴使用" << std::endl;
					continue;
				}
				// 残局玩法
				else if (!s.compare("3")) { 

				}
				// 冲关玩法
				else if (!s.compare("4")) {
					// 处理用户输入
					std::cout << "有女仆输入1(1为开启, 0为关闭): " << std::endl;
					std::string cmd1;
					std::cin >> cmd1;
					bool is_ban_maidCheat = (cmd1 != "1");

					// 创建并启动新线程（立即执行）
					disable_quick_edit_mode(); // 布阵器禁用编辑功能：包括复制什么的
					std::thread worker([this, is_ban_maidCheat] {
						try {
							register_RushMode_hotkey();
							LevelRush(is_ban_maidCheat);
							unregister_RushMode_hotkey();
						}
						catch (...) {
							// 异常处理（可选）
							std::cerr << "任务执行异常!" << std::endl;
						}
						});

					// 主线程在此等待子线程完成
					worker.join();  // 阻塞直到线程结束
					enable_quick_edit_mode();

				}
				// 选手生成唯一机器码 发给裁判
				else if (!s.compare("5")) { 
					auto machine_code = EncryptUtils::generate_machine_code();
					std::cout << machine_code << std::endl;
					copyToClipBoard(machine_code);
					std::cout << "本台机器的机器码已复制到剪贴板,请私发给裁判" << std::endl;
					continue;
				}
				// 裁判输入双机器码及有效期生成加密机器码
				else if (!s.compare("6")) {
					std::vector<std::string> machine_codes;
					std::string input;
					std::cout << "请输入机器码（输入 -1 结束）：" << std::endl;
					while (true) {
						std::cin >> input;
						if (input == "-1") break;
						if (input.size() != 32) {
							std::cout << "输入不合法! 请重新输入" << std::endl;
							continue;
						}
						machine_codes.push_back(input);
					}

					int expire_minute;
					do{
						std::cout << "是否设置布阵码有效期(0则不设置): " << std::endl;
						std::cin >> expire_minute;
						if (expire_minute < 0) std::cout << "输入不合法!" << std::endl;
					} while (expire_minute < 0);
					

					// 获取当前时间戳
					std::string timeSecond_expire = std::to_string(static_cast<std::size_t>(std::time(nullptr)));


					GenerateLayoutCode code_geneator;
					auto ls = code_geneator.generate_ssb6_code();

					// 拼接规则和加密
					std::string encode_data = EncryptUtils::encode_ls(machine_codes, timeSecond_expire, std::to_string(expire_minute), ls);
					copyToClipBoard(encode_data);
					std::cout << encode_data << std::endl;
					std::cout << "加密布阵码已复制到剪贴板,请私发给选手" << std::endl;
				}
				// 选手解密后进行布阵
				else if (!s.compare("7")) {
					// 1. 输入加密数据（比如从剪贴板或用户输入获取）
					std::string enc_ls;
					std::cout << "请输入加密布阵码：" << std::endl;
					std::cin >> enc_ls;

					std::vector<std::string> machine_codes;
					std::string timeSecond_expire;
					std::string expire_minute;
					std::string ls;

					if (!EncryptUtils::decode_ls(enc_ls, machine_codes, timeSecond_expire, expire_minute, ls)) {
						std::cout << "解密失败!" << std::endl;
						continue;
					}

					std::string machine_code = EncryptUtils::generate_machine_code();
					// 1. 如果机器码不在列表中，不合法continue
					bool found = false;
					for (const auto& code : machine_codes) {
						if (code == machine_code) {
							found = true;
							break;
						}
					}
					if (!found) {
						std::cout << "输入不合法!" << std::endl;
						continue;
					}
					
					// 2. 时间戳转换成数字，必须比当前时间的时间戳小, 当前时间戳是: static_cast<std::size_t>(std::time(nullptr))
					size_t ts = 0;
					try {
						ts = std::stoull(timeSecond_expire);
					}
					catch (...) {
						std::cout << "输入不合法!" << std::endl;
						continue;
					}
					size_t current_ts = static_cast<size_t>(std::time(nullptr));
					if (ts > current_ts) {
						std::cout << "输入不合法!" << std::endl;
						continue;
					}
					// 3. 有效期分钟数转换成数字，必须>=0, 如果是0则继续后续操作；如果不是0，则要求当前时间-拿到的时间戳的分钟数<有效期
					int expireMin = 0;
					try {
						expireMin = std::stoi(expire_minute);
					}
					catch (...) {
						std::cout << "有效期格式错误!" << std::endl;
						continue;
					}

					if (expireMin < 0) {
						std::cout << "有效期不能为负数!" << std::endl;
						continue;
					}
					if (expireMin != 0) { // 如果设置了0代表是不加有效期，如果没设置就是设置了有效期的
						if ((TimeStruct::getNow() - TimeStruct(ts)).minute > expireMin) {
							std::cout << "布阵码已经超过有效期:" << expire_minute << "min! 请裁判重新刷一个" << std::endl;
							continue;
						}
						else {
							std::cout << "此布阵码有效期为" << expire_minute << "min" << std::endl;
						}
					}

					bool is_cheat_check = true;
					// 创建并启动新线程（立即执行）
					disable_quick_edit_mode();
					std::thread worker([this, ls, is_cheat_check] {
						register_RaceMode_hotkey();
						SpeedRun30min(ls, is_cheat_check);
						register_RaceMode_hotkey();
						});

					// 主线程在此等待子线程完成
					worker.join();  // 阻塞直到线程结束
					enable_quick_edit_mode();

				}
				// 缩主题锁花数练习
				else if (!s.compare("8")) { // 指定主题指定花数练习
					std::cout << "请输入主题号(1-8): " << std::endl;
					int theme;
					std::cin >> theme;

					// 检测输入是否失败或者数字范围不合法
					if (std::cin.fail() || theme < 1 || theme > 8) {
						std::cout << "输入不合法!\n";
						// 清除错误标志
						std::cin.clear();
						// 忽略当前行剩余的输入
						std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
						continue;
					}

					std::cout << "请输入花数(1-8):" << std::endl;
					int flower_num;
					std::cin >> flower_num;

					if (std::cin.fail() || flower_num < 1 || flower_num > 8) {
						std::cout << "输入不合法!\n";
						std::cin.clear();
						std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
						continue;
					}

					// 如果输入都合法，可以跳出循环或者进行后续操作
					std::cout << "你输入的主题号为: " << theme
						<< "，花数为: " << flower_num << std::endl;



					// 创建并启动新线程（立即执行）
					disable_quick_edit_mode();
					std::thread worker([this, theme, flower_num] {
						try {
							register_reviewMode_hotkey();
							lock_theme_flower_num(theme, flower_num);
							unregister_reviewMode_hotkey();
						}
						catch (...) {
							// 异常处理（可选）
							std::cerr << "任务执行异常!" << std::endl;
						}
						});

					// 主线程在此等待子线程完成
					worker.join();  // 阻塞直到线程结束
					enable_quick_edit_mode();

				}
				// 导出本关ize阵型代码
				else if (!s.compare("a")) {
					DWORD pid = ProcessOpener::Open();
					if (!pid) continue;
					PVZ::InitPVZ(pid);
					GameControl game_controler(pid);

					std::string ls;
					game_controler.export_layout_string(ls);

					copyToClipBoard(ls);
					std::cout << ls << std::endl;
					std::cout << "当前阵型已导出为阵型代码,已复制到剪贴板,可直接粘贴使用" << std::endl;
					PVZ::QuitPVZ();
				}
				// 弹出磁铁倒计时
				else if (!s.compare("b")) {
					

				}
				else if (!s.compare("c")) {
					std::cout << "还没写" << std::endl;

				}
			}
		}
	}

};



int main() {
	// 实例化布阵器控制
	ConsoleControler console_controler;
	console_controler.main();
	return 0;
};
