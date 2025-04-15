#include "IZESpeedLayout.h"

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
#include "LevelData.hpp"
// 时间类【参考六届布阵器】
#include "TimeStruct.h"

// 全局随机数
std::random_device rd;
std::mt19937_64 gen(rd()); // 全局随机数生成器

// 关闭和暂停子线程
std::atomic<bool> stop_flag(false);
std::atomic<bool> pause_flag(false);

// 统计阳光的线程向反作弊线程传数据
std::mutex g_mutex;
std::queue<int> g_data_queue;


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
	DWORD pid;
	
	// 僵尸简称
	std::unordered_map < ZombieType::ZombieType, std::string > zombie_str = {
		{ ZombieType::Imp,  "鬼" },
		{ ZombieType::ConeheadZombie,      "障" },
		{ ZombieType::PoleVaultingZombie,  "杆"},
		{ ZombieType::BucketheadZombie,    "桶"} ,
		{ ZombieType::BungeeZombie,        "偷" },
		{ ZombieType::DiggerZombie,        "矿" },
		{ ZombieType::LadderZombie,        "梯" },
		{ ZombieType::FootballZombie,      "橄"} ,
		{ ZombieType::DancingZombie,       "舞"},
	};

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

	//GameControl() {};

	bool find_pvz() {
		// 1. 寻找 pvz
		this->pid = ProcessOpener::Open();
		if (!pid) return false;
		EnableBackgroundRunning(true); // 启用pvz后台运行
		
		PVZ::InitPVZ(this->pid);

		this->pvz = PVZ::GetPVZApp();
		if (!this->pvz) {
			PVZ::QuitPVZ();
			return false;
		}
		this->board = PVZ::GetBoard();
		if (!this->board) {
			PVZ::QuitPVZ();
			return false;
		}
		return true;
	}

	bool refind_pvz() {
		DWORD pid = ProcessOpener::Open();
		if (pid && this->pid == pid) return true;

		PVZ::QuitPVZ();
		this->pid = pid;
		EnableBackgroundRunning(true); // 启用pvz后台运行
		PVZ::InitPVZ(this->pid);
		this->board = PVZ::GetBoard();
		if (!this->board->GetBaseAddress()) {
			PVZ::QuitPVZ();
			return false;
		}
		this->pvz = PVZ::GetPVZApp();
		if (!this->pvz->GetBaseAddress()) {
			PVZ::QuitPVZ();
			return false;
		}
		return true;
	}

	void clear_caption() {
		if (is_in_ize()) {
			AsmCode code;
			code.asm_init();
			code.asm_mov_exx(AsmCode::Reg::ESI, PVZ::Memory::ReadPointer(0x6a9ec0, 0x768));
			code.asm_call(0x40CA50);
			code.asm_ret();
			code.asm_code_inject(PVZ::Memory::hProcess);
		}
	}

	// 改pvz窗口名
	void modify_pvz_handle_title(const LPCSTR title) {
		if (this->pvz) {
			SetWindowTextA(PVZ::Memory::mainwindowhandle, title); // 禁掉一些寻找游戏是通过窗口名的：如算血器
		}
	}
	
	// 【bug】清除重开时的僵尸
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
	void set_layout_seed(const std::string& ls, int flower_num) {
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
	void set_layout_order(const int theme_index, const int flower_num, const int sun, const std::array<int, 25>orders) {
		if (is_in_ize()) {
			// 按道理是停掉了游戏的种植的
			for (auto plant : board->GetAllPlants()) {
				if (plant->NotExist) continue;
				clear_reverse_all_plants();
			}
			// 获得种植位置
			auto plantTypes = GenerateLayoutCode::get_theme_plants(flower_num, static_cast<Theme>(theme_index)); // 获取不同主题的植物生成顺序
			if (sun != 0 && sun % 25==0) {
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
				int row = i, column = 0;
				SPT<PVZ::IZBrain> iz_brain = MKS<PVZ::IZBrain>(Creator::CreateGriditem()->GetBaseAddress());
				iz_brain->Row = row;
				iz_brain->Column = column;
				iz_brain->Layer = row * 0x2710 + 0x49BB0;
				iz_brain->Type = GriditemType::IZBrain;
				iz_brain->NotExist = false;
				iz_brain->Hp = 70;
				iz_brain->X = PVZ::GetBoard()->GridToXPixel(row, column) - 40.0;
				iz_brain->Y = PVZ::GetBoard()->GridToYPixel(row, column) + 40.0;
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

	// 统计场上还有的阳光
	void calc_board_sun() {

	}

	// 栈位逆序删植物
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

};

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
			logger_cheat->log("检测到速度异常，帧间隔异常，" + std::to_string(time_ms), Logger::DEBUG);
			return true;
		}

		// pvz自带加速: 25px, 0.25px
		if (PVZ::Memory::ReadMemory<bool>(0x6A9EAB) || PVZ::Memory::ReadMemory<bool>(0x6A9EAA)) {
			logger_cheat->log("检测到速度异常，可能启用了20px和0.25px", Logger::DEBUG);
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
			{PVZ::Memory::ReadMemory<float>(0x679660), 0.37, "其他僵尸相对速度浮动上界"},


			{ PVZ::Memory::ReadMemory<byte>(0x52A99F), byte(217), "扶梯速度" },
			{ PVZ::Memory::ReadMemory<byte>(0x52852D), byte(217), "矿工晕" },
			{ PVZ::Memory::ReadMemory<byte>(0x52834C), byte(217), "钻头矿工" },
			{ PVZ::Memory::ReadMemory<byte>(0x525E7B), byte(217), "撑杆跳" },
			{ PVZ::Memory::ReadMemory<byte>(0x525109), byte(217), "蹦极落地后速度" },
			{ PVZ::Memory::ReadMemory<byte>(0x528D33), byte(217), "舞王召唤速度" },
			{ PVZ::Memory::ReadMemory<byte>(0x40B06F), byte(150), "我是僵尸初始阳光150" },
			{ PVZ::Memory::ReadMemory<byte>(0x5234F3), byte(12), "舞王僵尸滑步时间偏差" },
			{ PVZ::Memory::ReadMemory<byte>(0x52350A), byte(300), "舞王僵尸滑步时间下界" },

		};

		// 检查每个速度值是否符合预期
		for (const auto& check : speed_checks) {
			if (check.current_value != check.expected_value) {
				logger_cheat->log(
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

	// 检测子弹攻击力常量是否被修改
	bool check_damage_constant() {

		bool data_changed = false; // 记录是否有数据变化
		// 定义速度和预期值的对照表
		struct SpeedCheck {
			float current_value;
			float expected_value;
			const char* name;
		};

		// 初始化每个检查项
		SpeedCheck speed_checks[] = {
			{PVZ::Memory::ReadMemory<int>(0x69F1C8), 20, "普通豌豆攻击力"},
			{PVZ::Memory::ReadMemory<int>(0x69F1D4), 20, "冰豌豆攻击力"},
			{PVZ::Memory::ReadMemory<int>(0x69F1F8), 20, "孢子攻击力"},
			{PVZ::Memory::ReadMemory<int>(0x69F210), 40, "火豌豆攻击力"},
			{PVZ::Memory::ReadMemory<int>(0x69F21C), 20, "星星攻击力"},
			{PVZ::Memory::ReadMemory<int>(0x69F228), 20, "尖刺攻击力"},
			{PVZ::Memory::ReadMemory<int>(0x69F240), 20, "玉米粒攻击力"},
			{PVZ::Memory::ReadMemory<int>(0x69F258), 40, "黄油攻击力"},
		};

		// 检查每个速度值是否符合预期
		for (const auto& check : speed_checks) {
			if (check.current_value != check.expected_value) {
				logger_cheat->log(
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

	// 检测新生成的植物血量常量
	bool check_plant_constant() {
		bool data_changed = false; // 记录是否有数据变化
		// 定义速度和预期值的对照表
		struct SpeedCheck {
			float current_value;
			float expected_value;
			const char* name;
		};

		// 初始化每个检查项
		SpeedCheck speed_checks[] = {
			{PVZ::Memory::ReadMemory<int>(0x45DC55), 300, "一般植物的血量"},
			{PVZ::Memory::ReadMemory<int>(0x45E1A7), 4000, "坚果血量"},
			{PVZ::Memory::ReadMemory<int>(0x532FDC), 1800, "灰烬攻击力"},
			{PVZ::Memory::ReadMemory<byte>(0x4309F0), byte(25), "普通阳光价值"},
			{PVZ::Memory::ReadMemory<byte>(0x69F2CC), byte(150), "植物射速"},
			{PVZ::Memory::ReadMemory<byte>(0x45F8B6), byte(15), "植物攻击间隔误差(负值)"},
			{PVZ::Memory::ReadMemory<byte>(0x45F1E1), byte(4), "玉米投手黄油概率的倒数"},
		};

		// 检查每个速度值是否符合预期
		for (const auto& check : speed_checks) {
			if (check.current_value != check.expected_value) {
				logger_cheat->log(
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


	// 检测僵尸本体及饰品血量常量
	bool check_zombie_hp_constant() {

		bool data_changed = false; // 记录是否有数据变化
		// 定义速度和预期值的对照表
		struct SpeedCheck {
			float current_value;
			float expected_value;
			const char* name;
		};

		// 初始化每个检查项
		SpeedCheck speed_checks[] = {
			{PVZ::Memory::ReadMemory<int>(0x5227BB), 270, "一般僵尸血量"},
			{PVZ::Memory::ReadMemory<int>(0x522892), 370, "路障饰品血量"},
			{PVZ::Memory::ReadMemory<int>(0x522CBF), 500, "撑杆僵尸血量"},
			{PVZ::Memory::ReadMemory<int>(0x52292B), 1100, "铁桶饰品血量"},
			{PVZ::Memory::ReadMemory<int>(0x522BB0), 1400, "橄榄球饰品血量"},
			{PVZ::Memory::ReadMemory<int>(0x523530), 500, "舞王僵尸血量"},
			{PVZ::Memory::ReadMemory<int>(0x522BEF), 100, "矿工僵尸血量(?应该是饰品血量)"},
			{PVZ::Memory::ReadMemory<int>(0x522A1B), 450, "蹦极僵尸血量"},
			{PVZ::Memory::ReadMemory<int>(0x52299C), 500, "梯子僵尸血量"},
			{PVZ::Memory::ReadMemory<int>(0x5235AC), 70, "小鬼僵尸在ize的血量"},
		};

		// 检查每个速度值是否符合预期
		for (const auto& check : speed_checks) {
			if (check.current_value != check.expected_value) {
				logger_cheat->log(
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
			logger_cheat->log("检测到开启了免费种植!", Logger::DEBUG);
			return true;
		}

		return false;
	}

	// 检测是否锁玉米或者黄油
	bool check_Kernelpult() {
		if (PVZ::Memory::ReadMemory<byte>(0x45F1EC) == byte(235)) {
			logger_cheat->log("检测到锁玉米粒!", Logger::DEBUG);
			return true;
		}
		if (PVZ::Memory::ReadMemory<byte>(0x45F1EC) == byte(112)) {
			logger_cheat->log("检测到锁黄油!", Logger::DEBUG);
			return true;
		}
		if (PVZ::Memory::ReadMemory<byte>(0x45F1EC) != byte(117)) {
			logger_cheat->log("检测到玉米子弹异常!", Logger::DEBUG);
			return true;
		}


		return false;
	}

	// 检测是否开启dance
	bool check_dance() {
		auto board = PVZ::GetBoard();
		if (board->Dance) {
			logger_cheat->log("检测到开启dance!, " + std::to_string(PVZ::Memory::ReadMemory<bool>(PVZ::Memory::ReadPointer(0x6A9EC0, 0x768) + 0x5765)), Logger::DEBUG);
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
				logger_cheat->log(
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
				logger_cheat->log(
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
			logger_cheat->log("铲过植物数量: " + std::to_string(PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x579c)), Logger::DEBUG);
			return true;
		}
		return false;
	}

	// 检测僵尸速度分布
	bool check_zombie_speed() {
		if (PVZ::Memory::ReadMemory<byte>(0x52b215) != byte(117)) {
			logger_cheat->log("开启僵尸速度加快", Logger::DEBUG);
			return true;
		}
		if (PVZ::Memory::ReadMemory<short int>(0x0052F103) != 21620) {
			logger_cheat->log("开启僵尸速度更快", Logger::DEBUG);
			return true;
		}
		if (PVZ::Memory::ReadMemory<byte>(0x52aaad) != byte(116)) {
			logger_cheat->log("开启僵尸匀速前进", Logger::DEBUG);
			return true;
		}
		if (PVZ::Memory::ReadMemory<byte>(0x00531045) != byte(200)) {
			logger_cheat->log("僵尸状态异常: 疑似开了僵尸无敌", Logger::DEBUG);
			return true;
		}
		return false;
	}

	// 将测僵尸状态
	bool check_zombie_status() {
		if (PVZ::Memory::ReadMemory<byte>(0x0053095C) != byte(132)) {
			logger_cheat->log("开启僵尸免疫减速", Logger::DEBUG);
			return true;
		}
		if (PVZ::Memory::ReadMemory<byte>(0x00531A1A) != byte(116)) {
			logger_cheat->log("开启僵尸免疫黄油", Logger::DEBUG);
			return true;
		}
		if (PVZ::Memory::ReadMemory<byte>(0x004620C5) != byte(2)
			|| PVZ::Memory::ReadMemory<byte>(0x004620CA) != byte(3)
			|| PVZ::Memory::ReadMemory<byte>(0x004620D5) != byte(1)
			|| PVZ::Memory::ReadMemory<byte>(0x004620DA) != byte(3)
			|| PVZ::Memory::ReadMemory<byte>(0x004620DF) != byte(15)
			|| PVZ::Memory::ReadMemory<byte>(0x004620ED) != byte(0)
			) {
			logger_cheat->log("开启僵尸免疫磁力菇", Logger::DEBUG);
			return true;
		}
		if (PVZ::Memory::ReadMemory<byte>(0x004248AA) != byte(117)) {
			logger_cheat->log("开启僵尸快跑", Logger::DEBUG);
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
			logger_cheat->log("开启植物虚弱", Logger::DEBUG);
			return true;
		}
		return false;
	}

	// 检测是否开启自动收集
	bool check_auto_collected() {
		if (PVZ::Memory::ReadMemory<byte>(0x0043158f) != 0x75) {
			logger_cheat->log("检测到开启自动收集", Logger::DEBUG);
			PVZ::Memory::WriteMemory<byte>(0x0043158f, 0x75);
			return true;
		}
		return false;
	}

	// 检测红线位置
	bool check_redline() {
		if ((PVZ::Memory::ReadMemory<int>(0x004253F7) - 10) / 83 != 5) {
			return true;
		}
		return false;
	}

	// 扫描内存中常修改的数据
	bool check_memory() {
		// 游戏速度
		if (check_speed()) {
			logger_cheat->log("检测到速度异常", Logger::DEBUG);
			return true;
		}
		// 自动收集
		else if (check_auto_collected()) {
			logger_cheat->log("检测到开启自动收集", Logger::DEBUG);
			return true;
		}
		// 速度常量
		else if (check_speed_constant()) {
			logger_cheat->log("检测到速度常量异常", Logger::DEBUG);
			return true;
		}
		// 子弹攻击力常量
		else if (check_damage_constant()) {
			logger_cheat->log("检测到子弹攻击力常量异常", Logger::DEBUG);
			return true;
		}
		// 植物血量常量
		else if (check_plant_constant()) {
			logger_cheat->log("检测到植物血量及其他常量异常", Logger::DEBUG);
			return true;
		}
		// 僵尸血量及饰品常量
		else if (check_zombie_hp_constant()) {
			logger_cheat->log("检测到僵尸血量及饰品常量异常", Logger::DEBUG);
			return true;
		}
		// 锁玉米黄油
		else if (check_Kernelpult()) {
			logger_cheat->log("检测到玉米异常", Logger::DEBUG);
			return true;
		}
		// 是否使用rnd修改随机数
		else if (check_rnd()) {
			logger_cheat->log("检测到随机数异常", Logger::DEBUG);
			return true;
		}
		// 是否使用过铲子
		else if (check_shovel()) {
			logger_cheat->log("检测使用过铲子", Logger::DEBUG);
			PVZ::Memory::WriteMemory(PVZ::Memory::ReadPointer(0x6a9ec0, 0x768) + 0x579c, 0);//报过一次作弊后就改回0
			return true;
		}
		// 是否开启dance
		else if (check_dance()) {
			logger_cheat->log("检测出dance", Logger::DEBUG);
			return true;
		}
		// 是否开启免费种植
		else if (check_free_plant()) {
			logger_cheat->log("检测出免费种植", Logger::DEBUG);
			return true;
		}
		// 僵尸速度状态是否异常：匀速，变速什么的
		else if (check_zombie_speed()) {
			logger_cheat->log("检测出僵尸速度异常", Logger::DEBUG);
			return true;
		}
		// 僵尸状态是否异常：无敌，免疫
		else if (check_zombie_status()) {
			logger_cheat->log("检测出僵尸状态异常", Logger::DEBUG);
			return true;
		}
		// 植物状态是否异常：被秒吃，虚弱
		else if (check_plant_status()) {
			logger_cheat->log("检测出僵尸状态异常", Logger::DEBUG);
			return true;
		}
		// 红线位置
		else if (check_redline()) {
			logger_cheat->log("检测出红线异常", Logger::DEBUG);
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
				logger_cheat->log("应用标题: " + WideToANSI(win.title) + " (PID: " + std::to_string(win.pid) + ")", Logger::DEBUG);
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

				logger_cheat->log(
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
				logger_cheat->log(
					"第" + std::to_string(plant->Row + 1) +"行, 第" + std::to_string(plant->Row + 1) + "列,坐标为:(" + std::to_string(plant->ImageX) + "," + std::to_string(plant->ImageY)
					+"), 栈位为 : " + std::to_string(plant->Index) + "的" + PlantType::ToString(plant->Type)
					+ "hp为: " + std::to_string(plant->Hp) 
					+ ", 最大血量为: " + std::to_string(plant->MaxHp) 
					+ ", 属性倒计时(一般是磁力菇): " + std::to_string(plant->AttributeCountdown)
					, Logger::DEBUG
				);
			}
		}
	}

	// 记录鼠标数据
	void log_mouse() {
		logger_cheat->log(PVZ::Memory::ReadPointer(0x6a9ec0, 0x320, 0xdc) ? "鼠标移出屏幕" : "鼠标还在屏幕内", Logger::DEBUG);
		logger_cheat->log("鼠标坐标: " + std::to_string(PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x5558)) + "-" + std::to_string(PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x555c)), Logger::DEBUG);
	}

	// 扫描是否只有1个pvz进程
	bool check_n_PvzWindow() {
		int valid_count = 0;
		EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
			// 只统计可见主窗口
			if (!IsWindowVisible(hwnd) || GetParent(hwnd) != nullptr) {
				return TRUE;
			}

			char title[256] = { 0 };
			GetWindowTextA(hwnd, title, sizeof(title));

			std::string window_title = title;
			if (window_title != "Plants vs. Zombies" && window_title != "Plants vs. Zombies(cheatCheck: open, maidCheat: disable)") {
				return TRUE; // continue enumeration
			}

			DWORD pid = 0;
			GetWindowThreadProcessId(hwnd, &pid);
			if (pid == 0) return TRUE;

			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
			if (!hProcess) return TRUE;

			char exeName[MAX_PATH] = { 0 };
			if (GetModuleBaseNameA(hProcess, nullptr, exeName, MAX_PATH)) {
				std::string process_name = exeName;
				if (process_name == "PlantsVsZombies.exe") {
					int* count_ptr = reinterpret_cast<int*>(lParam);
					(*count_ptr)++;
				}
			}

			CloseHandle(hProcess);
			return TRUE;
			}, reinterpret_cast<LPARAM>(&valid_count));

		if (valid_count != 1) {
			logger_cheat->log("检测到多个pvz窗口, 请关闭多余的!", Logger::INFO);
			logger_cheat->log("检测到 " + std::to_string(valid_count) + " 个 PvZ 窗口!", Logger::DEBUG);
			return true;
		}

		return false;
	}

public:
	int check_interval;
	Logger* logger_layout; // 布阵器日志类
	Logger* logger_cheat; // 反作弊检测日志类
	DWORD cheat_check_thread_id;
	DWORD count_sun_thread_id;

	// 类实例化：检测间隔(s)，日志记录类引用
	GameCheatCheck(int interval, Logger* logger_ref1, Logger* logger_ref2)
		: check_interval(interval), logger_layout(logger_ref1), logger_cheat(logger_ref2) // 初始化列表
	{
	}

	// 检测和扫描所有可疑项目：游戏速度，内存数据，植物和僵尸数据，后台进程，是否多开pvz
	bool check_all() {
		// 扫描鼠标
		log_mouse();
		// 扫描植物
		log_zombie_plant();
		// 扫描后台进程
		log_background_process();
		// 扫描游戏内存
		if (check_memory()) {
			logger_cheat->log("检测出内存异常!", Logger::DEBUG);
			return true;
		}
		// 扫描是否有多个pvz
		if (check_n_PvzWindow()) {
			logger_cheat->log("检测出pvz多开!", Logger::DEBUG);
			return true;
		}
		return false;
	}

	// 开启反作弊检测的环境初检测
	void check_envirnoment() {
		if (check_all()) {
			logger_layout->log("当前pvz环境检测结果: 异常!", Logger::INFO);
		}
		else {
			logger_layout->log("当前pvz环境检测结果: 正常，请继续游戏!", Logger::INFO);
		}
		return;
	}

	// 统计阳光的线程启动函数
	void count_sun_thread() {
		this->count_sun_thread_id = GetCurrentThreadId();
		GameControl game_controler;
		while (!game_controler.find_pvz()) std::this_thread::sleep_for(std::chrono::milliseconds(1));

		std::unordered_set<int> processed_coin_ids;
		std::unordered_map<int, bool> collect_flag;

		while (!stop_flag) {
			if (pause_flag) {
				do { // 如果没board才找
					if (game_controler.board) continue;
					while(!game_controler.refind_pvz()) std::this_thread::sleep_for(std::chrono::milliseconds(10));
				} while (0);
				continue;
			}

			for (auto coin : game_controler.board->GetAllCoins()) {
				// 只处理阳光
				if (coin->Type != CoinType::NormalSun) continue;
				// 如果消失了就删除
				if (coin->NotExist) {
					processed_coin_ids.erase(coin->Id);
					continue;
				}
				// 如果处理过了
				if (processed_coin_ids.count(coin->Id)) continue;

				// 还没处理过: 阳光收集了但是还没记录过
				if (coin->Collected && !collect_flag[coin->Id]) {
					processed_coin_ids.insert(coin->Id);
					{
 						logger_cheat->log("点了阳光, 地址:" + std::to_string(coin->GetBaseAddress()), Logger::DEBUG);
						std::lock_guard<std::mutex> lock(g_mutex);
						g_data_queue.push(1);
					}
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}



	// 反作弊检测类的线程启动函数
	void cheat_check_thread(const std::vector<std::string> ls_vec) {

		// 设定检测时间
		// ----------------- 0. 找到pvz -------------------------
		GameControl game_controler;
		while (!game_controler.find_pvz()) { Sleep(1); };
		logger_cheat->log("已找到pvz!", Logger::DEBUG);
		while (!game_controler.is_in_ize()) { Sleep(1); };
		logger_cheat->log("已进入ize!", Logger::DEBUG);
		TimeStruct check_time = TimeStruct::getNow();
		this->cheat_check_thread_id = GetCurrentThreadId();

		logger_layout->log("----------------反作弊检测已开启(" + std::to_string(this->cheat_check_thread_id) + ")------------------", Logger::DEBUG);
		game_controler.modify_pvz_handle_title("Plants vs. Zombies(cheatCheck: open, maidCheat: disable)"); // 禁掉一些寻找游戏是通过窗口名的：如算血器

		
		// ----------------- 1.最开始记录一次详细信息 -----------------------
		logger_cheat->log(
			std::string("玩家已经进入ize, ")
			+ "当前日期与时间为: " + TimeStruct::getCurrentDateTime() + "\n"
			+ "执行本次计时的线程号: " + std::to_string(this->cheat_check_thread_id) + " 游戏进程号: " + std::to_string(game_controler.pid) + "\n"
			+ "游戏宽高: " + std::to_string(PVZ::Memory::ReadPointer(0x6a9ec0, 0xc0)) + "-" + std::to_string(PVZ::Memory::ReadPointer(0x6a9ec0, 0xc4)) + "\n"
			+ "游戏窗口坐标: " + std::to_string(PVZ::Memory::ReadPointer(0x6a9ec0, 0x320, 0x94, 0x30)) + "-" + std::to_string(PVZ::Memory::ReadPointer(0x6a9ec0, 0x320, 0x94, 0x34)) + "\n"
			+ "游戏内玩家名字为: " + game_controler.get_player_name()
			, Logger::DEBUG
		);


		// 初始化数据
		int current_flag = 0;
		auto current_address = game_controler.board->GetBaseAddress();
		TimeStruct start_time = TimeStruct::getNow();
		bool has_started = false;
		LevelData leveldata;
		std::vector<LevelData> save_data;
		// 只开一个阳光统计线程
		bool has_start_collect_sun_thread = false;

		{
			leveldata.initial_sun = 150;
			leveldata.flower_num = ls_vec[current_flag][1]-'0';
		}
		std::unordered_set<int> processed_zombie_ids;  // 记录已经监测并处理的僵尸
		std::unordered_set<int> processed_projectile_ids;  // 记录已经监测并处理的僵尸
		std::unordered_set<uintptr_t> processed_coins;                 // 已处理阳光地址
		std::unordered_map<uintptr_t, bool> collected_sun_map;             // 地址 -> 是否已收集

		// 要记录的数据
		int restart_num = 0;



		// ------------------- 2. 开始循环检测 -------------------------
		while (!stop_flag) { // 没暂停就一直检测
			// 如果暂停子线程
			if (pause_flag) {
				//logger_cheat->log("当前时间暂停检测!", Logger::DEBUG);
				std::this_thread::sleep_for(std::chrono::milliseconds(100));// 等一下
				continue; // 暂停了
			}
			try{
				// ----------------------- 2.1 监测游戏开始 -------------------------
				if (!has_started) {
					// 跳过已经开始游戏的
					if (!game_controler.board || // 找不到board
						game_controler.pvz->LevelId != PVZLevel::I_Zombie_Endless ||
						game_controler.pvz->GameState != PVZGameState::Playing)
						continue;
					// 判断游戏开始条件:放了一个僵尸
					if (game_controler.board->ZombiesCount != 1) continue;
					// 开始游戏了
					has_started = true;
					leveldata.flower_num = 8; leveldata.initial_sun = 150;
					start_time = TimeStruct::getNow();
					leveldata.current_use_time = TimeStruct(0);
					// 第一个僵尸释放时间，记录第一关的反应时间，僵尸释放时间-布阵时间
					leveldata.first_zombie_release_time = start_time;
					leveldata.reaction_time = leveldata.first_zombie_release_time - leveldata.setlayout_time;
					logger_cheat->log("开始游戏! 布阵到放置僵尸耗时: " + leveldata.reaction_time.enPrint(), Logger::DEBUG);
				}


				// ----------------------- 2.2 重开次数 ------------------------------------------------------
				do { // board不正确才是重开
					if (!game_controler.pvz // 如果pvz都找不到了就不算重开了
						|| PVZ::Memory::ReadPointer(0x6a9ec0, 0x768) == 0 // 如果完全没有board就是不在ize界面了
						|| current_address == PVZ::Memory::ReadPointer(0x6a9ec0, 0x768) // 如果当前的board地址和记录的地址相同就代表没啥问题
						|| game_controler.pvz->GameState != PVZGameState::Playing // 如果现在没在进行游戏，那就不用管了
						) continue;
					// 确实重开了
					while (current_address != PVZ::Memory::ReadPointer(0x6a9ec0, 0x768)) {
						std::this_thread::sleep_for(std::chrono::milliseconds(100));// 等一下pvzclass找到新board
						game_controler.board = PVZ::GetBoard();// 重新拿一下board;
						current_address = game_controler.board->GetBaseAddress();
					}
					restart_num += 1;
					logger_cheat->log("游戏重开了, 当前游戏重开次数为: "  + std::to_string(restart_num) + "!", Logger::DEBUG);
					if (current_flag == 0) has_started = false;
					leveldata.init();
				} while (0);


				// ----------------------- 2.3 监测跨关： 是否存在跳关，修改阳光 ------------------------------
				do {
					if (!has_started // 只检查开始游戏后的跨关即可
						||!game_controler.pvz // 游戏崩溃了就别进跨关
						|| !game_controler.board
						|| current_address != game_controler.board->GetBaseAddress()
						|| game_controler.board->GetMiscellaneous()->Round == current_flag
						|| game_controler.pvz->GameState != PVZGameState::Playing) continue;

					// -------------- 2.3.1 通关了 --------------
					if (game_controler.board->GetMiscellaneous()->Round >= 25) logger_cheat->log("玩家通关了!", Logger::DEBUG);

					// -------------- 2.3.2 正常跨关 --------------
					current_flag = game_controler.board->GetMiscellaneous()->Round;
					
	
					// ------------------------ 统计过关数据 ------------------------------------------------
					std::this_thread::sleep_for(std::chrono::milliseconds(200)); //等布阵器输出完毕
					// (1) 过关提示
					std::ostringstream oss_tmp;
					oss_tmp << std::fixed << std::setprecision(1) << (std::round(leveldata.score * 10) / 10.0);
					std::string score_str = oss_tmp.str();
					std::string level_str = "已经使用: " + (TimeStruct::getNow() - start_time).enPrint()
						+ " 已经通过" + std::to_string(game_controler.board->GetMiscellaneous()->Round)
						+ "关, 阳光" + std::to_string(game_controler.board->Sun)
						+ ", 花费" + std::to_string(leveldata.zombie_cost)
						+ ", 收集阳光" + std::to_string(leveldata.collected_sun)
						+ ", 遗漏阳光" + std::to_string(leveldata.flower_num * 200 - leveldata.collected_sun)
						+ ", 目前得分" + score_str;

					logger_layout->log(level_str, Logger::DEBUG);
					logger_cheat->log(level_str, Logger::DEBUG);

					// (2) 打印每关的具体解法, 最后的数据为
					// 释放时间:std::get<2>(it)
					std::array<std::string, 5> solve_str_row = {}; std::string solve_str_total;
					std::array<int, 5> solve_cost_row = {}; std::string cost_str_total;
					// 按行来的
					for (int i = 0; i < 5; i++) {
						for (auto it : leveldata.solve_info) {
							if (std::get<1>(it) == i) {
								// 记录每条路花费< ZombieType::ZombieType, std::pair<int, std::string >>
								solve_cost_row[i] += game_controler.ZombieSunCost[std::get<0>(it)].first;
								solve_str_row[i] += game_controler.zombie_str[std::get<0>(it)];
							}
						}
						cost_str_total += std::to_string(solve_cost_row[i]) + "+";
						solve_str_total += "(" + std::to_string(i + 1) + ")" + solve_str_row[i] + ";";
					}
					// 【TODO】按时间,如delay...

					cost_str_total = cost_str_total.substr(0, cost_str_total.size() - 1) + "=" + std::to_string(leveldata.zombie_cost);
					solve_str_total = "花费: " + cost_str_total + "\t解法: " + solve_str_total.substr(0, solve_str_total.size() - 1) + "\t耗时: " + (leveldata.last_brain_eaten_time - leveldata.setlayout_time).enPrint();
					logger_layout->log(solve_str_total, Logger::DEBUG);
					logger_cheat->log(solve_str_total, Logger::DEBUG);

					// (3) 吃脑时间
					std::string eat_brain_time_str = "吃脑时间: ";
					for (auto it : leveldata.brain_eaten_times) {
						eat_brain_time_str += (it - leveldata.setlayout_time).enPrint() + ' ';
					}
					logger_layout->log(eat_brain_time_str, Logger::DEBUG);
					logger_cheat->log(eat_brain_time_str, Logger::DEBUG);


					// (4) 黄油率
					if (leveldata.butter_count == 0) {
						leveldata.kernelpult_butter_rate == 0.00f;
					}
					else {
						leveldata.kernelpult_butter_rate = (float)leveldata.butter_count / (leveldata.kernel_count + leveldata.butter_count);
					}
					std::string rate_str = "黄油率: " + std::to_string(leveldata.kernelpult_butter_rate) + "=" + std::to_string(leveldata.butter_count) + "/(" + std::to_string(leveldata.kernel_count) + "+" + std::to_string(leveldata.butter_count) + ")";
					logger_layout->log(rate_str, Logger::DEBUG);
					logger_cheat->log(rate_str, Logger::DEBUG);


					// ------------------------ 存档数据     -----------------------------------------------
					// (1) 存档上一关游戏数据
					if (has_started) save_data.push_back(leveldata);



					// ---------------------------------- 检测跨关是否异常 -------------------------------
					// (1) 检测是否跳关
					if (leveldata.brain_eaten_times.size() != 5) {
						logger_cheat->log("检测到跳关, 赛后裁定即可, 请继续游戏!", Logger::INFO);
						logger_cheat->log("检测到跳关! 实际吃脑数: " + std::to_string(leveldata.brain_eaten_times.size()), Logger::DEBUG);
					}

					// (2) 是否修改过阳光: 
					// a）每关都有最高上限阳光  
					if (game_controler.board->Sun > (leveldata.initial_sun + leveldata.flower_num * 200 - leveldata.zombie_cost)) {
						logger_cheat->log("检测到阳光异常, 赛后裁定即可, 请继续游戏!", Logger::INFO);
						logger_cheat->log("检测到阳光异常！ 现在阳光:" + std::to_string(game_controler.board->Sun)
							+ " ? "
							+ "上一关阳光: " + std::to_string(leveldata.initial_sun)
							+ " "
							+ "上一关最多阳光: " + std::to_string(leveldata.flower_num * 200)
							+ " "
							+ "上一关僵尸花费" + std::to_string(leveldata.zombie_cost)
							, Logger::DEBUG
						);
					} 
					// b）有精准上限阳光：看收集到多少阳光（但是存在bug：收集阳光监测不准
					else if (game_controler.board->Sun > (leveldata.initial_sun + leveldata.collected_sun - leveldata.zombie_cost)) {
						logger_cheat->log("检测到阳光异常, 赛后裁定即可, 请继续游戏!", Logger::INFO);
						logger_cheat->log("检测到阳光异常！ 现在阳光:" + std::to_string(game_controler.board->Sun)
							+ " ? "
							+ "上一关阳光: " + std::to_string(leveldata.initial_sun)
							+ " "
							+ "上一关点击的阳光: " + std::to_string(leveldata.collected_sun)
							+ " "
							+ "上一关僵尸花费" + std::to_string(leveldata.zombie_cost)
							, Logger::DEBUG
						);
					}

					// ------------------------------- 初始化下一关数据 ----------------------------------------
					// (1) 初始化下一关数据
					processed_zombie_ids.clear();
					leveldata.init();
					leveldata.setlayout_time = TimeStruct::getNow();
					leveldata.score = current_flag;
					leveldata.initial_sun = game_controler.board->Sun;
					leveldata.current_use_time = TimeStruct::getNow() - start_time;//每关已经使用的时间
					leveldata.last_brain_eaten_time = TimeStruct::getNow();
					leveldata.flower_num = ls_vec[current_flag][0] == '8'? ls_vec[current_flag][1] - '0'+5: ls_vec[current_flag][1] - '0';
				} while (0);

				// ----------------------- 2.4 监测脑子变化 --------------------------
				do {
					if (leveldata.eaten_brain_count == game_controler.countEatenBrain()
						|| game_controler.countEatenBrain() == 0 // 刚进入新的一关
						) continue;
					leveldata.score = game_controler.board->GetMiscellaneous()->Round + game_controler.countEatenBrain() * 0.2;
					leveldata.last_brain_eaten_time = TimeStruct::getNow();

					// 如果是同时吃脑子, 一次塞入多个: 当前5原来是3，代表同时吃脑子,则需要塞入5-3次
					while (leveldata.eaten_brain_count != game_controler.countEatenBrain()) {
						logger_cheat->log((leveldata.last_brain_eaten_time - start_time).enPrint() + " 吃了第" + std::to_string(game_controler.board->GetMiscellaneous()->Round) + "关的第" + std::to_string(leveldata.eaten_brain_count) + "个脑子", Logger::DEBUG);
						logger_layout->log((leveldata.last_brain_eaten_time - start_time).enPrint() + " 吃了第" + std::to_string(game_controler.board->GetMiscellaneous()->Round) + "关的第" + std::to_string(leveldata.eaten_brain_count) + "个脑子", Logger::DEBUG);
						
						leveldata.brain_eaten_times.push_back(leveldata.last_brain_eaten_time);
						leveldata.eaten_brain_count += 1;
					}

				} while (0);


				// ----------------------- 2.5 监测放置的僵尸、释放时间以及计算花费、反应时间 -----------------------
				do {
					std::vector<SPT<PVZ::Zombie>> zombies = game_controler.board->GetAllZombies();
					for (auto& zombie : zombies) {
						// 第一关的话多等一会儿游戏自动清除选卡界面的僵尸
						if (game_controler.board->GetMiscellaneous()->Round == 0)  std::this_thread::sleep_for(std::chrono::milliseconds(50)); //等布阵器输出完毕
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
							if (current_flag == 0) continue;
							logger_layout->log("检测到僵尸异常!", Logger::INFO);
							logger_cheat->log("检测到在第" + std::to_string(zombie->Row + 1) + "行放置了非ize关卡的僵尸,僵尸类型为" + ZombieType::ToString(zombie->Type), Logger::DEBUG);
							logger_cheat->log("检测到异常, 赛后仲裁, 请继续游戏...", Logger::DEBUG);
							continue;
						}
						// 如果是ize中的僵尸，但又不是伴舞僵尸，日志记录并且计算花费【每一关结束赋值】
						if (zombie->Type == ZombieType::BackupDancer) continue;

						auto zombie_info = game_controler.ZombieSunCost[zombie->Type];
						leveldata.zombie_cost += zombie_info.first;
						leveldata.released_zombies_count += 1;
						logger_cheat->log("在第" + std::to_string(zombie->Row + 1) + "行放置了" + std::string(zombie_info.second) + ",目前一共放了" + std::to_string(leveldata.released_zombies_count) + "个僵尸", Logger::DEBUG);
						// 记录解法信息
						leveldata.solve_info.push_back(std::make_tuple(zombie->Type, zombie->Row, TimeStruct::getNow()-leveldata.setlayout_time));
						// 记录反应时间
						if (leveldata.released_zombies_count == 1) {
							leveldata.first_zombie_release_time = TimeStruct::getNow();
							leveldata.reaction_time = leveldata.first_zombie_release_time - leveldata.setlayout_time;
							logger_cheat->log("第" + std::to_string(current_flag + 1) + "关，第一个僵尸释放时间: " + (leveldata.first_zombie_release_time - start_time).enPrint(), Logger::DEBUG);
							logger_cheat->log("第" + std::to_string(current_flag + 1) + "关，反应时间: " + leveldata.reaction_time.enPrint(), Logger::DEBUG);
						}
					}
				} while (0);

				
				// ----------------------- 2.6 监测子弹: 玉米和黄油 -----------------------------------
				do {
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
							logger_cheat->log("第" + std::to_string(projectile->Row + 1) + "行" + std::to_string(projectile->ImageX) + "坐标处出现了一个玉米粒", Logger::DEBUG);
						}
						else if (projectile->Type == ProjectileType::Butter) {
							leveldata.butter_count += 1;
							logger_cheat->log("第" + std::to_string(projectile->Row + 1) + "行" + std::to_string(projectile->ImageX) + "坐标处出现了一个黄油", Logger::DEBUG);
						}
					}
				} while (0);


				// ----------------------- 2.7. 统计收集的阳光 --------------------------------------
				if(!has_start_collect_sun_thread){ // 只开一个线程进行检测
					std::thread t(&GameCheatCheck::count_sun_thread, this);
					t.detach();
					has_start_collect_sun_thread = true;
				}
				do {
					{ // 检测阳光被收集
						std::lock_guard<std::mutex> lock(g_mutex);
						while (!g_data_queue.empty()) {
							int data = g_data_queue.front();
							g_data_queue.pop();
							if (data == 1)
							{
								leveldata.collected_sun += 25;
							}
						}
					}
				} while (0);



				// ---------------------- 2.8 间隔时间反作弊检查一次 ------------------------
				do{
					if ((TimeStruct::getNow() - check_time).second < this->check_interval)  continue;
					// 检测
					check_time = TimeStruct::getNow();
					if (check_all()) {
						logger_layout->log("检测到作弊!", Logger::INFO);
						logger_cheat->log("检测到作弊!", Logger::DEBUG);
					}
				} while (0);
			}
			catch (...) {
				continue;
			}
		}



		// 游戏结束的时候再记录一次
		std::this_thread::sleep_for(std::chrono::milliseconds(200)); //等布阵器输出完毕
		// (1) 过关提示
		std::ostringstream oss_tmp;
		oss_tmp << std::fixed << std::setprecision(1) << (std::round(leveldata.score * 10) / 10.0);
		std::string score_str = oss_tmp.str();
		std::string level_str = "已经使用: " + (TimeStruct::getNow() - start_time).enPrint()
			+ " 未通过第" + std::to_string(game_controler.board->GetMiscellaneous()->Round)
			+ "关, 阳光" + std::to_string(game_controler.board->Sun)
			+ ", 花费" + std::to_string(leveldata.zombie_cost)
			+ ", 收集阳光" + std::to_string(leveldata.collected_sun)
			+ ", 遗漏阳光" + std::to_string(leveldata.flower_num * 200 - leveldata.collected_sun)
			+ ", 目前得分" + score_str;

		logger_layout->log(level_str, Logger::DEBUG);
		logger_cheat->log(level_str, Logger::DEBUG);

		// (2) 打印每关的具体解法, 最后的数据为
		// 释放时间:std::get<2>(it)
		std::array<std::string, 5> solve_str_row = {}; std::string solve_str_total;
		std::array<int, 5> solve_cost_row = {}; std::string cost_str_total;
		// 按行来的
		for (int i = 0; i < 5; i++) {
			for (auto it : leveldata.solve_info) {
				if (std::get<1>(it) == i) {
					// 记录每条路花费< ZombieType::ZombieType, std::pair<int, std::string >>
					solve_cost_row[i] += game_controler.ZombieSunCost[std::get<0>(it)].first;
					solve_str_row[i] += game_controler.zombie_str[std::get<0>(it)];
				}
			}
			cost_str_total += std::to_string(solve_cost_row[i]) + "+";
			solve_str_total += "(" + std::to_string(i + 1) + ")" + solve_str_row[i] + ";";
		}
		cost_str_total = cost_str_total.substr(0, cost_str_total.size() - 1) + "=" + std::to_string(leveldata.zombie_cost);
		solve_str_total = "花费: " + cost_str_total + "\t解法: " + solve_str_total.substr(0, solve_str_total.size() - 1) + "\t耗时: " + (leveldata.last_brain_eaten_time - leveldata.setlayout_time).enPrint();
		logger_layout->log(solve_str_total, Logger::DEBUG);
		logger_cheat->log(solve_str_total, Logger::DEBUG);
		// 【TODO】按时间,如delay...


		// (3) 吃脑时间
		std::string eat_brain_time_str = "吃脑时间: ";
		for (auto it : leveldata.brain_eaten_times) {
			eat_brain_time_str += (it - leveldata.setlayout_time).enPrint() + ' ';
		}
		logger_layout->log(eat_brain_time_str, Logger::DEBUG);
		logger_cheat->log(eat_brain_time_str, Logger::DEBUG);


		// (4) 黄油率
		if (leveldata.butter_count == 0) {
			leveldata.kernelpult_butter_rate == 0.00f;
		}
		else {
			leveldata.kernelpult_butter_rate = (float)leveldata.butter_count / (leveldata.kernel_count + leveldata.butter_count);
		}
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(2) << (std::round(leveldata.kernelpult_butter_rate * 10) / 10.0);
		std::string rate_str = oss.str();
		rate_str = "黄油率: " + rate_str + "=" + std::to_string(leveldata.butter_count) + "/(" + std::to_string(leveldata.kernel_count) + "+" + std::to_string(leveldata.butter_count) + ")";
		logger_layout->log(rate_str, Logger::DEBUG);
		logger_cheat->log(rate_str, Logger::DEBUG);


		logger_layout->log("----------------反作弊检测已结束------------------", Logger::DEBUG);
		game_controler.modify_pvz_handle_title("Plants vs. Zombies"); // 禁掉一些寻找游戏是通过窗口名的：如算血器

	}
};

// 布阵器控制
class ConsoleControler {
private:
	// 启用冲关快捷键：加速 自收 强制退出 跳关 切换女仆
	void register_reviewMode_hotkey() {
		RegisterHotKey(NULL, 1, MOD_SHIFT, 'D'); // 切换加速
		RegisterHotKey(NULL, 2, MOD_SHIFT, 'A'); // 切换自动收集
		RegisterHotKey(NULL, 3, MOD_SHIFT, 'Q'); // 强制退出
		RegisterHotKey(NULL, 4, MOD_SHIFT, 'J'); // 跳关
		RegisterHotKey(NULL, 5, MOD_SHIFT, 'R'); // 重新布阵
	}
	// 销毁冲关快捷键
	void unregister_reviewMode_hotkey() {
		UnregisterHotKey(NULL, 1);
		UnregisterHotKey(NULL, 2);
		UnregisterHotKey(NULL, 3);
		UnregisterHotKey(NULL, 4);
		UnregisterHotKey(NULL, 5);
	}
	// 比赛模式：强制退出，重开
	void register_SpeedRaceMode_hotkey() {
		RegisterHotKey(NULL, 1, MOD_SHIFT, 'Q'); // 强制退出
		RegisterHotKey(NULL, 2, MOD_SHIFT, 'R'); // 重新布阵
		RegisterHotKey(NULL, 3, MOD_SHIFT, 'S'); // 恢复存档
		RegisterHotKey(NULL, 4, MOD_SHIFT, 'P'); // 暂停计时
	}
	void unregister_SpeedRaceMode_hotkey() {
		UnregisterHotKey(NULL, 1);
		UnregisterHotKey(NULL, 2);
		UnregisterHotKey(NULL, 3);
		UnregisterHotKey(NULL, 4);
	}

	// 禁掉快速编辑模式，但是坏消息是暂时没法复制文本内容
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

	// 获取当前 exe 所在的目录
	std::string GetExeDirectory() {
		char buffer[MAX_PATH];
		// 获取当前 exe 的完整路径
		GetModuleFileNameA(NULL, buffer, MAX_PATH);
		std::string fullPath(buffer);
		// 找到最后一个反斜杠的位置
		size_t pos = fullPath.find_last_of("\\/");
		return (pos != std::string::npos) ? fullPath.substr(0, pos) : fullPath;
	}

	// 查看线程是否关闭
	bool isThreadFinished(DWORD thread_id) {
		HANDLE hThread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, thread_id);
		if (hThread == NULL) {
			// 无法打开线程句柄，可能线程已结束
			return true;
		}

		DWORD exitCode;
		if (!GetExitCodeThread(hThread, &exitCode)) {
			CloseHandle(hThread);
			return false; // 获取状态失败
		}

		CloseHandle(hThread);
		return (exitCode != STILL_ACTIVE);
	}

	// 检测当前阵型是否合规，合规则转为布阵码
	bool export_layout_string(std::string& result) {
		// --------------------- 1.找到pvz ---------------------------
		GameControl game_controler;
		game_controler.find_pvz();
		if (!game_controler.board||!game_controler.is_in_ize()) {
			std::cout << "请先进入ize.." << std::endl;
			return false;
		}

		// ------------------ 2. 拿到主题 --------------------- 
		std::array<int, 25> positions = {};               // 对应位置

		// 1.判断当前主题
		std::unordered_map<PlantType::PlantType, int> plantCount;
		for (auto plant : game_controler.board->GetAllPlants()) {
			if (plant->NotExist) continue;
			plantCount[plant->Type]++;
		}

		int theme_index = 0;
		// 1.1 获取主题
			{
			if (
				plantCount[PlantType::SnowPea] == 9 && plantCount[PlantType::Repeater] == 4 && plantCount[PlantType::SplitPea] == 4
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
		std::cout << "主题合规, 主题序号为:" << theme_index << std::endl;

		// 胆小特殊处理
		int flower_num = theme_index == 8 ? plantCount[PlantType::Sunflower] - 5 : plantCount[PlantType::Sunflower];


		// ----------------------------- 2. 依次获取植物种植位置 -------------------------
		std::array<PlantType::PlantType, 25> plant_types =
			GenerateLayoutCode::get_theme_plants(flower_num, static_cast<Theme>(theme_index));

		// 使用vector存储植物类型及其位置
		std::vector<std::pair<PlantType::PlantType, std::vector<int>>> plants_position;

		// 收集植物位置
		for (auto plant : game_controler.board->GetAllPlants()) {
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
			if (!found && theme_index != 8) {
				std::cerr << "错误：类型 " << PlantType::ToString(required_type)
					<< " 在位置 " << i << " 没有可用植物" << std::endl;
				positions[order++] = -1; // 用-1标记错误
			}
		}
		if (game_controler.board->Sun > 99*25 || game_controler.board->Sun % 25 != 0) {
			game_controler.board->Sun = 2000;
			std::cout << "阳光存在一定问题！已调整到2000!" << std::endl;
		}
		int sun = game_controler.board->Sun / 25;
		std::ostringstream oss;
		oss << std::setw(2) << std::setfill('0') << sun;
		std::string sun_str = oss.str();

		// ---------------------- 3. 返回布阵码 -----------------------
		result = std::to_string(theme_index)
			+ std::to_string(flower_num)
			+ sun_str
			+ GenerateLayoutCode::encrypt_to_base25(positions);

		return true; // 返回成功
	}

	// 30min限时循环
	void SpeedRun30min(std::string ls, bool is_cheat_check=false) {
		// ------------ 1. 验证布阵码合法性  -----------------
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
				return;
			}
			if (flower_num < 1 || flower_num > 8) {
				std::cout << "输入不合法" << std::endl;
				return;
			}
			if (sun != 0) { // 常规模式不可设置阳光
				std::cout << "输入不合法" << std::endl;
				return;
			}
		};

		// ------------ 2. 寻找游戏 ----------------------
		GameControl game_controler; 
		while (!game_controler.find_pvz()) { Sleep(1); };
		std::cout << "已找到pvz!" << std::endl;
		while (!game_controler.is_in_ize()) { Sleep(1); };
		std::cout << "已进入ize!" << std::endl;


		// -------------- 3. 记录日志：布阵器---------------------------
		std::string current_time = TimeStruct::getCurrentDateTime();
		Logger* logger_layout = new Logger(current_time + ".log", Logger::DEBUG); // 默认打印INFO, 但是记录的话全部记录

		// --------------------- 4. 初始化数据 --------------------
		auto current_address = game_controler.board->GetBaseAddress(); // 记录board地址

		// 关卡数据
		int current_flag = -1;
		std::vector<LevelData> save_data;
		LevelData leveldata;
		int lowestSun = 50;

		// 时间类
		TimeStruct start_time = TimeStruct::getNow();
		// 布尔标志位
		bool has_started = false; // 还没开始游戏
		bool is_crashed = false; // 标记游戏是否崩溃
		bool is_timePaused = false; TimeStruct pause_time = TimeStruct::getNow();
		stop_flag = false; pause_flag = false;


		// ---------------------- 5. 游戏前准备：停掉游戏种植, 禁用女仆 -----------------
		game_controler.setInjectors();
		game_controler.disable_maidCheat();

		game_controler.board->Sun = 150;
		leveldata.initial_sun = game_controler.board->Sun;

		game_controler.board->GetMiscellaneous()->Round = 0;
		game_controler.clear_reverse_all_plants();
		game_controler.clear_not_colleted_sun();
		game_controler.clear_all_zombies();
		game_controler.clear_all_bullets();
		game_controler.update_brains();
		game_controler.board->GamePaused = false;
		//game_controler.board->Lose(); 

		logger_layout->log(std::string(get_terminal_width(), '-'), Logger::INFO);
		logger_layout->log("已经进入ize, 现在开始布阵", Logger::DEBUG);
		// 提示主题
		std::vector<std::string> parts;
		std::istringstream iss(ls);
		std::string token;
		while (getline(iss, token, '.')) {
			parts.push_back(token);
		}
		logger_layout->log("25个主题序号为：", Logger::INFO);
		int count = 0;
		std::ostringstream oss;
		for (const auto& part : parts) {
			count++;
			oss << part[0] << " ";
			if (count % 10 == 0) {
				oss << std::endl;
			}
		}
		logger_layout->log(oss.str(), Logger::INFO);



		// -----------------------------------    反作弊日志：在外部作用域声明指针和引用别名
		Logger* logger_cheat = nullptr;  // 原始指针
		// 先检测环境是否异常
		GameCheatCheck game_cheat_checker(2, logger_layout, logger_cheat); // 每2s检测一次
		TimeStruct check_time = TimeStruct::getNow();
		// 反作弊日志
		if (is_cheat_check) {
			// 拿到新指针
			logger_cheat = new Logger(
				current_time + "_cheatCheck.log",
				Logger::DEBUG,
				true
			);
			game_cheat_checker.logger_cheat = logger_cheat; // 重新赋值一下，拿到正确指针
			game_cheat_checker.check_envirnoment();
			// 正确的方式：传递成员函数指针和对象地址
			std::thread t(&GameCheatCheck::cheat_check_thread, &game_cheat_checker, vec);
			t.detach();
		}
		else { // 还原回标题
			game_controler.modify_pvz_handle_title("Plants vs. Zombies");
		}

		// ------------------- 6.开始游戏主循环 ---------------*
		MSG msg = { 0 };
		while (true) {
			// ---------------------- 6.1 监测快捷键 -------------------------
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_HOTKEY) {
					// shift+q 强制结束
					if (msg.wParam == 1) {
						std::ostringstream oss;
						oss << std::fixed << std::setprecision(1) << (std::round(leveldata.score * 10) / 10.0);
						std::string score_str = oss.str();
						logger_layout->log(std::string(get_terminal_width(), '-'), Logger::INFO);
						auto timeStr = leveldata.score == 0.00 && leveldata.eaten_brain_count == 0 ? "00:00" : (leveldata.last_brain_eaten_time - start_time).enPrint();
						logger_layout->log(TimeStruct::getCurrentTime() + " " + timeStr + " 提前结束游戏!", Logger::INFO);
						logger_layout->log("最后吃脑时间为: " + timeStr + " 最终得分为——  " + score_str, Logger::INFO);

						auto caption_str = timeStr.append("     ") + score_str ;
						Creator::CreateCaption(caption_str.c_str() + '\0', caption_str.size(), CaptionStyle::Lowermiddle);
						goto gameover;
					}
					// shift+R重新对当前关卡布阵
					else if (msg.wParam == 2) {
						// 清除本关
						game_controler.clear_not_colleted_sun();
						game_controler.update_brains();
						game_controler.clear_all_zombies();
						game_controler.clear_all_bullets();
						
						// 恢复阳光
						game_controler.board->Sun = current_flag == 0 ? 150 : leveldata.initial_sun;
						current_flag = game_controler.board->GetMiscellaneous()->Round;
						if (current_flag == 0 ) has_started = false;

						std::string one_layoutString = vec[current_flag];
						int theme_index = 0; int flower_num = 0; int sun = 0; std::array<int, 25> orders = {};
						if (!GenerateLayoutCode::decode_layout_string(one_layoutString, theme_index, flower_num, sun, orders)) {
							logger_layout->log("当前关布阵码不合法", Logger::INFO);
							return;
						}
						game_controler.set_layout_order(theme_index, flower_num, 0, orders);
						leveldata.setlayout_time = TimeStruct::getNow();
						logger_layout->log((TimeStruct::getNow() - start_time).enPrint() + " 当前对第" + std::to_string(current_flag) + "关进行布阵", Logger::DEBUG);

						TimeStruct restart_time = TimeStruct::getNow() - start_time;
						logger_layout->log(restart_time.enPrint() + " 对第" + std::to_string(current_flag) + "关重新布阵!", Logger::INFO);

						// 初始化下一关数据
						leveldata.init();
						leveldata.initial_sun = game_controler.board->Sun;
						leveldata.current_use_time = TimeStruct::getNow() - start_time;//每关已经使用的时间

						std::string caption_str = leveldata.current_use_time.enPrint() + std::string(" ReLayout");
						Creator::CreateCaption(caption_str.c_str() + '\0', caption_str.size(), CaptionStyle::Lowermiddle);
					}
					// shift+s崩溃
					else if (msg.wParam == 3) {
						if (!is_crashed) continue; // 没崩溃按了没反应
						// 重找pvz
						game_controler.refind_pvz();
						while (!game_controler.is_in_ize() || !game_controler.board) { Sleep(1); };
						current_address = game_controler.board->GetBaseAddress();

						pause_flag = false;
						// 禁掉女仆和游戏种植
						game_controler.clear_reverse_all_plants(); game_controler.update_brains();
						game_controler.disable_maidCheat();
						game_controler.setInjectors();
						// 重新布阵
						game_controler.board->GetMiscellaneous()->Round = current_flag; // 跳回关卡
						game_controler.board->Sun = leveldata.initial_sun;
						std::string one_layoutString = vec[current_flag];
						int theme_index = 0; int flower_num = 0; int sun = 0; std::array<int, 25> orders = {};
						if (!GenerateLayoutCode::decode_layout_string(one_layoutString, theme_index, flower_num, sun, orders)) {
							logger_layout->log("当前关布阵码不合法", Logger::INFO);
							return;
						}
						game_controler.set_layout_order(theme_index, flower_num, 0, orders);
						leveldata.setlayout_time = TimeStruct::getNow();

						// 初始化本关数据
						auto time_tmp = leveldata.current_use_time;
						leveldata.init();
						leveldata.initial_sun = game_controler.board->Sun;
						leveldata.current_use_time = time_tmp;

						// 计算还拥有的时间: 使用的时间, 第一个僵尸释放的时间也就是start_time，上一次存档的时间
						start_time = TimeStruct::getNow() - leveldata.current_use_time;

						logger_layout->log(TimeStruct::getCurrentTime() + " 现已恢复存档, 请继续游戏...", Logger::INFO);
						logger_layout->log(std::string(get_terminal_width(), '*'), Logger::INFO);
						is_crashed = false;
					}
					// shift+p暂停与重新暂停
					else if (msg.wParam == 4) {
						is_timePaused = !is_timePaused;
						if (is_timePaused) // 暂停计时
						{
							// 暂停游戏
							game_controler.board->GamePaused = true;
							pause_flag = game_controler.board->GamePaused;
							PostMessage(PVZ::Memory::mainwindowhandle, WM_KEYDOWN, VK_SPACE, 0); // 发空格指令
							PostMessage(PVZ::Memory::mainwindowhandle, WM_KEYUP, VK_SPACE, 0); // 发空格指令
						
							// 布阵器和游戏报时
							pause_time = TimeStruct::getNow();
							logger_layout->log((pause_time - start_time).enPrint() + " 暂停计时! 按shift+p恢复计时", Logger::INFO);
							auto time_str = (pause_time - start_time).enPrint() + " Pause";
							Creator::CreateCaption(time_str.c_str() + '\0', time_str.size(), CaptionStyle::Lowermiddle);
						}
						else
						{
							game_controler.board->GamePaused = false;
							pause_flag = game_controler.board->GamePaused;
							PostMessage(PVZ::Memory::mainwindowhandle, WM_KEYDOWN, VK_SPACE, 0); // 发空格指令
							PostMessage(PVZ::Memory::mainwindowhandle, WM_KEYUP, VK_SPACE, 0); // 发空格指令

							logger_layout->log((pause_time - start_time).enPrint() + " 已继续计时! ", Logger::INFO);
							auto time_str = (pause_time - start_time).enPrint() + " Continue";
							Creator::CreateCaption(time_str.c_str() + '\0', time_str.size(), CaptionStyle::Lowermiddle);

							start_time = TimeStruct::getNow() + start_time - pause_time;
							leveldata.last_brain_eaten_time = TimeStruct::getNow() + leveldata.last_brain_eaten_time - pause_time;
						}
						continue;
					}
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			if (is_crashed || is_timePaused) continue;
			// ---------------------- 6.2 检测重开 ---------------------------
			do { // board不正确才是重开
				if (!game_controler.pvz // 如果pvz都找不到了就不算重开了
					|| PVZ::Memory::ReadPointer(0x6a9ec0, 0x768) == 0 // 如果完全没有board就是不在ize界面了
					|| current_address == PVZ::Memory::ReadPointer(0x6a9ec0, 0x768) // 如果当前的board地址和记录的地址相同就代表没啥问题
					|| game_controler.pvz->GameState != PVZGameState::Playing // 如果现在没在进行游戏，那就不用管了
					) continue;
				// 确实重开了
				while (current_address != PVZ::Memory::ReadPointer(0x6a9ec0, 0x768)) {
					std::this_thread::sleep_for(std::chrono::milliseconds(100));// 等一下pvzclass找到新board
					game_controler.board = PVZ::GetBoard();// 重新拿一下board;
					current_address = game_controler.board->GetBaseAddress();
				}
				logger_layout->log((TimeStruct::getNow() - start_time).enPrint() + " 游戏重开!", Logger::INFO);
				if (current_flag == 0) has_started = false;
				current_flag = -1;// 跳到跨关那里重新布阵
			} while (0);


			// ---------------------- 6.3 检测跨关 ---------------------------
			do {
				if (!game_controler.pvz // 游戏崩溃了就别进跨关
					|| !game_controler.board
					|| is_crashed
					|| current_address != game_controler.board->GetBaseAddress()
					|| game_controler.board->GetMiscellaneous()->Round == current_flag
					|| game_controler.pvz->GameState != PVZGameState::Playing) continue;

				// -------------- 6.3.1 通关了 --------------
				if (game_controler.board->GetMiscellaneous()->Round >= 25) {
					// 布阵器报通关
					logger_layout->log(std::string(get_terminal_width(), '-'), Logger::INFO);
					logger_layout->log(TimeStruct::getCurrentTime() + " 恭喜打通!!!!", Logger::INFO);
					logger_layout->log("最后吃脑时间为: " + (leveldata.last_brain_eaten_time - start_time).cnPrint(), Logger::INFO);
					// 游戏报字幕
					auto timeStr = (leveldata.last_brain_eaten_time - start_time).enPrint().append("     ").append(std::string("Congrats!"));
					Creator::CreateCaption(timeStr.c_str() + '\0', timeStr.size(), CaptionStyle::Lowermiddle);
					return;
				}

				// ------------- 6.3.2 正常跨关 --------------
				// (1) 存档上一关数据
				if (has_started) save_data.push_back(leveldata);
				// (2) 先布阵
				current_flag = game_controler.board->GetMiscellaneous()->Round;
				std::string one_layoutString = vec[current_flag];
				int theme_index = 0; int flower_num = 0; int sun = 0; std::array<int, 25> orders = {};
				if (!GenerateLayoutCode::decode_layout_string(one_layoutString, theme_index, flower_num, sun, orders)) {
					logger_layout->log("当前关布阵码不合法", Logger::INFO);
					return;
				}
				game_controler.set_layout_order(theme_index, flower_num, 0, orders);
				leveldata.setlayout_time = TimeStruct::getNow();
				logger_layout->log("当前对第" + std::to_string(current_flag) + "关进行布阵", Logger::DEBUG);
				

				// (3) 初始化下一关数据
				leveldata.init();
				leveldata.score = current_flag;
				leveldata.initial_sun = game_controler.board->Sun;
				leveldata.current_use_time = TimeStruct::getNow() - start_time;//每关已经使用的时间
				leveldata.last_brain_eaten_time = TimeStruct::getNow();

				// (4) 布阵器打印上一关数据
				if (!has_started) continue; // 第一关还没过就先不打印
				logger_layout->log((TimeStruct::getNow() - start_time).enPrint()
					+ " 已经通过" + std::to_string(game_controler.board->GetMiscellaneous()->Round)
					+ "关, 阳光" + std::to_string(game_controler.board->Sun)
					,Logger::INFO);

			} while (0);


			// -------------------- 4.4 检测开始游戏 --------------------------
			if (!has_started) {
				// 跳过已经开始游戏的
				if (!game_controler.board || // 找不到board
					game_controler.pvz->LevelId != PVZLevel::I_Zombie_Endless ||
					game_controler.pvz->GameState != PVZGameState::Playing)
					continue;
				// 判断游戏开始条件:放了一个僵尸
				if (game_controler.board->ZombiesCount != 1) continue;

				// 开始游戏了
				has_started = true;
				start_time = TimeStruct::getNow();
				leveldata.current_use_time = TimeStruct(0);
				// 第一个僵尸释放时间，记录第一关的反应时间，僵尸释放时间-布阵时间
				leveldata.first_zombie_release_time = start_time;
				leveldata.reaction_time = leveldata.first_zombie_release_time - leveldata.setlayout_time;
				logger_layout->log(start_time.getCurrentTime() + " 开始游戏!", Logger::INFO);
				logger_layout->log(std::string(get_terminal_width(), '-'), Logger::INFO);
			}


			// -------------------- 4.5 监测崩溃: 游戏是否被关闭 --------------------
			do {
				if (ProcessOpener::Open() || is_crashed) continue; // 游戏崩溃了
				game_controler.board = nullptr;
				game_controler.pvz = nullptr;
				is_crashed = true;
				pause_flag = true;
				// 崩溃时打印一次
				logger_layout->log(std::string(get_terminal_width(), '*'), Logger::INFO);
				logger_layout->log(TimeStruct::getCurrentTime() + " 游戏异常关闭!", Logger::INFO);
				//std::cout << leveldata.score << std::endl;
				logger_layout->log(
					"存档数据为: 关数: " + std::to_string(current_flag) + ", 耗时: " + leveldata.current_use_time.enPrint() + ", 阳光: " + std::to_string(leveldata.initial_sun)
					, Logger::INFO
				);
				logger_layout->log("请进入pvz并进入ize后，按shift+s可继续游戏", Logger::INFO);
			} while (0);


			// -------------------- 4.6 监测脑子变化 --------------------------
			do {
				if (leveldata.eaten_brain_count == game_controler.countEatenBrain()
					|| game_controler.countEatenBrain() == 0 // 刚进入新的一关
					) continue;

				leveldata.score = game_controler.board->GetMiscellaneous()->Round + game_controler.countEatenBrain() * 0.2;
				leveldata.last_brain_eaten_time = TimeStruct::getNow();
				leveldata.brain_eaten_times.push_back(leveldata.last_brain_eaten_time);
				leveldata.eaten_brain_count = game_controler.countEatenBrain();
			} while (0);


			// --------------------- 4.7 超时 -------------------------------
			do {
				if ((TimeStruct::getNow() - start_time).minute < 30) continue;
				// 把得分格式化1位小数
				std::ostringstream oss;
				oss << std::fixed << std::setprecision(1) << (std::round(leveldata.score * 10) / 10.0);
				std::string score_str = oss.str();
				// 布阵器提示
				logger_layout->log(std::string(get_terminal_width(), '-'), Logger::INFO);
				auto timeStr = (leveldata.last_brain_eaten_time - start_time).enPrint(); 
				logger_layout->log(TimeStruct::getCurrentTime() + " 超时, 游戏结束!", Logger::INFO);
				logger_layout->log("最后吃脑时间为: " + timeStr + " 最终得分为——  " + score_str, Logger::INFO);

				auto caption_str = timeStr.append("     ") + score_str;
				Creator::CreateCaption(caption_str.c_str() + '\0', caption_str.size(), CaptionStyle::Lowermiddle);
				goto gameover;
			} while (0);


			// --------------------- 4.8 有时间但是没阳光了 ------------------
			do{
				if (!game_controler.board || !game_controler.pvz || !game_controler.board->GetBaseAddress() || !ProcessOpener::Open()
					|| game_controler.board->Sun >= lowestSun
					|| game_controler.board->ZombiesCount != 0) continue;

				bool is_dead = true;
				// 如果还找不到掉落的阳光
				for (auto& coin : game_controler.board->GetAllCoins()) {
					if (coin->Type == CoinType::NormalSun) is_dead = false;
				}

				std::this_thread::sleep_for(std::chrono::seconds(2));

				if (is_dead && ProcessOpener::Open()) {
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << (std::round(leveldata.score * 10) / 10.0);
					std::string score_str = oss.str();

					logger_layout->log(std::string(get_terminal_width(), '-'), Logger::INFO);
					auto timeStr = (leveldata.last_brain_eaten_time - start_time).enPrint();
					logger_layout->log(TimeStruct::getCurrentTime() + " 阳光用完, 游戏结束!", Logger::INFO);
					logger_layout->log("最后吃脑时间为: " + timeStr + " 最终得分为——  " + score_str, Logger::INFO);

					auto caption_str = timeStr.append("     ") + score_str;
					Creator::CreateCaption(caption_str.c_str() +'\0', caption_str.size(), CaptionStyle::Lowermiddle); // 去掉浮点数的两位小数
					goto gameover;
				}
			} while (0);
			

		}

		gameover: {
			game_controler.enable_maidCheat();
			logger_layout->log("本次IZE竞速结束，日志文件名为: " + current_time , Logger::INFO);
			stop_flag = true; pause_flag = true;
			if (is_cheat_check) {
				// 检测线程结束
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				logger_layout->log("反作弊检测线程(" +std::to_string(game_cheat_checker.cheat_check_thread_id) + ", status:" + std::string(isThreadFinished(game_cheat_checker.cheat_check_thread_id) ? "exit" : "running") 
					+ ") 阳光统计线程(" + std::to_string(game_cheat_checker.count_sun_thread_id) + ", status:" + std::string(isThreadFinished(game_cheat_checker.count_sun_thread_id) ? "exit" : "running") + ")", Logger::DEBUG);
				// 结束日志文件并且计算hash值
				std::string log_file_path = logger_cheat->logFilePath;
				logger_cheat->~Logger();  std::string hash = Logger::calc_hash(log_file_path);
				logger_layout->log("加密日志文件为: " + current_time + "_cheatCheck.log，若文件被修改将被追责!", Logger::INFO); logger_layout->~Logger();
			}
			else{
				logger_layout->~Logger();
			}
			return;
		}
	};

	// 冲关
	void LevelRush(bool is_ban_maidCheat) {
		// ------------ 1. 寻找游戏 ----------------------
		GameControl game_controler;
		while (!game_controler.find_pvz()) { Sleep(1); };
		std::cout << "已找到pvz!" << std::endl;
		while (!game_controler.is_in_ize()) { Sleep(1); };
		std::cout << "已进入ize!" << std::endl;
		game_controler.modify_pvz_handle_title("Plants vs. Zombies");

		// -------------- 2. 记录日志：布阵器 ---------------------------
		std::string current_time = TimeStruct::getCurrentDateTime();
		Logger* logger_layout = new Logger(current_time + ".log", Logger::DEBUG); // 默认打印INFO, 但是记录的话全部记录

		// -------------  3. 初始化数据 --------------------
		GenerateLayoutCode code_generator;
		auto current_address = game_controler.board->GetBaseAddress(); // 记录board地址

		// 关卡数据
		int current_flag = -1;
		std::vector<LevelData> save_data;
		std::vector<std::string> all_layout_code;
		LevelData leveldata;
		int lowestSun = 50;
		std::unordered_set<int> processed_zombie_ids;

		// 时间类
		TimeStruct start_time = TimeStruct::getNow();
		// 布尔标志位
		bool is_speed_up = false; // 是否开启了加速
		bool is_auto = false; // 是否开启了自动收集
		bool has_started = false;


		game_controler.auto_collect(false); // 关掉自动收集
		game_controler.reset_speed(); // 恢复原速

		// 记录胆小数量
		int scardy_theme_count = 0;


		// ------------- 4. 游戏前准备 -----------------------
		game_controler.setInjectors(); // 停掉游戏种植
		if (is_ban_maidCheat) {
			game_controler.disable_maidCheat();
			logger_layout->log("当前女仆状态: 禁用!", Logger::INFO);
		}
		else {
			game_controler.enable_maidCheat();
			logger_layout->log("当前女仆状态: 不禁用!", Logger::INFO);
		}

		game_controler.board->GetMiscellaneous()->Round = 0;
		game_controler.board->Sun = 150;
		leveldata.initial_sun = game_controler.board->Sun;

		game_controler.clear_reverse_all_plants();
		game_controler.clear_not_colleted_sun();
		game_controler.clear_all_zombies();
		game_controler.clear_all_bullets();
		game_controler.update_brains();
		game_controler.board->GamePaused = false;

		logger_layout->log(std::string(get_terminal_width(), '-'), Logger::INFO);
		logger_layout->log("已经进入ize, 现在开始布阵", Logger::DEBUG);


		// ------------------- 5.开始游戏主循环 ---------------
		MSG msg = { 0 };
		while (true) {
			// ---------------------- 5.1 监测快捷键 -------------------------
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_HOTKEY) {
					// shift+D 切换加速
					if (msg.wParam == 1) {  // shift+D 切换加速与原速
						is_speed_up = !is_speed_up;
						if (is_speed_up) {
							game_controler.set_speed_10x();
							logger_layout->log(" 当前加速倍率: 10x", Logger::DEBUG);
						}
						else {
							game_controler.reset_speed();
							logger_layout->log(" 关闭加速", Logger::DEBUG);
						};
					}
					// shift+A 切换自动收集
					else if (msg.wParam == 2) { // shift+A 切换自动收集
						is_auto = !is_auto;
						game_controler.auto_collect(is_auto);
						logger_layout->log(std::string(is_auto ? "打开" : "关闭") + "自动收集", Logger::DEBUG);
					}
					// shift+q 退出
					else if (msg.wParam == 3) {
						//得分取一位小数
						std::ostringstream oss;
						oss << std::fixed << std::setprecision(1) << (std::round(leveldata.score * 10) / 10.0);
						std::string score_str = oss.str();

						logger_layout->log(std::string(get_terminal_width(), '-'), Logger::INFO);
						logger_layout->log(start_time.getCurrentTime() + " 提前结束游戏!", Logger::INFO);
						logger_layout->log(
							"最终得分: " + std::string(is_ban_maidCheat ? "无" : "有") + "女仆 " + std::to_string(scardy_theme_count) + "-" + score_str
							, Logger::INFO
						);
						auto game_over_str = std::string("GameOver..").append("     ").append(std::to_string(scardy_theme_count) + "-") + score_str;
						Creator::CreateCaption(game_over_str.c_str() + '\0', game_over_str.size(), CaptionStyle::Lowermiddle);
						goto gameover;
					}
					// shift+j 跳关
					else if (msg.wParam == 4) {
						game_controler.board->Win();
						logger_layout->log("跳过第" + std::to_string(game_controler.board->GetMiscellaneous()->Round) + "关, 当前脑子数: " + std::to_string(game_controler.countEatenBrain()) + "!", Logger::DEBUG);
					}
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}


			// ---------------------- 5.2 检测重开 ---------------------------
			do { // board不正确才是重开
				if (!game_controler.pvz // 如果pvz都找不到了就不算重开了
					|| PVZ::Memory::ReadPointer(0x6a9ec0, 0x768) == 0 // 如果完全没有board就是不在ize界面了
					|| current_address == PVZ::Memory::ReadPointer(0x6a9ec0, 0x768) // 如果当前的board地址和记录的地址相同就代表没啥问题
					|| game_controler.pvz->GameState != PVZGameState::Playing // 如果现在没在进行游戏，那就不用管了
					) continue;
				// 确实重开了
				while (current_address != PVZ::Memory::ReadPointer(0x6a9ec0, 0x768)) {
					std::this_thread::sleep_for(std::chrono::milliseconds(100));// 等一下pvzclass找到新board
					game_controler.board = PVZ::GetBoard();// 重新拿一下board;
					current_address = game_controler.board->GetBaseAddress();
				}
				logger_layout->log((TimeStruct::getNow() - start_time).enPrint() + " 游戏重开!", Logger::INFO);
				if (current_flag == 0) {
					all_layout_code.clear();
					has_started = false;
				}
				current_flag = -1;// 跳到跨关那里重新布阵
			} while (0);


			// ---------------------- 5.3 检测跨关 ---------------------------
			do {
				if (!game_controler.pvz // 游戏崩溃了就别进跨关
					|| !game_controler.board
					|| current_address != game_controler.board->GetBaseAddress()
					|| game_controler.board->GetMiscellaneous()->Round == current_flag
					|| game_controler.pvz->GameState != PVZGameState::Playing) continue;

				// ------------- 正常跨关 --------------
				// (1) 存档上一关数据
				if (has_started) save_data.push_back(leveldata);
				// (2) 先布阵
				current_flag = game_controler.board->GetMiscellaneous()->Round;
				auto ls = code_generator.generate_LevelRush_code(current_flag); all_layout_code.push_back(ls);
				int theme_index = 0; int flower_num = 0; int sun = 0;
				std::array<int, 25> orders = {};
				// 检测布阵码是否合法
				if (!GenerateLayoutCode::decode_layout_string(ls, theme_index, flower_num, sun, orders)) {
					std::cout << "布阵码不合法" << std::endl;
					return;
				}
				game_controler.set_layout_order(theme_index, flower_num, 0, orders);
				leveldata.setlayout_time = TimeStruct::getNow();
				logger_layout->log("当前对第" + std::to_string(current_flag) + "关进行布阵", Logger::DEBUG);
				
				// (3) 初始化下一关数据
				leveldata.init();
				leveldata.score = current_flag;
				leveldata.initial_sun = game_controler.board->Sun;
				game_controler.reset_speed(); is_speed_up = false;
				processed_zombie_ids.clear();

				// (4) 布阵器打印上一关数据
				if (!has_started) continue;
				if (!all_layout_code.empty() && all_layout_code.back()[0] == '8')
				{
					logger_layout->log("通关了一次胆小, 目前胆小次数为: " + std::to_string(scardy_theme_count), Logger::DEBUG);
					scardy_theme_count += 1;
				}
				logger_layout->log((TimeStruct::getNow() - start_time).enPrint()
					+ " 已经通过" + std::to_string(game_controler.board->GetMiscellaneous()->Round)
					+ "关, 阳光" + std::to_string(game_controler.board->Sun)
					+ "，花费" + std::to_string(leveldata.zombie_cost)
					, Logger::INFO);
				
			} while (0);

			// ---------------------- 5.4 检测开始游戏 --------------------------
			if (!has_started) {
				// 跳过已经开始游戏的
				if (!game_controler.board || // 找不到board
					game_controler.pvz->LevelId != PVZLevel::I_Zombie_Endless ||
					game_controler.pvz->GameState != PVZGameState::Playing)
					continue;
				// 判断游戏开始条件:放了一个僵尸
				if (game_controler.board->ZombiesCount != 1) continue;

				// 开始游戏了
				has_started = true;
				start_time = TimeStruct::getNow();
				// 第一个僵尸释放时间，记录第一关的反应时间，僵尸释放时间-布阵时间
				leveldata.first_zombie_release_time = start_time;
				logger_layout->log(start_time.getCurrentTime() + "开始游戏!", Logger::INFO);
				logger_layout->log(std::string(get_terminal_width(), '-'), Logger::INFO);
			}

			// ---------------------- 5.5 监测释放的僵尸花费 --------------------------
			do{
				if (!game_controler.board) continue;

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
				}
			} while (0);
			

			// ---------------------- 5.5 监测脑子变化 --------------------------
			do {
				if (leveldata.eaten_brain_count == game_controler.countEatenBrain()
					|| game_controler.countEatenBrain() == 0 // 刚进入新的一关
					) continue;

				leveldata.score = game_controler.board->GetMiscellaneous()->Round + game_controler.countEatenBrain() * 0.2;
				leveldata.last_brain_eaten_time = TimeStruct::getNow();
				leveldata.brain_eaten_times.push_back(leveldata.last_brain_eaten_time);
				leveldata.eaten_brain_count = game_controler.countEatenBrain();

				logger_layout->log((leveldata.last_brain_eaten_time - start_time).enPrint() + "吃了第" + std::to_string(game_controler.countEatenBrain()) + "个脑子", Logger::DEBUG);
			} while (0);


			// --------------------- 4.6 有时间但是没阳光了 ------------------
			do {
				if (!game_controler.board || !game_controler.pvz || !game_controler.board->GetBaseAddress() || !ProcessOpener::Open()
					|| game_controler.board->Sun >= lowestSun
					|| game_controler.board->ZombiesCount != 0) continue;

				bool is_dead = true;
				// 如果还找不到掉落的阳光
				for (auto& coin : game_controler.board->GetAllCoins()) {
					if (coin->Type == CoinType::NormalSun) is_dead = false;
				}

				std::this_thread::sleep_for(std::chrono::seconds(1));

				if (is_dead && ProcessOpener::Open()) {
					std::ostringstream oss;
					oss << std::fixed << std::setprecision(1) << (std::round(leveldata.score * 10) / 10.0);
					std::string score_str = oss.str();

					logger_layout->log(std::string(get_terminal_width(), '-'), Logger::INFO);
					logger_layout->log(start_time.getCurrentTime() + " 阳光用完, 游戏结束!", Logger::INFO);
					logger_layout->log(
						"最终得分: " + std::string(is_ban_maidCheat ? "无" : "有") + "女仆 " + std::to_string(scardy_theme_count) + "-" + score_str
						, Logger::INFO
					);
					auto game_over_str = std::string("GameOver..").append("     ").append(std::to_string(scardy_theme_count) + "-") + score_str;
					Creator::CreateCaption(game_over_str.c_str() + '\0', game_over_str.size(), CaptionStyle::Lowermiddle);
					goto gameover;
				}
			} while (0);
		};
		// goto退出总循环
		gameover:
		{
			// 关卡结束后打印本次冲关数据
			std::string layout_codes;
			for (const auto& ls : all_layout_code) {
				layout_codes += ls + ".";
			}
			layout_codes = layout_codes.substr(0, layout_codes.size() - 1); // 去掉最后的'.'
			logger_layout->log("本次冲关关卡布阵码为(已复制到剪切板): \n" + layout_codes, Logger::INFO);
			copyToClipBoard(layout_codes);

			// 把加速和自动收集关了
			game_controler.auto_collect(false);
			game_controler.reset_speed();
			if (is_ban_maidCheat) {
				game_controler.enable_maidCheat();
			}
			return;
		}
	}

	// 残局
	void IncompleteLevel() {
		// ------------ 1. 寻找游戏 ----------------------
		GameControl game_controler;
		while (!game_controler.find_pvz()) { Sleep(1); };
		std::cout << "已找到pvz!" << std::endl;
		while (!game_controler.is_in_ize()) { Sleep(1); };
		std::cout << "已进入ize!" << std::endl;
		game_controler.modify_pvz_handle_title("Plants vs. Zombies");

		// -------------- 2. 记录日志：布阵器 ---------------------------
		std::string current_time = TimeStruct::getCurrentDateTime();
		Logger* logger_layout = new Logger(current_time + ".log", Logger::DEBUG); // 默认打印INFO, 但是记录的话全部记录

		// -------------  3. 初始化数据 --------------------
		GenerateLayoutCode code_generator;
		auto current_address = game_controler.board->GetBaseAddress(); // 记录board地址

		// 关卡数据
		int current_flag = -1;
		std::vector<LevelData> save_data;
		std::vector<std::string> all_layout_code;
		LevelData leveldata;
		int lowestSun = 50;
		std::unordered_set<int> processed_zombie_ids;

		// 时间类
		TimeStruct start_time = TimeStruct::getNow();
		// 布尔标志位
		bool is_speed_up = false; // 是否开启了加速
		bool is_auto = false; // 是否开启了自动收集
		bool has_started = false;


		game_controler.auto_collect(false); // 关掉自动收集
		game_controler.reset_speed(); // 恢复原速


		// ------------- 4. 游戏前准备 -----------------------
		game_controler.setInjectors(); // 停掉游戏种植
		game_controler.disable_maidCheat();

		game_controler.board->GetMiscellaneous()->Round = 0;
		game_controler.board->Sun = 150;
		leveldata.initial_sun = game_controler.board->Sun;

		game_controler.clear_reverse_all_plants();
		game_controler.clear_not_colleted_sun();
		game_controler.clear_all_zombies();
		game_controler.clear_all_bullets();
		game_controler.update_brains();
		game_controler.board->GamePaused = false;

		logger_layout->log(std::string(get_terminal_width(), '-'), Logger::INFO);
		logger_layout->log("已经进入ize, 现在开始布阵", Logger::DEBUG);


		// ------------------- 5.开始游戏主循环 ---------------
		MSG msg = { 0 };
		while (true) {
			// ---------------------- 5.1 监测快捷键 -------------------------
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_HOTKEY) {
					// shift+D 切换加速
					if (msg.wParam == 1) {  // shift+D 切换加速与原速
						is_speed_up = !is_speed_up;
						if (is_speed_up) {
							game_controler.set_speed_10x();
							logger_layout->log(" 当前加速倍率: 10x", Logger::DEBUG);
						}
						else {
							game_controler.reset_speed();
							logger_layout->log(" 关闭加速", Logger::DEBUG);
						};
					}
					// shift+A 切换自动收集
					else if (msg.wParam == 2) { // shift+A 切换自动收集
						is_auto = !is_auto;
						game_controler.auto_collect(is_auto);
						logger_layout->log(std::string(is_auto ? "打开" : "关闭") + "自动收集", Logger::DEBUG);
					}
					// shift+q 退出
					else if (msg.wParam == 3) {
						//得分取一位小数
						std::ostringstream oss;
						oss << std::fixed << std::setprecision(1) << (std::round(leveldata.score * 10) / 10.0);
						std::string score_str = oss.str();

						logger_layout->log(std::string(get_terminal_width(), '-'), Logger::INFO);
						logger_layout->log(start_time.getCurrentTime() + " 提前结束游戏!", Logger::INFO);
						auto game_over_str = std::string("GameOver..");
						Creator::CreateCaption(game_over_str.c_str() + '\0', game_over_str.size(), CaptionStyle::Lowermiddle);
						goto gameover;
					}
					// shift+j 跳关
					else if (msg.wParam == 4) {
						game_controler.board->Win();
						logger_layout->log("跳过第" + std::to_string(game_controler.board->GetMiscellaneous()->Round) + "关, 当前脑子数: " + std::to_string(game_controler.countEatenBrain()) + "!", Logger::DEBUG);
					}
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}


			// ---------------------- 5.2 检测重开 ---------------------------
			do { // board不正确才是重开
				if (!game_controler.pvz // 如果pvz都找不到了就不算重开了
					|| PVZ::Memory::ReadPointer(0x6a9ec0, 0x768) == 0 // 如果完全没有board就是不在ize界面了
					|| current_address == PVZ::Memory::ReadPointer(0x6a9ec0, 0x768) // 如果当前的board地址和记录的地址相同就代表没啥问题
					|| game_controler.pvz->GameState != PVZGameState::Playing // 如果现在没在进行游戏，那就不用管了
					) continue;
				// 确实重开了
				while (current_address != PVZ::Memory::ReadPointer(0x6a9ec0, 0x768)) {
					std::this_thread::sleep_for(std::chrono::milliseconds(100));// 等一下pvzclass找到新board
					game_controler.board = PVZ::GetBoard();// 重新拿一下board;
					current_address = game_controler.board->GetBaseAddress();
				}
				game_controler.board->GetMiscellaneous()->Round = current_flag + 1; // 直接跳到下一关
			} while (0);


			// ---------------------- 5.3 检测跨关 ---------------------------
			do {
				if (!game_controler.pvz // 游戏崩溃了就别进跨关
					|| !game_controler.board
					|| current_address != game_controler.board->GetBaseAddress()
					|| game_controler.board->GetMiscellaneous()->Round == current_flag
					|| game_controler.pvz->GameState != PVZGameState::Playing) continue;

				// ------------- 正常跨关 --------------
				// (1) 存档上一关数据
				if (has_started) save_data.push_back(leveldata);
				// (2) 先布阵
				current_flag = game_controler.board->GetMiscellaneous()->Round;
				auto ls = code_generator.generate_incompleteLevel_one_code(current_flag); all_layout_code.push_back(ls);
				int theme_index = 0; int flower_num = 0; int sun = 0;
				std::array<int, 25> orders = {};
				// 检测布阵码是否合法
				if (!GenerateLayoutCode::decode_layout_string(ls, theme_index, flower_num, sun, orders)) {
					std::cout << "布阵码不合法" << std::endl;
					return;
				}
				game_controler.set_layout_order(theme_index, flower_num, sun, orders);
				leveldata.setlayout_time = TimeStruct::getNow();
				logger_layout->log("当前对第" + std::to_string(current_flag) + "关进行布阵", Logger::DEBUG);

				// (3) 初始化下一关数据
				leveldata.init();
				leveldata.score = current_flag;
				leveldata.initial_sun = game_controler.board->Sun;
				game_controler.reset_speed(); is_speed_up = false;
				processed_zombie_ids.clear();

				// (4) 布阵器打印上一关数据
				if (!has_started) continue;
				std::ostringstream oss;
				oss << std::fixed << std::setprecision(1) << (std::round(leveldata.eaten_brain_count * 0.2 * 10) / 10.0);
				std::string score_str = oss.str();
				logger_layout->log((TimeStruct::getNow() - start_time).enPrint()
					+ " 通过第" + std::to_string(game_controler.board->GetMiscellaneous()->Round)
					+ "关, 得分: " + score_str
					, Logger::INFO);
				
			} while (0);

			// ---------------------- 5.4 检测开始游戏 --------------------------
			if (!has_started) {
				// 跳过已经开始游戏的
				if (!game_controler.board || // 找不到board
					game_controler.pvz->LevelId != PVZLevel::I_Zombie_Endless ||
					game_controler.pvz->GameState != PVZGameState::Playing)
					continue;
				// 判断游戏开始条件:放了一个僵尸
				if (game_controler.board->ZombiesCount != 1) continue;

				// 开始游戏了
				has_started = true;
				start_time = TimeStruct::getNow();
				// 第一个僵尸释放时间，记录第一关的反应时间，僵尸释放时间-布阵时间
				leveldata.first_zombie_release_time = start_time;
				logger_layout->log(start_time.getCurrentTime() + " 开始游戏!", Logger::INFO);
				logger_layout->log(std::string(get_terminal_width(), '-'), Logger::INFO);
			}

			// ---------------------- 5.5 监测释放的僵尸花费 --------------------------
			do {
				if (!game_controler.board) continue;

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
				}
			} while (0);


			// ---------------------- 5.5 监测脑子变化 --------------------------
			do {
				if (leveldata.eaten_brain_count == game_controler.countEatenBrain()
					|| game_controler.countEatenBrain() == 0 // 刚进入新的一关
					) continue;

				leveldata.score = game_controler.board->GetMiscellaneous()->Round + game_controler.countEatenBrain() * 0.2;
				leveldata.last_brain_eaten_time = TimeStruct::getNow();
				leveldata.brain_eaten_times.push_back(leveldata.last_brain_eaten_time);
				leveldata.eaten_brain_count = game_controler.countEatenBrain();

				logger_layout->log((leveldata.last_brain_eaten_time - start_time).enPrint() + "吃了第" + std::to_string(game_controler.countEatenBrain()) + "个脑子", Logger::DEBUG);
			} while (0);


			// 阳光用完无所谓
		};
		// goto退出总循环
		gameover:
		{
			// 关卡结束后打印本次冲关数据
			std::string layout_codes;
			for (const auto& ls : all_layout_code) {
				layout_codes += ls + ".";
			}
			layout_codes = layout_codes.substr(0, layout_codes.size() - 1); // 去掉最后的'.'
			logger_layout->log("本次残局关卡布阵码为(已复制到剪切板): \n" + layout_codes, Logger::INFO);
			copyToClipBoard(layout_codes);

			// 把加速和自动收集关了
			game_controler.auto_collect(false);
			game_controler.reset_speed();
			game_controler.enable_maidCheat();
			return;
		}
	}

	// 连续布阵
	void continue_layout(const std::string ls) {
		// ------------ 1. 验证布阵码合法性  -----------------
		auto ss = std::stringstream(ls);
		auto str = std::string();
		auto vec = std::vector<std::string>();
		while (getline(ss, str, '.')) vec.push_back(str);
		for (auto it : vec) {
			int theme_index = static_cast<int>(it[0] - '0');
			int flower_num = static_cast<int>(it[1] - '0');
			int sun = std::stoi(it.substr(2, 2)) * 25;
			std::string order_str = it.substr(4);
			if (theme_index > 8 || theme_index < 1) {
				std::cout << "输入不合法" << std::endl;
				return;
			}
			if (flower_num < 1 || flower_num > 8) {
				std::cout << "输入不合法" << std::endl;
				return;
			}
		};

		// --------------- 2. 找到pvz --------------------
		GameControl game_controler;
		while (!game_controler.find_pvz()) { Sleep(1); };
		std::cout << "已找到pvz!" << std::endl;
		while (!game_controler.is_in_ize()) { Sleep(1); };
		std::cout << "已进入ize!" << std::endl;
		std::cout << "正在布阵!" << std::endl;

		int current_flag = -1;
		auto current_address = game_controler.board->GetBaseAddress();

		// ------------------- 3. 连续布阵 ----------------------		
		game_controler.setInjectors();
		game_controler.board->Sun = 150;
		game_controler.board->GetMiscellaneous()->Round = 0;


		RegisterHotKey(NULL, 1, MOD_SHIFT, 'Q'); // 强制退出
		RegisterHotKey(NULL, 2, MOD_SHIFT, 'R'); // 重新布阵

		MSG msg = { 0 };
		while (true) {
			// ---------------------- 6.1 监测快捷键 -------------------------
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_HOTKEY) {
					// shift+q 强制结束
					if (msg.wParam == 1) {
						goto gameover;
					}
					// shift+R重新对当前关卡布阵
					else if (msg.wParam == 2) {
						// 清除本关
						game_controler.clear_not_colleted_sun();
						game_controler.update_brains();
						game_controler.clear_all_zombies();
						game_controler.clear_all_bullets();

						current_flag = game_controler.board->GetMiscellaneous()->Round;
						std::string one_layoutString = vec[current_flag];
						int theme_index = 0; int flower_num = 0; int sun = 0; std::array<int, 25> orders = {};
						if (!GenerateLayoutCode::decode_layout_string(one_layoutString, theme_index, flower_num, sun, orders)) {
							std::cout << "当前关布阵码不合法" << std::endl;
							goto gameover;
						}
						game_controler.set_layout_order(theme_index, flower_num, sun, orders);
					}
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			// ---------------------- 跨关
			do {
				if (!game_controler.pvz // 游戏崩溃了就别进跨关
					|| !game_controler.board
					|| current_address != game_controler.board->GetBaseAddress()
					|| game_controler.board->GetMiscellaneous()->Round == current_flag
					|| game_controler.pvz->GameState != PVZGameState::Playing) continue;

				// -------------- 6.3.1 通关了 --------------
				if (game_controler.board->GetMiscellaneous()->Round >= vec.size()) {
					std::cout << "布阵完毕!" << std::endl;
					goto gameover;
				}

				// 布阵
				current_flag = game_controler.board->GetMiscellaneous()->Round;
				std::string one_layoutString = vec[current_flag];
				int theme_index = 0; int flower_num = 0; int sun = 0; std::array<int, 25> orders = {};
				if (!GenerateLayoutCode::decode_layout_string(one_layoutString, theme_index, flower_num, sun, orders)) {
					std::cout << "当前关布阵码不合法" << std::endl;
					goto gameover;
				}
				game_controler.set_layout_order(theme_index, flower_num, sun, orders);

			} while (0);

			if (!game_controler.board) {
				game_controler.refind_pvz();
			}
		}

		gameover:
		{
			std::cout << "结束布阵!" << std::endl;
			UnregisterHotKey(NULL, 1);
			UnregisterHotKey(NULL, 2);
		}



	}


public:
	// 实例化布阵器
	ConsoleControler() {
		// 布阵器实现
		setlocale(LC_ALL, ".936"); // 设置编码格式
		SetConsoleTitle(WINDOW_NAME);
	}

	bool clean_log() {
		// 布阵器定期清理日志文件
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

	// 布阵器循环测试
	void main() {
		while(true){
			// 打印功能卡
			std::cout << std::string(get_terminal_width(), '*') << std::endl;
			std::cout << INIT_WORDS << std::endl;
			std::cout << std::string(get_terminal_width(), '*') << std::endl;
			// 读指令
			std::string s;
			std::cin >> s;
			{
				// 使用说明
				if (!s.compare("0")) {// 使用说明
					std::cout << USE_GUIDES << std::endl;
				}
				// 30min限时玩法布阵
				else if (!s.compare("1")) {
					std::cout << "请先重开游戏保证第一关栈位正确, 并输入布阵码: " << std::endl;
					std::string ls;
					std::cin >> ls;

					std::cout << "是否开启反作弊(1为开启, 0为关闭): " << std::endl;
					std::string cmd1;
					std::cin >> cmd1;
					bool is_cheat_check = !cmd1.compare("1") ? true : false;

					// 创建并启动新线程（立即执行）
					disable_quick_edit_mode();
					register_SpeedRaceMode_hotkey();
					SpeedRun30min(ls, is_cheat_check);
					unregister_SpeedRaceMode_hotkey();
					enable_quick_edit_mode();
				}
				// 生成25关随机阵型代码
				else if (!s.compare("2")) {
					GenerateLayoutCode code_generator;
					auto ls = code_generator.generate_ssb_code();
					std::cout << ls << std::endl;
					copyToClipBoard(ls);
					std::cout << "已复制到剪贴板,可直接粘贴使用" << std::endl;
					continue;
				}
				// 选手生成唯一机器码 发给裁判
				else if (!s.compare("3")) {
					auto machine_code = EncryptUtils::generate_machine_code();
					std::cout << machine_code << std::endl;
					copyToClipBoard(machine_code);
					std::cout << "本台设备的机器码已复制到剪贴板,请私发给裁判" << std::endl;
					continue;
				}
				// 裁判输入双机器码及有效期生成加密机器码
				else if (!s.compare("4")) {
					std::vector<std::array<std::string, 3>> machine_code_info;

					std::cout << "请输入本次竞赛的玩家数量:" << std::endl;
					int player_num;
					std::cin >> player_num;
					if (player_num <= 0) {
						std::cout << "输入不合法!" << std::endl;
						continue;
					};

					bool input_correct = true;
					for (int i = 0; i < player_num; i++) {
						std::array<std::string, 3> player_info;
						// 1. 拿到机器码
						std::cout << "请输入玩家" << i + 1 << "的机器码:";
						std::string input;
						std::cin >> input;
						if (input.size() != 32) { // 机器码长度不合法
							std::cout << "机器码长度不合法不合法! 请重新输入" << std::endl;
							input_correct = false;
							continue;
						}

						// 2. 拿到这个玩家的开始布阵时间戳
						std::string input2;
						std::time_t startTs;
						std::cout << "请设置其布阵的有效 起始 时间(如:2025-04-07-11-02-00) : ";
						std::cin >> input2;
						if (!TimeStruct::parseTimestamp(input2, startTs)) {
							std::cout << "时间格式解析失败，请重新输入..\n";
							input_correct = false;
							continue;
						}

						// 3. 拿到这个玩家的结束布阵时间戳
						std::string input3;
						std::time_t endTs;
						std::cout << "请设置其布阵的有效 结束 时间(如:2025-04-07-11-02-00) : ";
						std::cin >> input3;
						if (!TimeStruct::parseTimestamp(input3, endTs)) {
							std::cout << "时间格式解析失败，请重新输入..\n";
							input_correct = false;
							continue;
						}

						// 比较开始和结束时间
						if (endTs <= startTs) {
							std::cerr << "结束时间必须晚于开始时间，请重新输入。\n";
							input_correct = false;
							continue;
						}

						player_info[0] = input; player_info[1] = std::to_string(startTs); player_info[2] = std::to_string(endTs);
						machine_code_info.push_back(player_info);
						std::cout << std::string(get_terminal_width(), '-') << std::endl;
					}

					if (!input_correct) continue;


					// 拼接规则加密
					GenerateLayoutCode code_generator;
					std::string ls = code_generator.generate_ssb_code();
					std::string encode_data = EncryptUtils::encode_ls(machine_code_info, ls);
					copyToClipBoard(encode_data);
					std::cout << encode_data << std::endl;
					std::cout << "加密布阵码已复制到剪贴板,请私发给选手" << std::endl;
				}
				// 选手解密后进行布阵
				else if (!s.compare("5")) {
					// 0. 输入加密数据（比如从剪贴板或用户输入获取）
					std::string enc_ls;
					std::cout << "请输入加密布阵码：" << std::endl;
					std::cin >> enc_ls;

					std::vector<std::array<std::string, 3>> machine_code_info;
					std::string ls;

					if (!EncryptUtils::decode_ls(enc_ls, machine_code_info, ls)) {
						std::cout << "解密失败, 请重新尝试!" << std::endl;
						continue;
					}

					std::string machine_code = EncryptUtils::generate_machine_code();
					std::array<std::string, 3> play_info = {};
					// 1. 如果机器码不在列表中，不合法continue
					bool found = false;
					for (const auto& it : machine_code_info) {
						if (it[0] == machine_code) {
							found = true;
							play_info = it;
							break;
						}
					}
					if (!found) {
						std::cout << "输入不合法,当前机器不满足此布阵码的有效身份!" << std::endl;
						continue;
					}

					// 2. 校验是否在有效期内
					std::size_t ts_start = 0;
					std::size_t ts_end = 0;
					std::size_t ts_now = static_cast<std::size_t>(std::time(nullptr));

					// 解析开始时间戳
					try {
						ts_start = std::stoull(play_info[1]);
					}
					catch (...) {
						std::cout << "起始时间错误!" << std::endl;
						continue;
					}

					// 现在布阵的时间必须大于等于起始时间
					if (ts_now < ts_start) {
						std::cout << "布阵码尚未生效，请在指定时间后再使用!" << std::endl;
						continue;
					}

					// 解析结束时间戳
					try {
						ts_end = std::stoull(play_info[2]);
					}
					catch (...) {
						std::cout << "结束时间戳格式错误!" << std::endl;
						continue;
					}

					// 校验是否过期
					if (ts_now > ts_end) {
						std::cout << "此布阵码对于本台机器已经过期!" << std::endl;
						continue;
					}

					int left_min = static_cast<int>((ts_end - ts_now) / 60);
					int left_second = static_cast<int>((ts_end - ts_now) % 60);
					std::cout << "当前时间信息: " << TimeStruct::getCurrentTime() << " 布阵码有效期剩余：" << left_min << ":" << left_second << std::endl;

					// 创建并启动新线程（立即执行）
					disable_quick_edit_mode();
					register_SpeedRaceMode_hotkey();
					SpeedRun30min(ls, true);
					unregister_SpeedRaceMode_hotkey();
					enable_quick_edit_mode();


					// 弹出日志文件
					std::string exeDir = GetExeDirectory(); std::string targetFolder = exeDir + "\\IZESpeedLayoutDatas";
					HINSTANCE result = ShellExecuteA(NULL, "open", targetFolder.c_str(), NULL, NULL, SW_SHOWNORMAL);
					if ((int)result <= 32) {
						std::cerr << "打开文件夹失败，错误代码：" << (int)result << std::endl;
						continue;
					}
					std::cout << "已打开日志文件所处文件夹，请妥善保存双日志文件！" << std::endl;
				}
				// 残局练习
				else if (!s.compare("6")) {
					// 创建并启动新线程（立即执行）
					disable_quick_edit_mode(); // 布阵器禁用编辑功能：包括复制什么的
					register_reviewMode_hotkey();
					IncompleteLevel();
					unregister_reviewMode_hotkey();
					enable_quick_edit_mode();
				}
				// 冲关玩法
				else if (!s.compare("7")) 
				{
					// 处理用户输入
					std::cout << "是否禁用女仆(1为禁用, 0为不禁用): " << std::endl;
					std::string cmd1;
					std::cin >> cmd1;
					bool is_ban_maidCheat = (cmd1 == "1");

					// 创建并启动新线程（立即执行）
					disable_quick_edit_mode(); // 布阵器禁用编辑功能：包括复制什么的
					register_reviewMode_hotkey();
					LevelRush(is_ban_maidCheat);
					unregister_reviewMode_hotkey();
					enable_quick_edit_mode();

				}
				// 生成自定义布阵码
				else if (!s.compare("8")) 
				{
					GenerateLayoutCode code_generator;
					// --------------- 1. 多少关 -------------------------
					std::cout << "布阵码关数: " << std::endl;
					int flag_num;
					std::cin >> flag_num;
					if (flag_num <= 0) {
						std::cout << "输入不合法!" << std::endl;
						continue;
					}

					// ---------------- 2. 主题分布 -------------------------
					std::cout << "主题分布(1为随机(1-8)，2为自定义): " << std::endl;
					int theme_distribution;
					std::cin >> theme_distribution;
					std::vector<int> theme_index_distribution = {};
					if (theme_distribution == 2) {
						std::cout << "你选择的是自定义主题分布, 请输入主题分布 " << std::endl;
						std::cout << "例如:输入1 - 8代表锁某个主题，输入关数长度字符串 代表指定关 为 指定主题(如11243228123...) : " << std::endl;
						std::string theme_index_string;
						std::cin >> theme_index_string;
						if (theme_index_string.size() == 1) {
							int num = theme_index_string[0] - '0';  // 转换字符为数字，例如 '3' -> 3
							for (int i = 0; i < flag_num; i++) theme_index_distribution.push_back(num);
						}
						else if (theme_index_string.size() == flag_num) {
							for (char ch : theme_index_string) {
								// 检查字符是否为数字字符
								if (std::isdigit(static_cast<unsigned char>(ch))) {
									int num = ch - '0';  // 转换字符为数字，例如 '3' -> 3
									theme_index_distribution.push_back(num);
								}
								else {
									std::cerr << "字符不合法, 非数字字符: " << ch << std::endl;
									continue;
								}
							}
						}
						else {
							std::cout << "长度不合法!" << std::endl;
							continue;
						}
					}
					else if (theme_distribution == 1) {
						for (int i = 0; i < flag_num; i++) {
							theme_index_distribution.push_back(code_generator.get_random_in_range(1, 8));
						}
					}
					else {
						std::cout << "输入不合法!" << std::endl;
						continue;
					}

					// ----------------------- 3. 花数分布 --------------------------- 
					std::cout << "花数分布(1为随机(1-8)，2为自定义): " << std::endl;
					int flower_distribution;
					std::cin >> flower_distribution;
					std::vector<int> flower_num_distribution = {};
					if (flower_distribution == 2) {
						std::cout << "你选择的是自定义花数分布, 请输入花数分布 " << std::endl;
						std::cout << "例如:输入1 - 8代表锁花数，输入关数长度字符串 代表指定关 为 指定花数(如87655...) : " << std::endl;
						std::string flower_num_string;
						std::cin >> flower_num_string;

						if (flower_num_string.size() == 1) {
							int num = flower_num_string[0] - '0';  // 转换字符为数字，例如 '3' -> 3
							for (int i = 0; i < flag_num; i++) flower_num_distribution.push_back(num);
						}
						else if (flower_num_string.size() == flag_num) {
							for (char ch : flower_num_string) {
								// 检查字符是否为数字字符
								if (std::isdigit(static_cast<unsigned char>(ch))) {
									int num = ch - '0';  // 转换字符为数字，例如 '3' -> 3
									flower_num_distribution.push_back(num);
								}
								else {
									std::cerr << "字符不合法, 非数字字符: " << ch << std::endl;
									continue;
								}
							}
						}
						else {
							std::cout << "长度不合法!" << std::endl;
							continue;
						}

					}
					else if (flower_distribution == 1) {
						for (int i = 0; i < flag_num; i++) {
							flower_num_distribution.push_back(code_generator.get_random_in_range(1, 8));
						}
					}
					else {
						std::cout << "输入不合法!" << std::endl;
						continue;
					}

					// -------------------- 4. 是否残局 ------------------------------------
					std::cout << "是否为残局(1是，2否): " << std::endl;
					std::string cmd;
					std::cin >> cmd;
					bool is_IncompleteLevel = (cmd == "1");


					// ---------------- 4. 生成加密种子顺便生成布阵码 -----------------------
					std::string ls;
					for (int i = 0; i < flag_num; i++) {
						std::string sun_str;
						if (is_IncompleteLevel) {
							std::cout << "设置花数+阳光的上下限(用-分隔，例如:500-800, 计算方法是: 2花100阳光->500):" << std::endl;
							std::string cmd1;
							std::cin >> cmd1;

							int lower = 0, upper = 0; bool input_valid = false;
							// 查找分隔符位置
							size_t dash_pos = cmd1.find('-');

							// 检查分隔符是否存在且不在首尾
							if (dash_pos == std::string::npos || dash_pos == 0 || dash_pos == cmd1.length() - 1) {
								std::cout << "错误：输入格式无效，请使用 下限-上限 格式" << std::endl;
								continue;
							}

							// 分割字符串
							std::string lower_str = cmd1.substr(0, dash_pos);
							std::string upper_str = cmd1.substr(dash_pos + 1);

							// 转换数字
							try {
								lower = std::stoi(lower_str);
								upper = std::stoi(upper_str);

								// 检查下限 <= 上限
								if (lower > upper) {
									std::cout << "错误：下限不能大于上限" << std::endl;
									continue;
								}
								input_valid = true;
							}
							catch (const std::invalid_argument&) {
								std::cout << "错误：输入包含非数字字符" << std::endl;
							}
							catch (const std::out_of_range&) {
								std::cout << "错误：输入数值超出整数范围" << std::endl;
							}

							if (!input_valid) {
								std::cout << "输入不合法!" << std::endl;
								continue;
							}

							int sun = 0;
							//TODO: 根据不同主题设置合适阳光
							// 要求 sun*25+flower_num*200在500->800之间
							do {
								sun = code_generator.get_random_in_range(3, 12); // 直接赋值给外层变量
							} while (
								(sun * 25 + flower_num_distribution[i] * 200) < 500 || // 总和太小
								(sun * 25 + flower_num_distribution[i] * 200) > 800    // 或太大时继续循环
								);
							// 格式化sun为两位: 9->09
							std::ostringstream oss;
							oss << std::setw(2) << std::setfill('0') << sun;
							sun_str = oss.str();
						}
						else {
							sun_str = "00";
						}

						auto result = code_generator.generate_arr_seed(static_cast<Theme>(theme_index_distribution[i]));
						auto seed = result.second;
						ls += std::to_string(theme_index_distribution[i])
							+ std::to_string(flower_num_distribution[i])
							+ sun_str
							+ code_generator.encode_seed(seed)
							+ ".";
					}
					std::string layout_code = ls.substr(0, ls.size() - 1);
					std::cout << layout_code << std::endl;
					copyToClipBoard(layout_code);
					std::cout << "已复制到剪贴板,可直接粘贴使用" << std::endl;
					continue;

				}
				else if (!s.compare("9")) {
					std::cout << "请先重开游戏保证第一关栈位正确, 并输入布阵码: " << std::endl;
					std::string ls;
					std::cin >> ls;
					continue_layout(ls);
					continue;
				}
				else if (!s.compare("a")) {
					// 导出本关ize阵型代码
					std::string ls;
					if (!export_layout_string(ls)) {
						std::cout << "请检查后再次读取阵型导出代码" << std::endl;
						continue;
					};

					copyToClipBoard(ls);
					std::cout << "当前阵型已导出为阵型代码, 请注意分辨!" << std::endl;
					std::cout << "当前带阳光信息的布阵码为: " <<ls << "  忽略阳光的布阵码为 : " << ls.substr(0, 2) + "00" + ls.substr(4) << std::endl;
					std::cout << "带阳光信息的布阵码已复制到剪贴板, 如不需要请手动复制另一个布阵码" << std::endl;
				}
				else {
					std::cout<<"输入不合法"<<std::endl;
					continue;
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
