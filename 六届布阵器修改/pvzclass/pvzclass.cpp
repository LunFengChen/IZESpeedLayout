#include "pvzclass.h"
#include <conio.h>


TimeStruct::TimeStruct(std::size_t _second)
{
	minute = _second / 60;
	second = _second % 60;
};

TimeStruct TimeStruct::operator+(const TimeStruct& ts) const
{
	auto second = this->minute * 60 + this->second + ts.minute * 60 + ts.second;
	return TimeStruct(second);
};

TimeStruct TimeStruct::operator-(const TimeStruct& ts) const
{
	auto second = this->minute * 60 + this->second - ts.minute * 60 - ts.second;
	return TimeStruct(second);
};

// 截图
//std::string imgs_path = "/imgs/";// 时间戳做文件夹
size_t game_start_timesecond = 0;


// variables
std::deque<uint32_t> pEffects{}; // 所有的被创建的particleSystem对象的指针
constexpr size_t BASE = 66;
constexpr uint32_t PORTAL_FLAG = 0x6b0900;
auto enterTimes = std::vector<TimeStruct>(25, TimeStruct::getNow());
SlotType slotZombieTypes = { ZombieType::Imp, // ZombieType::Zombie
	ZombieType::ConeheadZombie, ZombieType::PoleVaultingZombie, // ZombieType::ZombieYeti, ZombieType::FlagZombie, ZombieType::NewspaperZombie
	ZombieType::BucketheadZombie, ZombieType::BungeeZombie, // ZombieType::ScreenDoorZombie, ZombieType::JackintheboxZombie
	ZombieType::DiggerZombie, ZombieType::LadderZombie, ZombieType::FootballZombie, // ZombieType::PogoZombie
	ZombieType::DancingZombie
};
// 初始值为游戏卡槽, 每一行内是一个随机池子

std::vector<Injector*> injectors = {};

constexpr std::array<std::array<ZombieType::ZombieType, 5>, 4> pool = { {
	{ ZombieType::Imp, ZombieType::Zombie, ZombieType::None, ZombieType::None,ZombieType::None },
	{ ZombieType::ConeheadZombie, ZombieType::PoleVaultingZombie, ZombieType::ZombieYeti, ZombieType::FlagZombie, ZombieType::NewspaperZombie },
	{ ZombieType::BucketheadZombie, ZombieType::BungeeZombie, ZombieType::ScreenDoorZombie, ZombieType::JackintheboxZombie, ZombieType::None },
	{ ZombieType::DiggerZombie, ZombieType::LadderZombie, ZombieType::FootballZombie, ZombieType::PogoZombie, ZombieType::None } } };

constexpr std::array<int, 4> poolLimit = { 1, 3, 2, 3 };  // 四个池子分别抽取的个数
constexpr std::array<int, 4> poolMax = { 2, 5, 4, 4 };
std::random_device rd;
std::mt19937_64 gen(rd()); // 全局随机数生成器



PVZ* __pvz = nullptr; // 全局pvz对象
LevelState* levelState = nullptr;// 全局 切关时游戏状态保存





void iterPlants(PVZ* pvz, VoidLambda<Plant> func, bool reverseOrder)
{
	std::shared_ptr<Plant>* pPlants = new std::shared_ptr<Plant>[1024];
	auto count = pvz->GetAllPlants(pPlants);
	for (size_t i = 0; i < count; i++)
	{
		auto idx = reverseOrder ? count - 1 - i : i;
		if (pPlants[idx] != nullptr) func(pvz, pPlants[idx]);
	}
	delete[] pPlants;
}

void iterZombies(PVZ* pvz, VoidLambda<Zombie> func, bool reverseOrder)
{
	std::shared_ptr<Zombie>* pZombies = new std::shared_ptr<Zombie>[1024];
	auto count = pvz->GetAllZombies(pZombies);
	for (size_t i = 0; i < count; i++)
	{
		auto idx = reverseOrder ? count - 1 - i : i;
		if (pZombies[idx] != nullptr) func(pvz, pZombies[idx]);
	}
	delete[] pZombies;
}

void iterGriditems(PVZ* pvz, VoidLambda<PVZ::Griditem> func, bool reverseOrder)
{
	std::shared_ptr<PVZ::Griditem>* pGriditems = new std::shared_ptr<PVZ::Griditem>[1024];
	auto count = pvz->GetAllGriditems(pGriditems);
	for (size_t i = 0; i < count; i++)
	{
		auto idx = reverseOrder ? count - 1 - i : i;
		if (pGriditems[idx] != nullptr) func(pvz, pGriditems[idx]);
	}
	delete[] pGriditems;
}

void iterCoins(PVZ* pvz, VoidLambda<PVZ::Coin> func, bool reverseOrder)
{
	std::shared_ptr<PVZ::Coin>* pCoins = new std::shared_ptr<PVZ::Coin>[1024];
	auto count = pvz->GetAllCoins(pCoins);
	for (size_t i = 0; i < count; i++)
	{
		auto idx = reverseOrder ? count - 1 - i : i;
		if (pCoins[idx] != nullptr) func(pvz, pCoins[idx]);
	}
	delete[] pCoins;
}

class ConsoleControl {
public:

	ConsoleControl() {
		// TODO: 统计使用次数
		setlocale(LC_ALL, ".936");// 设置编码格式
		SetConsoleTitle(WINDOW_NAME); // 设置窗口名字
	}


	~ConsoleControl() {
		std::cout << "退出布阵器" << std::endl;
	}


	static int get_terminal_width() {
		int width = 80; // 默认宽度
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		width = csbi.srWindow.Right - csbi.srWindow.Left - 1;

		return width;
	}

	static void copyToClipBoard(const std::string& str)
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

	void main() {
		while (true) {
			std::cout << std::string(get_terminal_width(), '*') << std::endl; // 分隔符
			std::cout << INIT_WORDS << std::endl; // 打印布阵器初始信息
			std::cout << std::string(get_terminal_width(), '*') << std::endl; // 分隔符

			std::string s; // 用户输入信息
			//std::getline(std::cin, s); // 整行读取，但需要进行strip一下
			std::cin >> s;


			if (!s.compare("0"))
			{
				std::cout << "退出." << std::endl;
				return;
			}
			else if (!s.compare("1"))
			{
				std::cout << "请输入布阵码：" << std::endl;
				std::string ls;
				std::cin >> ls;

				std::array<Theme, 25> themes{}; // 存储每一关的主题
				std::array<size_t, 25> seeds{}; // 存储每一关的种子
				std::vector<SmallFeature> smallFeatures{}; // 存储小特性

				if (!LayoutStringGenerator::decode_layout_string(ls, themes, seeds)) // 解析不出东西
				{
					std::cout << std::string(ConsoleControl::get_terminal_width(), '*') << std::endl;
					std::cout << "输入不合法!" << std::endl;
					std::cout << std::string(ConsoleControl::get_terminal_width(), '*') << std::endl; // 分隔符

					continue;
				}
				// 正常输入
				std::cout << std::string(ConsoleControl::get_terminal_width(), '*') << std::endl; // 分隔符
				std::cout << "输入成功!" << std::endl;
				std::cout << std::string(ConsoleControl::get_terminal_width(), '*') << std::endl; // 分隔符

				int i = 0;
				if (!ls.compare(""))
				{
					smallFeatures = { SmallFeature::NIL };
				}
				else
				{
					int ret = LayoutStringGenerator::decodeFeatureString(ls, slotZombieTypes, smallFeatures);
					if (ret == 0) continue;
				}

				i = GameControl::startGame(themes, seeds, smallFeatures); // 正常开始游戏，themes是主题，seeds是每一关的种子（据此生成图）
				if (i == 2)
				{
					GameControl::crashSolution();
				}
				else if (i != 0)
				{
					std::cout << "可能出错了!, 出错代码" << i << std::endl;
				}
				GameControl::exitGame(__pvz, slotZombieTypes, smallFeatures);
			}
			else if (!s.compare("2")) // 生成布阵码
			{
				auto ls = LayoutStringGenerator::generateLayoutString();
				ls.append(".IhGv"); // “常规”小鹤双拼
				std::cout << ls << std::endl;
				ConsoleControl::copyToClipBoard(ls);
				std::cout << "已复制到剪贴板,可直接粘贴使用" << std::endl;
				continue;
			}
			else if (!s.compare("3"))
			{
				auto zombies = slotZombieTypes;
				LayoutStringGenerator::genSlotTypeCli(zombies);
				std::vector<SmallFeature> givenFeatures{};
				std::cout << SMALL_FEATURE_WORDS << std::endl;
				std::cout << "请输入小特性数量，输入0则不生成小特性: " << std::endl;
				int number;
				std::cin >> number;
				std::cin.ignore(LLONG_MAX, '\n');
				if (number < 0)
				{
					std::cout << "输入不合法!" << std::endl;
					continue;
				}
				auto vec = std::vector<std::string>();
				if (number == 0)
				{
					vec.push_back("0");
				}
				else
				{
					std::cout << "请输入指定的小特性前方编号, 用空格分隔, 随机请输入0" << std::endl;
					std::string featureStr;
					getline(std::cin, featureStr);
					auto ss = std::stringstream(featureStr);
					auto str = std::string();


					while (getline(ss, str, ' '))
					{
						vec.push_back(str);
					}
				}
				auto ls = LayoutStringGenerator::generateLayoutString();
				if (vec.size() == 0)
				{
					std::cout << "输入错误! 请重新生成" << std::endl;
					continue;
				}
				else if (stoi(vec[0]) != 0)
				{
					for (const auto& it : vec)
					{
						givenFeatures.push_back(static_cast<SmallFeature>(stoi(it)));
					}
				}
				auto smallFeatures = LayoutStringGenerator::generateSmallFeatures(number, extraFeatures, givenFeatures);
				std::sort(smallFeatures.begin(), smallFeatures.end());
				LayoutStringGenerator::getFeaturedLayoutString(ls, zombies, smallFeatures);
				std::cout << ls << std::endl;
				ConsoleControl::copyToClipBoard(ls);
				std::cout << "已复制到剪贴板" << std::endl;
				auto st = std::string("最终生成小特性为");
				for (auto it : smallFeatures)
				{
					st.append(SMALL_FEATURE_NAMES[static_cast<int>(it) - 1]);
				}
				std::cout << st << std::endl;
				continue;
			}
			else if (!s.compare("4"))
			{
				std::cout << BIG_FEATURE_WORDS << std::endl;
				std::cout << SMALL_FEATURE_WORDS << std::endl;
			}
			else if (!s.compare("5"))
			{
				std::cout << USING_INSTRUCTIONS << std::endl;
			}
			else if (!s.compare("6")) { // 生成机器码(sha256)，再制作公钥为"xiaofeng", 用aes128进行加密
				// 生成机器码
				std::string machine_code = ENCRYPT::MachineCode::generate();
				// aes128进行加密
				auto key = ENCRYPT::CryptoUtils::sha256("xiaofeng"); // 扩充到16字符
				std::string aes_machine_code = ENCRYPT::CryptoUtils::aes128Encrypt(machine_code, key);
				// 玩家上传这个数据给到裁判
				std::cout << aes_machine_code << std::endl;
				ConsoleControl::copyToClipBoard(aes_machine_code);
				std::cout << "已复制到剪贴板,可直接粘贴使用" << std::endl;
				continue;
			}
			else if (!s.compare("7")) {
				// 裁判拿到两个机器码
				// A aes218解密
				std::cout << std::string(ConsoleControl::get_terminal_width(), '*') << std::endl; // 分隔符
				std::cout << "请输入玩家A的机器码：" << std::endl;
				std::string enc_machine_code_A;
				std::cin >> enc_machine_code_A;
				// A aes218解密
				std::cout << std::string(ConsoleControl::get_terminal_width(), '*') << std::endl; // 分隔符
				std::cout << "请输入玩家B的机器码：" << std::endl;
				std::string enc_machine_code_B;
				std::cin >> enc_machine_code_B;


				auto key = ENCRYPT::CryptoUtils::sha256("xiaofeng"); // 扩充到16字符
				auto machine_code_A = ENCRYPT::CryptoUtils::aes128Decrypt(enc_machine_code_A, key);
				auto machine_code_B = ENCRYPT::CryptoUtils::aes128Decrypt(enc_machine_code_A, key);

				// 解密
				//std::cout << machine_code_A << std::endl;
				//std::cout << machine_code_B << std::endl;





			}
#ifdef _DEBUG
			else if (!s.compare("p7ia"))
			{
				DWORD pid = ProcessOpener::Open();
				if (!pid)
				{
					std::cout << "未找到pvz!" << std::endl;
					return 1;
				}
				std::cout << "已找到pvz!" << std::endl;
				__pvz = new PVZ(pid);
				auto pvz = __pvz;
				EnableBackgroundRunning(true);


				PVZ::Memory::WriteMemory<uint32_t>(5, PLANT_FLAG);

				Injector* lPlantStart = new Injector{ L_PLANT_START };
				lPlantStart->jmp(L_PLANT_FIRST);
				injectors.push_back(lPlantStart);

				Injector* lPlantFirst = new Injector{ L_PLANT_FIRST };
				lPlantFirst->pushad().mov(REG::ECX, PLANT_FLAG).ptrCmp(REG::ECX, 5).je(L_PLANT_END)
					.mov(REG::EAX, 0).jmp(L_PLANT_LOOP);
				injectors.push_back(lPlantFirst);

				Injector* lPlantLoop = new Injector{ L_PLANT_LOOP };
				lPlantLoop->push(REG::EAX).mov(REG::ECX, 12).mul(REG::ECX).add(REG::EAX, PLANT_MEMORY)
					.movPtr(REG::EDI, 0x6a9ec0).movPtr(REG::EDI, REG::EDI, 0x768).movPtr(REG::EDI, REG::EDI, 0x160)
					.movPtr(REG::EBX, REG::EAX).movPtr(REG::ECX, REG::EAX, 4).push(REG::ECX)
					.movPtr(REG::ECX, REG::EAX, 8).push(REG::ECX).call(0x42a660)
					.pop(REG::EAX).cmp(REG::EAX, 24).ja(L_PLANT_END).add(REG::EAX, 1).jmp(L_PLANT_LOOP);
				injectors.push_back(lPlantLoop);

				Injector* lPlantEnd = new Injector{ L_PLANT_END };
				lPlantEnd->mov(REG::ECX, PLANT_FLAG).ptrMov(REG::ECX, 0, 5).popad().call(0x41ca10).jmp(L_PLANT_START + 5);
				injectors.push_back(lPlantEnd);

				Injector* lStopGamePlanting = new Injector(0x42A6C0, { 0xc2, 0x0c, 0x00 }); // ret 000c
				injectors.push_back(lStopGamePlanting);

				GameControl::setLayout(__pvz, 0, Theme::INSTANT_KILL, 465466, { SmallFeature::NIL });


			}
#endif
			else
			{
				std::cout << "输入不合法!" << std::endl;
			}
		}
	}

};





class LayoutStringGenerator {
public:


	static bool setMixingLayout(std::array<PlantType::PlantType, 25>& plantTypes, size_t seed, Theme theme)
	{
		std::mt19937_64 genMixing(seed + 1);

		std::uniform_int_distribution<unsigned int> rngTheme(1, 7); // [1..7], 为不是胆小的所有主题.
		auto extraTheme = static_cast<Theme>(rngTheme(genMixing));

		// 将两个主题的二期植物数组拼起来shuffle
		auto originalTypes = LayoutStringGenerator::getThemePlantTypes(theme);
		auto vec = std::vector<PlantType::PlantType>(originalTypes.begin(), originalTypes.end());
		for (auto it : LayoutStringGenerator::getThemePlantTypes(extraTheme)) vec.push_back(it);
		std::shuffle(vec.begin(), vec.end(), genMixing);

		// 把二期植物换成shuffle后的前17个植物
		for (size_t i = 0; i < 17; i++)
		{
			plantTypes[i + 8] = vec[i];
		}
		return true;
	}

	static std::array<PlantType::PlantType, 17> getThemePlantTypes(Theme theme)
	{
		switch (theme)
		{
		case Theme::COMPOSITE:
			return {
				PlantType::Wallnut,
				PlantType::Torchwood,
				PlantType::PotatoMine,
				PlantType::Chomper, PlantType::Chomper,
				PlantType::Peashooter,
				PlantType::SplitPea,
				PlantType::Kernelpult,
				PlantType::Threepeater,
				PlantType::SnowPea,
				PlantType::Squash,
				PlantType::Fumeshroom,
				PlantType::UmbrellaLeaf,
				PlantType::Starfruit,
				PlantType::Magnetshroom,
				PlantType::Spickweed, PlantType::Spickweed
			};
		case Theme::CONTROL:
			return {
				PlantType::Torchwood,
				PlantType::SplitPea, PlantType::SplitPea, PlantType::SplitPea,
				PlantType::Repeater,
				PlantType::Kernelpult, PlantType::Kernelpult, PlantType::Kernelpult,
				PlantType::Threepeater,
				PlantType::SnowPea, PlantType::SnowPea, PlantType::SnowPea,
				PlantType::UmbrellaLeaf,
				PlantType::Magnetshroom,
				PlantType::Spickweed, PlantType::Spickweed, PlantType::Spickweed
			};
		case Theme::INSTANT_KILL:
			return {
				PlantType::PotatoMine, PlantType::PotatoMine, PlantType::PotatoMine, PlantType::PotatoMine,
				PlantType::Chomper, PlantType::Chomper, PlantType::Chomper,
				PlantType::Squash, PlantType::Squash, PlantType::Squash,
				PlantType::Fumeshroom, PlantType::Fumeshroom, PlantType::Fumeshroom, PlantType::Fumeshroom,
				PlantType::Spickweed, PlantType::Spickweed, PlantType::Spickweed
			};
		case Theme::PEAS:
			return {
				PlantType::SnowPea, PlantType::SnowPea, PlantType::SnowPea, PlantType::SnowPea, PlantType::SnowPea,
				PlantType::SnowPea, PlantType::SnowPea, PlantType::SnowPea, PlantType::SnowPea,
				PlantType::SplitPea, PlantType::SplitPea, PlantType::SplitPea, PlantType::SplitPea,
				PlantType::Repeater, PlantType::Repeater, PlantType::Repeater, PlantType::Repeater
			};
		case Theme::STAR_AND_SPIKE:
			return {
				PlantType::Spickweed, PlantType::Spickweed, PlantType::Spickweed, PlantType::Spickweed,
				PlantType::Spickweed, PlantType::Spickweed, PlantType::Spickweed, PlantType::Spickweed, PlantType::Spickweed,
				PlantType::Starfruit, PlantType::Starfruit, PlantType::Starfruit, PlantType::Starfruit,
				PlantType::Starfruit, PlantType::Starfruit, PlantType::Starfruit, PlantType::Starfruit
			};
		case Theme::EXPLODING:
			return {
				PlantType::PotatoMine, PlantType::PotatoMine, PlantType::PotatoMine, PlantType::PotatoMine,
				PlantType::PotatoMine, PlantType::PotatoMine, PlantType::PotatoMine, PlantType::PotatoMine, PlantType::PotatoMine,
				PlantType::Chomper, PlantType::Chomper, PlantType::Chomper, PlantType::Chomper,
				PlantType::Chomper, PlantType::Chomper, PlantType::Chomper, PlantType::Chomper
			};
		case Theme::MAGNAT_AND_FUME:
			return {
				PlantType::Fumeshroom, PlantType::Fumeshroom, PlantType::Fumeshroom, PlantType::Fumeshroom,
				PlantType::Fumeshroom, PlantType::Fumeshroom, PlantType::Fumeshroom, PlantType::Fumeshroom, PlantType::Fumeshroom,
				PlantType::Magnetshroom, PlantType::Magnetshroom, PlantType::Magnetshroom, PlantType::Magnetshroom,
				PlantType::Magnetshroom, PlantType::Magnetshroom, PlantType::Magnetshroom, PlantType::Magnetshroom
			};
		case Theme::SCARDY:
			return {
				PlantType::Scaredyshroom, PlantType::Scaredyshroom, PlantType::Scaredyshroom, PlantType::Scaredyshroom,
				PlantType::Scaredyshroom, PlantType::Scaredyshroom, PlantType::Scaredyshroom, PlantType::Scaredyshroom,
				PlantType::Scaredyshroom, PlantType::Scaredyshroom, PlantType::Scaredyshroom, PlantType::Scaredyshroom,
				PlantType::Sunflower, PlantType::Sunflower, PlantType::Sunflower, PlantType::Sunflower, PlantType::Sunflower
			};
		default:
			return std::array<PlantType::PlantType, 17>();
		}
	}

	static int setFirstPhasePlants(std::array<PlantType::PlantType, 25>& arr, int flowerNumber)
	{
		for (size_t i = 0; i < flowerNumber; i++)
		{
			arr[i] = PlantType::Sunflower;
		}
		for (size_t i = flowerNumber; i < 8; i++)
		{
			arr[i] = PlantType::Puffshroom;
		}
		return 0;
	}

	static std::array<PlantType::PlantType, 25> getFlagPlantTypes(int flag, Theme theme)
	{
		auto ret = std::array<PlantType::PlantType, 25>();
		if (flag == 0) setFirstPhasePlants(ret, 8);
		else if (flag == 1) setFirstPhasePlants(ret, 7);
		else if (flag == 2) setFirstPhasePlants(ret, 6);
		else if (flag == 3 || flag == 4) setFirstPhasePlants(ret, 5);
		else if (flag == 5 || flag == 6) setFirstPhasePlants(ret, 4);
		else if (flag == 7 || flag == 8) setFirstPhasePlants(ret, 3);
		else if (flag == 9) setFirstPhasePlants(ret, 2);
		else if (flag == 10 || flag == 11) setFirstPhasePlants(ret, 1);
		else if (flag == 12 || flag == 13) setFirstPhasePlants(ret, 2);
		else setFirstPhasePlants(ret, 3);

		for (size_t i = 0; i < 17; i++)
		{
			ret[i + 8] = getThemePlantTypes(theme)[i];
		}

		return ret;
	}

	static bool getFeaturedLayoutString(std::string& originalLayoutString, const SlotType& zombies, const std::vector<SmallFeature>& smallFeatures)
	{
		originalLayoutString.append(".VgHo."); // “整活”小鹤双拼
		for (auto it : zombies)
		{
			originalLayoutString.append(1, numberToChar(static_cast<int>(it)));
		};
		originalLayoutString.append("-");
		for (auto it : smallFeatures)
		{
			originalLayoutString.append(std::to_string(static_cast<int>(it)));
		}
		return true;
	}



	static int decodeFeatureString(std::string& layoutString, SlotType& zombies, std::vector<SmallFeature>& smallFeatures)
	{
		auto ss = std::stringstream(layoutString);
		auto str = std::string();
		auto vec = std::vector<std::string>(2);
		zombies = {};
		smallFeatures = {};
		size_t idx = 0;

		while (getline(ss, str, '-'))
		{
			vec[idx] = str;
			idx++;
		}
		int ret = idx == 2 ? 1 : 0;
		idx = 0;
		for (auto it : vec[0])
		{
			zombies[idx] = static_cast<ZombieType::ZombieType>(charToNumber(it));
			idx++;
		}
		if (vec[1].size() == 0)
		{
			std::cout << "是否忘记输入小特性? 请重新输入" << std::endl;
			return 0;
		}
		for (auto it : vec[1])
		{
			smallFeatures.push_back(static_cast<SmallFeature>(it - '0'));
		}

		return ret;
	}


	// 指定ascii码转换为[0, BASE)范围内的数字
	static size_t charToNumber(char c)
	{
		if (c >= '0' && c <= '9') return c - '0';
		if (c <= 'Z') return c + 10 - 'A';
		return c + 8 - 'A';
	}
	// [0, BASE)范围内的数字转换为指定ascii码
	static char numberToChar(size_t i)
	{
		if (i >= 0 && i <= 9) return i + '0';
		if (i <= 35) return i - 10 + 'A';
		return i - 8 + 'A';
	}
	// 对每一个seed做对BASE的进制转换到string, 首位加主题对应号码, 用'.'分隔
	// 进制转换, 第1位为个位, 第2位为十位, 往上类推
	static std::string encodeLayoutString(const std::array<Theme, 25>& themes, const std::array<size_t, 25>& seeds)
	{
		auto ret = std::string();

		for (size_t i = 0; i < 25; i++)
		{
			ret.append(std::to_string(static_cast<int>(themes[i])));
			auto t = seeds[i];

			//std::cout << "seed[" << i << "]: " << t << std::endl;
			while (t)
			{
				ret.append(1, numberToChar(t % BASE));
				t /= BASE;
			}
			if (i != 24) ret.append(1, '.');

		}


		return std::move(ret);
	}

	static std::array<int, 25> getShuffledArray(size_t seed)
	{
		std::array<int, 25> arr = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 , 20, 21, 22, 23, 24 };
		std::mt19937_64 gen(seed);
		std::shuffle(arr.begin(), arr.end(), gen);
		return arr;
	}

	static inline std::pair<int, int> intToPos(int i)
	{
		return { i / 5, i % 5 };
	}

	static size_t generateSeed(Theme theme)
	{
		switch (theme)
		{
		case Theme::COMPOSITE:
			while (true)
			{
				size_t ret = gen();
				auto arr = getShuffledArray(ret);
				if (intToPos(arr[8]).second >= 3 && intToPos(arr[9]).second >= 3) return ret; // 8->o 9->j
			}
			break;
		case Theme::CONTROL:
			while (true)
			{
				size_t ret = gen();
				auto arr = getShuffledArray(ret);
				if (intToPos(arr[8]).second >= 3) return ret; // 8->j
			}
			break;
		default:
			return gen();
		}

	}

	static std::array<Theme, 25> generateThemeDistribution()
	{
		// order of B theme.	
		std::array<Theme, 5> BOrder = { Theme::PEAS, Theme::STAR_AND_SPIKE, Theme::EXPLODING, Theme::MAGNAT_AND_FUME, Theme::SCARDY };
		while (BOrder[0] != Theme::SCARDY && BOrder[1] != Theme::SCARDY) std::shuffle(BOrder.begin(), BOrder.end(), gen);


		// flags of B themes.
		std::uniform_int_distribution<unsigned int> d1(1, 4);
		std::uniform_int_distribution<unsigned int> d2(0, 4);
		std::array<int, 5> tmp1 = { d1(gen), d2(gen), d2(gen), d2(gen), d2(gen) };

		auto ret = std::array<Theme, 25>();
		for (size_t i = 0; i < 5; i++)
		{
			ret[tmp1[i] + 5 * i] = BOrder[i];
		}

		std::uniform_int_distribution<unsigned> d3(0, 5);
		for (auto& it : ret)
		{
			if (it != Theme::NIL) continue;
			int i = d3(gen);
			if (i == 5) it = Theme::INSTANT_KILL;
			else if (i == 4 || i == 3) it = Theme::CONTROL;
			else if (i == 0 || i == 1 || i == 2) it = Theme::COMPOSITE;
		}

		return ret;
	}



	static std::string generateLayoutString()
	{
		auto themes1 = generateThemeDistribution();
		auto seeds1 = std::array<size_t, 25>();
		for (size_t i = 0; i < 25; i++)
		{
			seeds1[i] = generateSeed(themes1[i]);
		}
		return encodeLayoutString(themes1, seeds1);
	}

	static bool decode_layout_string(std::string& layoutString, std::array<Theme, 25>& themes, std::array<size_t, 25>& seeds) {// layoutString: 加密后字符串；返回主题、种子
		auto ss = std::stringstream(layoutString);
		auto str = std::string(); // 存储布阵码的结尾标识
		auto vec = std::vector<std::string>(); // vec[i]存储每一关的字符串，一共25*11，并没有返回，作为中间值

		while (getline(ss, str, '.')) // 读结尾标识
		{
			vec.push_back(str);
		}

		if (vec.size() <= 25) return false;

		for (size_t i = 0; i < 25; i++)
		{
			themes[i] = static_cast<Theme>(vec[i][0] - '0'); // 首位字符代表主题，-0的作用是将字符转换为对应的整数值
			if (vec[i][0] - '0' <= 0 || vec[i][0] - '0' > 8) return false; // 只能输入1-8主题的内容
			seeds[i] = 0; // 第二位代表每一关的种子，不同码是变动的

			for (size_t j = 1; j < vec[i].length(); j++)
			{
				seeds[i] += charToNumber(vec[i][j]) * stPow(BASE, j - 1); // 布阵码,每一个字符按照66的j-1次幂进行计算，后数字对66取模获取对应的数字
			}
		}


		if (!vec[25].compare("IhGv"))
		{
			layoutString = "";
			return true;
		}
		if (!vec[25].compare("VgHo"))
		{
			layoutString = vec[26];
			return true;
		}
		return false;
	}

	// 生成大特性僵尸 givenTypes输入各类索引+1, (或输入0表示无限制).
	static SlotType generateSlotTypes(std::array<std::vector<int>, 4> givenTypes)
	{
		auto ret = SlotType();
		ret[ret.size() - 1] = ZombieType::DancingZombie;
		size_t initedTypeCount = 0;

		for (size_t i = 0; i < 4; i++)
		{
			const auto& gt = givenTypes[i];
			auto poolTypes = pool[i];
			const int count = poolLimit[i];
			std::vector<ZombieType::ZombieType> tmpZt{};
			for (auto it : gt)
			{
				if (it <= 0 || it > poolMax[i])
				{
					std::cout << "输入错误! 按照默认卡槽" << std::endl;
					ret = slotZombieTypes;
					return ret;
				}
				if (poolTypes[it - 1] != ZombieType::None && (std::find(tmpZt.begin(), tmpZt.end(), poolTypes[it - 1]) == tmpZt.end())) tmpZt.push_back(poolTypes[it - 1]);
			}

			// 随机剩余僵尸
			if (tmpZt.size() < count)
			{
				for (size_t j = 0; j < tmpZt.size(); j++)
				{
					ret[initedTypeCount + j] = tmpZt[j];
				}
				std::shuffle(poolTypes.begin(), poolTypes.end(), gen);
				for (auto it : poolTypes)
				{
					if (it != ZombieType::None && std::find(tmpZt.begin(), tmpZt.end(), it) == tmpZt.end()) tmpZt.push_back(it);
				}
			}
			for (size_t j = 0; j < count; j++)
			{
				ret[initedTypeCount + j] = tmpZt[j];
			}
			initedTypeCount += count;
		}
		std::sort(ret.begin(), ret.end(), [](ZombieType::ZombieType zt1, ZombieType::ZombieType zt2)
			{
				return getCardOrder(zt1) < getCardOrder(zt2);
			});
		return ret;
	}

	static std::vector<SmallFeature> generateSmallFeatures(int count, bool hasSpecialFeatures, std::vector<SmallFeature> givenFeatures)
	{
		std::vector<SmallFeature> ret(count, SmallFeature::NO_SMALL_FEATURE);
		if (count == 0)
		{
			ret = { SmallFeature::NO_SMALL_FEATURE };
			return ret;
		}
		size_t idx = 0;
		for (auto it : givenFeatures)
		{
			if (idx >= count) return ret;
			if ((it == SmallFeature::NIL || it == SmallFeature::NO_SMALL_FEATURE) || (!hasSpecialFeatures && (it == SmallFeature::GARLICS || it == SmallFeature::LEFT_MOVE)))
			{
				std::cout << "输入有误, 按照无小特性生成" << std::endl;
				ret = { SmallFeature::NO_SMALL_FEATURE };
				return ret;
			}
			if (std::find(ret.begin(), ret.end(), it) != ret.end()) continue;
			ret[idx] = it;
			idx++;
		}
		std::vector<SmallFeature> features;
		if (hasSpecialFeatures) features = { SmallFeature::MINI_ZOMBIE, SmallFeature::AVOID_FLOWERS, SmallFeature::MIXING, SmallFeature::TELEPORT, SmallFeature::GARLICS, SmallFeature::LEFT_MOVE };
		else features = { SmallFeature::MINI_ZOMBIE, SmallFeature::AVOID_FLOWERS, SmallFeature::MIXING, SmallFeature::TELEPORT };
		std::shuffle(features.begin(), features.end(), gen);
		for (auto it : features)
		{
			if (idx >= count) return ret;
			if (std::find(ret.begin(), ret.end(), it) == ret.end())
			{
				ret[idx] = it;
				idx++;
			}
		}
		return ret;
	}


	static void genSlotTypeCli(SlotType& zombies)
	{
		std::cout << "大特性设置: 从以下四个池子中选择僵尸" << std::endl;
		std::cout << "以下为池子: " << std::endl;
		std::cout << "第一池 抽取 1 个: 【小鬼】【普僵】" << std::endl;
		std::cout << "第二池 抽取 3 个: 【路障】【撑杆】【雪人】【旗帜】【报纸】" << std::endl;
		std::cout << "第三池 抽取 2 个: 【铁桶】【蹦极】【铁门】【小丑】" << std::endl;
		std::cout << "第四池 抽取 3 个: 【矿工】【扶梯】【橄榄】【跳跳】" << std::endl;
		std::cout << "请输入数字：数字表示指定该行中的第x个僵尸。对每个池子，输入0表示该池子随机；输入R表示全随机、输入D表示按照原版卡槽。" << std::endl;
		std::cout << "同一个池子中的数字用空格分隔，不同池子用回车分隔。" << std::endl;

		std::array<std::vector<int>, 4> givenTypes = { {{}, {}, {}, {}} };
		std::cin.ignore(LLONG_MAX, '\n');
		for (size_t i = 0; i < 4; i++)
		{
			std::string inputStr;
			getline(std::cin, inputStr);
			if (!inputStr.compare("R"))
			{
				std::cout << "全随机!" << std::endl;
				zombies = generateSlotTypes(givenTypes);
				return;
			}
			else if (!inputStr.compare("D"))
			{
				std::cout << "按照原版卡槽! " << std::endl;
				return;
			}
			auto ss = std::stringstream(inputStr);
			auto str = std::string();
			std::vector<std::string> vec = std::vector<std::string>();

			while (getline(ss, str, ' '))
			{
				vec.push_back(str);
			}

			if (vec.size() == 0)
			{
				std::cout << "输入错误!  第" << i + 1 << "池全随机!" << std::endl;
				continue;
			}

			if (stoi(vec[0]) == 0)
			{
				std::cout << "第" << i + 1 << "池全随机!" << std::endl;
				continue;
			}
			for (auto it : vec)
			{
				givenTypes[i].push_back(stoi(it));
			}
			std::cout << "第" << i + 1 << "池已指定!" << std::endl;
		}
		zombies = generateSlotTypes(givenTypes);
	}
	static constexpr int getCardOrder(ZombieType::ZombieType zt)
	{
		switch (zt)
		{
		case ZombieType::Zombie:
			return 50;
		case ZombieType::FlagZombie:
			return 75;
		case ZombieType::ConeheadZombie:
			return 76;
		case ZombieType::PoleVaultingZombie:
			return 77;
		case ZombieType::BucketheadZombie:
			return 125;
		case ZombieType::NewspaperZombie:
			return 78;
		case ZombieType::ScreenDoorZombie:
			return 100;
		case ZombieType::FootballZombie:
			return 175;
		case ZombieType::DancingZombie:
			return 350;
		case ZombieType::JackintheboxZombie:
			return 79;
		case ZombieType::DiggerZombie:
			return 126;
		case ZombieType::PogoZombie:
			return 127;
		case ZombieType::ZombieYeti:
			return 80;
		case ZombieType::BungeeZombie:
			return 128;
		case ZombieType::LadderZombie:
			return 150;
		case ZombieType::Imp:
			return 51;
		default:
			return 0;
		}
	}


};

class GameControl {
public:
	GameControl() {

	};

	~GameControl() {

	};
	// 检测游戏开启
	static bool is_game_on()
	{
		return PVZ::Memory::ReadMemory<int>(0x6a9ec0) != 0;
	}

	// 获取游戏mode
	static int get_game_mode() {
		return PVZ::Memory::ReadPointer(0x6a9ec0, 0x7f8);
	}
	// 获取游戏ui
	static int get_game_ui()
	{
		return PVZ::Memory::ReadPointer(0x6a9ec0, 0x7fc);
	}


	static bool is_in_ize() { // 进入了ize中
		return (is_game_on() && get_game_mode() == 70 && (get_game_ui() == 2 || get_game_ui() == 3));
	}

	static bool setTeleport(PVZ* pvz, size_t seed)
	{
		// 生成传送门
		std::mt19937_64 genTeleport(seed + 1);
		std::uniform_int_distribution<unsigned int> genTpRow(0, 4);
		int interTpRow = genTpRow(genTeleport);
		int outerTpRow = genTpRow(genTeleport);
		iterGriditems(pvz, [](PVZ* pvz, std::shared_ptr<PVZ::Griditem> pGriditem)
			{
				if (pGriditem->Type == GriditemType::PortalYellow) pGriditem->Remove();
			});
		Sleep(20);
		setPortal(interTpRow, 1, outerTpRow, 5, 9, 9, 9, 9);
		EnablePortal(pvz, true);
		Sleep(20);
		PVZ::Memory::WriteMemory<uint32_t>(PORTAL_FLAG, 3);
		Sleep(20);
		//iterGriditems(pvz, [](PVZ* pvz, std::shared_ptr<PVZ::Griditem> pGriditem)
		//	{
		//		if (pGriditem->Type == GriditemType::PortalBlue) pGriditem->Remove();
		//	});
		FixPortal(true);
		setPortal(9, 9, 9, 9, 9, 9, 9, 9);
		return true;
	}


	static int countEatenBrain()
	{
		return PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x160, 0x60);
	}

	// 汇编删脑子
	static bool clear_all_brains() {
		if (is_game_on() && (get_game_ui() == 2 || get_game_ui() == 3))
		{
			// 初始化汇编环境
			IZE::Code code;
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
					code.asm_mov_exx(IZE::Reg::ESI, addr);
					code.asm_call(0x0044d000);
				}
			}
			// 注入代码
			code.asm_ret();
			code.asm_code_inject(PVZ::Memory::hProcess);
			return true;
		}
		return false;
	}


	// 更新脑子
	static bool update_brains() {
		if (is_in_ize()) { // 在ize中即可
			// 1. 清空脑子
			clear_all_brains();
			// 2. 设置关卡进程0/5，进度条0
			// 设置吃的脑子数为0，进度条为0;
			PVZ::Memory::WriteMemory<int>(PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x160) + 0x60, 0);
			PVZ::Memory::WriteMemory<int>(PVZ::Memory::ReadPointer(0x6a9ec0, 0x768) + 0x5610, 0);

			// 3. 生成5个脑子
			for (size_t i = 0; i < 5; i++) {
				SPT<PVZ::Griditem> iz_brain_new = Creator::CreateGriditem();
				// 设置场地物品的基本参数
				iz_brain_new->Row = i; iz_brain_new->Column = 0;
				iz_brain_new->Layer = 302000 + i * 10000;//图层
				iz_brain_new->NotExist = false;
				iz_brain_new->Type = GriditemType::IZBrain;
				// IZ脑子专有属性：hp 和 Y坐标
				PVZ::Memory::WriteMemory<int>(iz_brain_new->BaseAddress + 0x18, 70);// hp
				PVZ::Memory::WriteMemory<float>(iz_brain_new->BaseAddress + 0x28, 120 + i * 100); // Y
			}
			return true;
		}
		return false;
	}
	// 写内存删除全部僵尸
	static void clear_all_zombies() {
		auto zombie_count_max = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x94); // 0x6a9ec0: lawnl; 0x768: board; 0x94:zombie_count_max
		auto zombie_offset = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0x90); // 0x90: zombie

		for (size_t i = 0; i < zombie_count_max; i++) { // 0xec: zombie_dead ;0x28: zombie_status
			if (!PVZ::Memory::ReadMemory<bool>(zombie_offset + 0xec + i * 0x15c)) { // 如果僵尸没死就给他们设置死的状态
				PVZ::Memory::WriteMemory<int>(zombie_offset + 0x28 + i * 0x15c, 3); // 3为iz布阵器那种删除， 1为缓慢消失
			}
		}
	}


	// 汇编删全部子弹
	static void clear_all_bullets() {
		unsigned int bullet_struct_size = 0x94; // 每个植物的分块内存大小 332字节
		auto bullet_count_max = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0xcc);
		auto bullet_offset = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768, 0xc8);


		// 初始化汇编环境
		IZE::Code code;
		code.asm_init();

		for (size_t i = 0; i < bullet_count_max; i++) {
			auto bullet_disappered = PVZ::Memory::ReadMemory<bool>(bullet_offset + 0x50 + bullet_struct_size * i);
			if (!bullet_disappered) { // 子弹消失则为true, 取反
				uint32_t addr = bullet_offset + bullet_struct_size * i;
				code.asm_mov_exx(IZE::Reg::EAX, addr);
				code.asm_call(0x46eb20); // 调用植物删除函数call_delete_plant
			}
		}

		// 执行上述汇编命令
		code.asm_ret();
		code.asm_code_inject(PVZ::Memory::hProcess);
	}


	static bool CreateDirectoryIfNotExists(const std::string& dir) {
		struct _stat info;
		if (_stat(dir.c_str(), &info) != 0) {
			// 目录不存在，尝试创建
			if (_mkdir(dir.c_str()) != 0) {
				std::cerr << "无法创建目录: " << dir << std::endl;
				return false;
			}
		}
		return true;
	}

	static bool SaveBitmapToFile(HBITMAP hBitmap, const std::string& filename) {
		BITMAP bmp;
		BITMAPINFO bmi = { 0 };
		BITMAPFILEHEADER bfh;
		std::vector<BYTE> pixels;

		HDC hdcMem = CreateCompatibleDC(NULL);
		HGDIOBJ hbmOld = SelectObject(hdcMem, hBitmap);

		GetObject(hBitmap, sizeof(BITMAP), &bmp);

		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = bmp.bmWidth;
		bmi.bmiHeader.biHeight = bmp.bmHeight;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 24;
		bmi.bmiHeader.biCompression = BI_RGB;

		int rowSize = ((bmp.bmWidth * 24 + 31) / 32) * 4;
		int imageSize = rowSize * bmp.bmHeight;
		pixels.resize(imageSize);

		GetDIBits(hdcMem, hBitmap, 0, bmp.bmHeight, pixels.data(), &bmi, DIB_RGB_COLORS);

		SelectObject(hdcMem, hbmOld);
		DeleteDC(hdcMem);


		std::ofstream file(filename, std::ios::binary);
		if (!file) {
			std::cerr << "无法创建文件: " << filename << std::endl;
			return false;
		}

		bfh.bfType = 0x4D42; // 'BM'
		bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		bfh.bfSize = bfh.bfOffBits + imageSize;
		bfh.bfReserved1 = 0;
		bfh.bfReserved2 = 0;

		file.write(reinterpret_cast<const char*>(&bfh), sizeof(bfh));
		file.write(reinterpret_cast<const char*>(&bmi.bmiHeader), sizeof(bmi.bmiHeader));
		file.write(reinterpret_cast<const char*>(pixels.data()), pixels.size());
		file.close();

		return true;
	}


	static bool CaptureWindow(HWND hwnd, const std::string& filename) {
		HDC hdcWindow = GetDC(hwnd);
		HDC hdcMem = CreateCompatibleDC(hdcWindow);

		RECT rect;
		GetClientRect(hwnd, &rect);

		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;

		HBITMAP hbm = CreateCompatibleBitmap(hdcWindow, width, height);
		HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbm);

		BitBlt(hdcMem, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY);

		bool result = SaveBitmapToFile(hbm, filename);

		SelectObject(hdcMem, hbmOld);
		DeleteObject(hbm);
		DeleteDC(hdcMem);
		ReleaseDC(hwnd, hdcWindow);

		return result;
	}


	static void setLayout(PVZ* pvz, int flag, Theme theme, size_t seed, const std::vector<SmallFeature>& smallFeatures)
	{// flag:当前关数 theme:当前主题 布阵完截图
		auto plantTypes = LayoutStringGenerator::getFlagPlantTypes(flag, theme); // 获取不同主题的植物生成顺序
		auto orders = LayoutStringGenerator::getShuffledArray(seed); // 传入种子，按照种子复原植物位置

		std::array<bool, 2> garlicDirections = { 0, 0 }; // 向下0, 向上1
		int garlicDirectionsIdx = 0; // 遍历到index为几的大蒜

		// 有特性
		if (std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::NIL) == smallFeatures.end())
		{
			// CancelCardCooldown(true);

			setCards(slotZombieTypes, pvz);

			// 小特性有LEFT_MOVE时的处理（<int>6A9EC0+768+63C,1为开0为关）
			const int BASE_ADDRESS = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768);
			if (std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::LEFT_MOVE) != smallFeatures.end())
			{
				PVZ::Memory::WriteMemory<int>(BASE_ADDRESS + 0x63c, 1);
			}

			// 小特性有AVOID_FLOWERS时的处理
			if (std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::AVOID_FLOWERS) != smallFeatures.end())
			{
				PVZ::Memory::WriteMemory<byte>(0x41ba74, 0x01);
				PVZ::Memory::WriteMemory<byte>(0x41ba75, 0xDE);
				SetSunMax(30000);
				AutoCollect(pvz, true);

			}

			// 小特性有缝合MIXING时的处理
			if (std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::MIXING) != smallFeatures.end() && theme != Theme::SCARDY)
			{
				LayoutStringGenerator::setMixingLayout(plantTypes, seed, theme);
			}

			// 小特性有大蒜GARLICS时的处理, 这个有多个步骤过于复杂不拆函数
			if (std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::GARLICS) != smallFeatures.end())
			{
				// 几个大蒜
				std::mt19937_64 genGarlics(seed + 1);
				int garlicNumber = 0;
				do
				{
					if (flag == 0) continue;
					if (flag == 1)
					{
						garlicNumber = 1;
						continue;
					}
					std::uniform_int_distribution<int> rngGarlicNumber(1, 2);
					garlicNumber = rngGarlicNumber(genGarlics);

				} while (0);

				// 大蒜方向
				if (garlicNumber)
				{
					std::uniform_int_distribution<int> rngGarlicDirection(0, 1);
					for (auto& it : garlicDirections)
					{
						it = rngGarlicDirection(genGarlics);
					}
				}

				if (garlicNumber >= 1) plantTypes[7] = PlantType::Garlic;
				if (garlicNumber >= 2) plantTypes[6] = PlantType::Garlic;

				// 删大蒜动画
				for (auto& it : pEffects)
				{
					if (it == 0) continue;
					__removeEffect(it);
				}
				pEffects.clear();
			}

			// 处理传送特性
			if (std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::TELEPORT) != smallFeatures.end())
			{
				setTeleport(pvz, seed);
			}

		}

		// 第一关设置阳光150
		if (flag == 0) {
			// 删僵尸
			GameControl::clear_all_zombies();
			// 删子弹
			GameControl::clear_all_bullets();
			// 删除场上没收集的阳光; 总是删不干净（
			iterCoins(pvz, [](PVZ* pvz, std::shared_ptr<PVZ::Coin> pCoin)
				{
					if (pCoin->Type == CoinType::NormalSun) {
						pCoin->Collected = false;
						pCoin->NotExist = true;
					}
				});
			// 设置初始阳光150
			pvz->Sun = 150;
		}
		// 更新脑子
		GameControl::update_brains();



		// 删植物
		if (pvz->PlantsCount > 0) {
			iterPlants(pvz, [](PVZ* pvz, std::shared_ptr<Plant> pPlant)
				{
					if (!pPlant->NotExist) pPlant->Remove();
					Sleep(1); // 不sleep1会不逆序删.
				}, true); // 所有删植物均确保逆序删. 这样生成栈位将和自然发展完全相同.
		}

		// 种植物以及修改植物特性
		std::mt19937 genPuffshroom(seed + 1);
		std::uniform_int_distribution<int> rngPuffshroomX(-5, 4);
		std::uniform_int_distribution<int> rngPuffshroomY(-3, 2);

		//const uint32_t BASE_ADDR = PVZ::Memory::ReadMemory<uint32_t>(0x6a9ec0);

		for (size_t i = 0; i < 25; i++) // 种植物
		{//
			int written[] = { LayoutStringGenerator::intToPos(orders[i]).first , LayoutStringGenerator::intToPos(orders[i]).second ,static_cast<int>(plantTypes[i]) };
			PVZ::Memory::WriteArray<int>(PLANT_MEMORY + 12 * i, written, 12);
		}
		Sleep(20);
		PVZ::Memory::WriteMemory<uint32_t>(PLANT_FLAG, 3);
		Sleep(20);


		// ？
		std::vector<Plant*> pPlants{};
		iterPlants(pvz, [&](PVZ* _pvz, std::shared_ptr<Plant> pPlant)
			{
				pPlants.push_back(pPlant.get());
			});

		std::sort(pPlants.begin(), pPlants.end(), [](Plant*& p1, Plant*& p2)
			{
				return p1->Row * 5 + p1->Column < p2->Row * 5 + p2->Column;
			});

		for (auto& pPlant : pPlants)
		{
			//  大蒜
			if (pPlant->Type == PlantType::Garlic)
			{
				pPlant->Hp = 100;
				uint32_t direction = garlicDirections.at(garlicDirectionsIdx) ? -1 : 1; // 向下1向上-1
				if (pPlant->Row == 0) direction = 1;
				else if (pPlant->Row == 4) direction = -1;
				garlicDirectionsIdx++;

				// 生成大蒜箭头
				auto arrowType = direction == 1 ? EffectType::ARROW2 : EffectType::ARROW;
				int idxEffect = PVZ::Memory::ReadPointer(0x6a9ec0, 0x820, 0x0, 0xc);
				DWORD pEffect = PVZ::Memory::ReadPointer(0x6a9ec0, 0x820, 0x0, 0x0) + 0x2c * idxEffect;

				std::pair<int, int> pos = { pPlant->Row - 1, pPlant->Column - 1 };
				if (arrowType == EffectType::ARROW2)
				{
					Creator::CreateEffect(arrowType, posToFloat(pos).first + 20 + 180, posToFloat(pos).second - 10 + 200);
				}
				else Creator::CreateEffect(arrowType, posToFloat(pos).first - 10 + 180, posToFloat(pos).second - 80 + 200);

				pEffects.push_back(pEffect);


				//  大蒜方向开关
				PVZ::Memory::WriteMemory<uint32_t>(pPlant->BaseAddress + 0x80, direction);
			}

			// 小喷坐标统一
			if (pPlant->Type == PlantType::Puffshroom)
			{
				auto fPos = posToFloat(pPlant->Row, pPlant->Column);
				int x = static_cast<int>(fPos.first) + 40 + rngPuffshroomX(genPuffshroom);
				int y = static_cast<int>(fPos.second) + 40 + rngPuffshroomY(genPuffshroom);
				pPlant->X = x;
				pPlant->Y = y;
			}
		};

		// 每次布阵后进行截图   
		 // 截图保存 

		std::string dir = "./" + std::to_string(game_start_timesecond);
		if (GameControl::CreateDirectoryIfNotExists(dir)) {

			std::ostringstream filenameStream;

			std::string file_name = dir + "/" + std::to_string(flag) + ".bmp";
			filenameStream << file_name;
			std::string filename = filenameStream.str();
			std::this_thread::sleep_for(std::chrono::milliseconds(300)); // 等图生成
			GameControl::CaptureWindow(PVZ::Memory::mainwindowhandle, filename);
		}
	}

	static void setInjectors(const SlotType& zombieTypes, const std::vector<SmallFeature>& smallFeatures)
	{
		//bool isMiniZombie = std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::MINI_ZOMBIE) != smallFeatures.end();

		//bool isFeatured = std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::NIL) != smallFeatures.end();


		//Injector* allCardsAreZombieCard = new Injector{ 0x42b57b };
		//allCardsAreZombieCard->addConst<byte>(0xb0).addConst<byte>(0x01); // mov al,01
		//injectors.push_back(allCardsAreZombieCard);

		/*Injector* yetiSun = new Injector{ 0x0069F56C };
		yetiSun->addConst<uint32_t>(isMiniZombie ? 75 / 5 * 2 : 75);
		injectors.push_back(yetiSun);

		Injector* flagSun = new Injector{ 0x0069f2e4 };
		flagSun->addConst<uint32_t>(isMiniZombie ? 75 / 5 * 2 : 75);
		injectors.push_back(flagSun);

		Injector* newspaperSun = new Injector{ 0x0069F374 };
		newspaperSun->addConst<uint32_t>(isMiniZombie ? 75 / 5 * 2 : 75);
		injectors.push_back(newspaperSun);

		Injector* jackInTheBoxCard = new Injector{ 0x42a0bc };
		jackInTheBoxCard->addConst<uint32_t>(15);
		injectors.push_back(jackInTheBoxCard);

		Injector* jackInTheBoxSun = new Injector{ 0x467b48 };
		jackInTheBoxSun->addConst<uint32_t>(isMiniZombie ? 75 / 5 * 2 : 75);
		injectors.push_back(jackInTheBoxSun);

		Injector* pogoCard = new Injector{ 0x0042a0d2 };
		pogoCard->addConst<uint32_t>(18);
		injectors.push_back(pogoCard);

		Injector* pogoSun = new Injector{ 0x00467B84 };
		pogoSun->addConst<uint32_t>(isMiniZombie ? 125 / 5 * 2 : 125);
		injectors.push_back(pogoSun);

		Injector* screenDoorSun = new Injector{ 0x00467B3D };
		screenDoorSun->addConst<uint32_t>(isMiniZombie ? 100 / 5 * 2 : 100);

		injectors.push_back(screenDoorSun);*/

		Injector* disableMaidCheat = new Injector(0x52dfcb, { 0x68, 0x07, 0x00, 0x00, 0x8b, 0x80, 0x68, 0x55, 0x00, 0x00, 0x99, 0xf7, 0xf9, 0x8b, 0xc2, 0x99, 0xf7, 0xfe, 0x5e, 0xc3 });
		injectors.push_back(disableMaidCheat);

		//Injector* noDroppingCoins = new Injector(0x51d79a,{0xb0, 0x01, 0x90});
		//injectors.push_back(noDroppingCoins);

		Injector* disablePlantingEffect = new Injector{ 0x40ce60 };
		disablePlantingEffect->ret(0x0004);
		injectors.push_back(disablePlantingEffect);


		std::cout << "回到这一步" << std::endl;
		PVZ::Memory::WriteMemory<uint32_t>(PLANT_FLAG, 5);

		Injector* lPlantStart = new Injector{ L_PLANT_START };
		lPlantStart->jmp(L_PLANT_FIRST);
		injectors.push_back(lPlantStart);

		Injector* lPlantFirst = new Injector{ L_PLANT_FIRST };
		lPlantFirst->pushad().mov(REG::ECX, PLANT_FLAG).ptrCmp(REG::ECX, 5).je(L_PLANT_END)
			.mov(REG::EAX, 0).jmp(L_PLANT_LOOP);
		injectors.push_back(lPlantFirst);

		Injector* lPlantLoop = new Injector{ L_PLANT_LOOP };
		lPlantLoop->push(REG::EAX).mov(REG::ECX, 12).mul(REG::ECX).add(REG::EAX, PLANT_MEMORY)
			.mov(REG::ECX, -1).push(REG::ECX)
			.movPtr(REG::ECX, REG::EAX, 8).push(REG::ECX)
			.movPtr(REG::ECX, REG::EAX, 4).push(REG::ECX)
			.movPtr(REG::EAX, REG::EAX)
			.movPtr(REG::ECX, 0x6a9ec0).movPtr(REG::ECX, REG::ECX, 0x768).push(REG::ECX).call(0x40d120)
			.test(REG::EAX, REG::EAX).je(L_PLANT_TEST)
			.push(REG::EAX)
			.movPtr(REG::EAX, 0x6a9ec0).movPtr(REG::EAX, REG::EAX, 0x768).movPtr(REG::EAX, REG::EAX, 0x160).call(0x42a530)
			.jmp(L_PLANT_TEST);
		injectors.push_back(lPlantLoop);

		Injector* lPlantTest = new Injector{ L_PLANT_TEST };
		lPlantTest->pop(REG::EAX).cmp(REG::EAX, 23).ja(L_PLANT_END).add(REG::EAX, 1).jmp(L_PLANT_LOOP);
		injectors.push_back(lPlantTest);

		Injector* lPlantEnd = new Injector{ L_PLANT_END };
		lPlantEnd->mov(REG::EAX, PLANT_FLAG).ptrMov(REG::EAX, 0, 5).popad().call(0x41ca10).jmp(L_PLANT_START + 5);
		injectors.push_back(lPlantEnd);

		Injector* lStopGamePlanting = new Injector(0x42A6C0, { 0xc2, 0x0c, 0x00 }); // ret 000c
		injectors.push_back(lStopGamePlanting);

		//	if (isMiniZombie)
		//	{
		////		PVZ::Memory::WriteMemory<byte>(0x523ed5, 0xeb);
		//		Injector* miniZombieSwitch = new Injector(0x523ed5, { 0xeb });
		//		injectors.push_back(miniZombieSwitch);
		//		PVZ::Memory::WriteMemory<int>(0x467b60, 20);
		//		PVZ::Memory::WriteMemory<int>(0x467b60 + 6, 30);
		//		PVZ::Memory::WriteMemory<int>(0x467b60 + 6 * 2, 140);
		//		PVZ::Memory::WriteMemory<int>(0x467b60 + 6 * 3, 50);
		//		PVZ::Memory::WriteMemory<int>(0x467b60 + 6 * 4, 60);
		//		PVZ::Memory::WriteMemory<int>(0x467b60 + 6 * 5, 70);
		//	}
		//
		//	if (std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::GARLICS) != smallFeatures.end())
		//	{
		//		Injector* garlics1 = new Injector{ 0x52fcf0 };
		//		(*garlics1).jmp(0x6b0000).nop().nop();
		//		injectors.push_back(garlics1);
		//
		//		Injector* garlics2 = new Injector{ 0x6b0000 };
		//		(*garlics2).push(REG::EAX).mov(REG::EAX, -0x04).ptrAdd(REG::ESI, 0x40, REG::EAX).movPtr(REG::ECX, REG::ESI, 0x40).cmp(REG::ESI, 0x24, 36)
		//			.pop(REG::EAX).jne(0x52fcf7).push(REG::EAX).movPtr(REG::EAX, REG::ESI, 0x80).ptrMov(REG::EBP, 0x130, REG::EAX).pop(REG::EAX).jmp(0x52fcf7);
		//		injectors.push_back(garlics2);
		//
		//		Injector* garlics3 = new Injector{ 0x52b902 };
		//		(*garlics3).push(REG::EAX).movPtr(REG::EAX, REG::EDI, 0x130).ptrAdd(REG::EDI, 0x1c, REG::EAX).pop(REG::EAX).jmp(0x52b91a);
		//		for (size_t i = 0; i < 11; i++)
		//		{
		//			garlics3->nop();
		//		}
		//		injectors.push_back(garlics3);
		//	}
		//
		//	if (std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::LEFT_MOVE) != smallFeatures.end())
		//	{
		//		Injector* leftMove1 = new Injector{ 0x52af86 };
		//		(*leftMove1).jmp(0x6b0030);
		//		injectors.push_back(leftMove1);
		//
		//		Injector* leftMove2 = new Injector{ 0x6b0030 };
		//		(*leftMove2).pushad().cmp(REG::ESI, 0x24, 0x11).je(0x6b0077)
		//			.movPtr(REG::EAX, 0x6a9ec0).movPtr(REG::EAX, REG::EAX, 0x768).cmp(REG::EAX, 0x63c, 0)
		//			.je(0x6b0077).fld(REG::ESI, 0x2c).cmp(REG::ESI, 0xac, 0).jg(0x6b006e).fsub(0x6b0098).jmp(0x6b0074)
		//			.fsub(0x6b009c).fstp(REG::ESI, 0x2c).popad().call(0x52BCA0).jmp(0x52af8b);
		//		injectors.push_back(leftMove2);
		//
		//		Injector* leftMoveFloats = new Injector{ 0x6b0098 };
		//		(*leftMoveFloats).addConst<float>(0.1f).addConst(0.05f);
		//		injectors.push_back(leftMoveFloats);
		//	}
		//
		//	if (std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::TELEPORT) != smallFeatures.end())
		//	{
		//		constexpr uint32_t L_PORTAL_START = 0x42b34a;
		//		constexpr uint32_t L_PORTAL = 0x6b1850;
		//		constexpr uint32_t L_DELETE_BLUE_PORTAL = 0x6b1900;
		//		constexpr uint32_t L_PORTAL_FIRST = 0x6b1b00;
		//		constexpr uint32_t L_END = 0x6b1C00;
		//
		//		Injector* lPortalStart = new Injector{ L_PORTAL_START };
		//		lPortalStart->jmp(L_PORTAL_FIRST);
		//		injectors.push_back(lPortalStart);
		//
		//		Injector* lPortalFirst = new Injector{ L_PORTAL_FIRST };
		//		lPortalFirst->pushad().mov(REG::ESI, 0x6b0910).ptrMov(REG::ESI,0, 0).jmp(L_DELETE_BLUE_PORTAL);
		//		injectors.push_back(lPortalFirst);
		//
		//		Injector* lDeleteBluePortal = new Injector{ L_DELETE_BLUE_PORTAL };
		//		injectors.push_back(lDeleteBluePortal);
		//		lDeleteBluePortal->movPtr(REG::EDX, 0x6a9ec0).movPtr(REG::EDX, REG::EDX, 0x768).call(0x41cad0)
		//			.addConst<byte>(0x84).addConst<byte>(0xc0) // test al,al
		//			.je(L_PORTAL)
		//			.movPtr(REG::ECX, REG::ESI).movPtr(REG::EAX, REG::ECX, 8).cmp(REG::EAX, 4).jne(L_DELETE_BLUE_PORTAL)
		//			.push(REG::ESI).mov(REG::ESI, REG::ECX).call(0x44d000)
		//			.pop(REG::ESI).jmp(L_PORTAL);
		//
		//		Injector* lPortal = new Injector{ L_PORTAL };
		//		lPortal->mov(REG::EAX, PORTAL_FLAG).ptrCmp(REG::EAX, 5).je(L_END)
		//			.movPtr(REG::EDI, 0x6a9ec0).movPtr(REG::EDI, REG::EDI, 0x768).movPtr(REG::EDI, REG::EDI, 0x160).call(0x426fc0)
		//			.mov(REG::EAX, PORTAL_FLAG).ptrMov(REG::EAX, 0, 5).jmp(L_END);
		//		injectors.push_back(lPortal);
		//
		//		Injector* lEnd = new Injector{ L_END };
		//		lEnd->popad().movPtr(REG::EBX, REG::EDX, 0x5560).jmp(L_PORTAL_START + 6);
		//		injectors.push_back(lEnd);
		//	}
		//
		//	if (std::find(zombieTypes.begin(), zombieTypes.end(), ZombieType::NewspaperZombie) != zombieTypes.end())
		//	{
		//
		//		constexpr uint32_t NO_SLOW_L_NOT = 0x6b0150;
		//
		//		Injector* newspaperNoSlowed1 = new Injector{ 0x52b448 };
		//		newspaperNoSlowed1->jmp(0x6b0100).nop().nop().nop();
		//		injectors.push_back(newspaperNoSlowed1);
		//		
		//		Injector* newspaperNoSlowed2 = new Injector{ 0x6b0100 };
		//		newspaperNoSlowed2->cmp(REG::EDI, 0x24, 0x5).jne(NO_SLOW_L_NOT).cmp(REG::EDI, 0x28, 0x1d).je(NO_SLOW_L_NOT)
		//			.cmp(REG::EDI, 0xac, 0).je(NO_SLOW_L_NOT)
		//			.ptrMov(REG::EDI, 0xac, 0).push(REG::EAX).push(REG::ECX).push(REG::EDX).call(0x52f050).pop(REG::EDX).pop(REG::ECX).pop(REG::EAX).jmp(0x52b451);
		//		injectors.push_back(newspaperNoSlowed2);
		//
		//		Injector* newspaperNoSlowed3 = new Injector{ NO_SLOW_L_NOT };
		//		newspaperNoSlowed3->add(REG::EAX, -1).ptrMov(REG::EDI, 0xac, REG::EAX).jmp(0x52b451);
		//		injectors.push_back(newspaperNoSlowed3);
		//
		//		constexpr uint32_t QUADRULPE_L_NOT = 0x6b0170;
		//
		//		Injector* newspaperQuadrupleEating1 = new Injector{ 0x52f689 };
		//		newspaperQuadrupleEating1->jmp(0x6b0200);
		//		injectors.push_back(newspaperQuadrupleEating1);
		//
		//		Injector* newspaperQuadrupleEating2 = new Injector{ 0x6b0200 };
		//		newspaperQuadrupleEating2->cmp(REG::EDI, 0x24, 5).jne(QUADRULPE_L_NOT).cmp(REG::EDI, 0x28, 0x1d).je(QUADRULPE_L_NOT)
		//			.pushad().mov(REG::EBX, REG::ECX).push(REG::EDI).call(0x52fb40).push(REG::EDI).mov(REG::ECX, REG::EBX).call(0x52fb40).push(REG::EDI).mov(REG::ECX, REG::EBX).call(0x52fb40).popad().jmp(QUADRULPE_L_NOT);
		//		injectors.push_back(newspaperQuadrupleEating2);
		//
		//		Injector* newspaperQuadrupleEating3 = new Injector{ QUADRULPE_L_NOT };
		//		newspaperQuadrupleEating3->call(0x52fb40).jmp(0x52f68e);
		//		injectors.push_back(newspaperQuadrupleEating3);
		//	}
		//
		//	if (std::find(zombieTypes.begin(), zombieTypes.end(), ZombieType::PogoZombie) != zombieTypes.end()
		//		|| std::find(zombieTypes.begin(), zombieTypes.end(), ZombieType::ZombieYeti) != zombieTypes.end()
		//		|| std::find(zombieTypes.begin(), zombieTypes.end(), ZombieType::JackintheboxZombie) != zombieTypes.end())
		//	{
		//		constexpr uint32_t L12 = 0x6b0300; // 跳跳
		//		constexpr uint32_t L15 = 0x6b0350; // 小丑
		//		constexpr uint32_t L19 = 0x6b0400; // 雪人
		//		constexpr uint32_t L_NOT = 0x6b0450;
		//		constexpr uint32_t L_EXPLODE = 0x6b0470;
		//		constexpr uint32_t L_NOT_DIXIAN = 0x6b0490;
		//		constexpr uint32_t L_NOT_ARRIVE = 0x6b0510;
		//		constexpr uint32_t L_SWITCH = 0x6b0530;
		//
		//		Injector* origin = new Injector{ 0x52afca };
		//		origin->jmp(L_SWITCH).nop().nop().nop();
		//		injectors.push_back(origin);
		//
		//		Injector* lSwitch = new Injector{ L_SWITCH };
		//		lSwitch->cmp(REG::ESI, 0x24, 18).je(L12)
		//			.cmp(REG::ESI, 0x24, 15).je(L15)
		//			.cmp(REG::ESI, 0x24, 19).je(L19)
		//			.jmp(L_NOT);
		//		injectors.push_back(lSwitch);
		//
		//		Injector* l12 = new Injector{ L12 };
		//		l12->cmp(REG::ESI, 0x8, -0x14).jg(L_NOT)
		//			.push(REG::EAX).push(REG::ECX).push(REG::EDX).push(0).push(REG::ESI).call(0x525350)
		//			.pop(REG::EDX).pop(REG::ECX).pop(REG::EAX).jmp(L_NOT);
		//		injectors.push_back(l12);
		//
		//		Injector* l15 = new Injector{ L15 };
		//		l15->cmp(REG::ESI, 0x68, 111).jb(L_NOT).bytePtrCmp(REG::ESI, 0xbb, 1).jne(L_EXPLODE)
		//			.bytePtrCmp(REG::ESI, 0x51, 1).jne(L_NOT_DIXIAN)
		//			.cmp(REG::ESI, 0x8, 100).jg(L_NOT_DIXIAN)
		//			.jmp(L_EXPLODE);
		//		injectors.push_back(l15);
		//
		//		Injector* lExplode = new Injector{ L_EXPLODE };
		//		lExplode->ptrMov(REG::ESI, 0x68, 1).jmp(L_NOT);
		//		injectors.push_back(lExplode);
		//
		//		Injector* lNotDixian = new Injector{ L_NOT_DIXIAN };
		//		lNotDixian->ptrMov(REG::ESI, 0x68, 2000).jmp(L_NOT);
		//		injectors.push_back(lNotDixian);
		//
		//		Injector* l19 = new Injector{ L19 }; 
		//		l19->cmp(REG::ESI, 0x8, 40).jg(L_NOT_ARRIVE).bytePtrMov(REG::ESI, 0xbc, 0).jmp(L_NOT);
		//		injectors.push_back(l19);
		//
		//		Injector* lNotArrive = new Injector{ L_NOT_ARRIVE };
		//		lNotArrive->ptrMov(REG::ESI, 0x68, 2000).jmp(L_NOT);
		//		injectors.push_back(lNotArrive);
		//
		//		Injector* lNot = new Injector{ L_NOT };
		//		lNot->ptrMov(REG::ESI, 0x8, REG::EAX).call(0x6397d0).jmp(0x52afd2);
		//		injectors.push_back(lNot);
		//	}

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

	// 整活特性更新函数，在布阵器更新循环内被调用，用于大小特性的实时检查
	static void featureUpdate(PVZ* pvz, const std::vector<SmallFeature>& smallFeatures, const SlotType& zombieTypes)
	{
		// 小特性有AVOID_FLOWERS时的处理
		if (std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::AVOID_FLOWERS) != smallFeatures.end())
		{
			if (pvz->Sun >= 25000)
			{
				// 失败,清阳光	
				iterZombies(pvz, [](PVZ* pvz, std::shared_ptr<Zombie> pZombie)
					{
						if (!pZombie->NotExist) pZombie->Remove();
					});
				pvz->Sun = 0;
			}
		}

		//大特性中对僵尸特性的处理
		bool hasFlagZombie = std::find(zombieTypes.begin(), zombieTypes.end(), ZombieType::FlagZombie) != zombieTypes.end();
		bool hasScreenDoorZombie = std::find(zombieTypes.begin(), zombieTypes.end(), ZombieType::ScreenDoorZombie) != zombieTypes.end();
		if (hasFlagZombie || hasScreenDoorZombie) {
			constexpr uint32_t IS_PLACED_ZOMBIE_FLAG_OFFSET = 0xf8;
			iterZombies(pvz, [hasFlagZombie, hasScreenDoorZombie](PVZ* pvz, std::shared_ptr<Zombie> pZombie)
				{
					if (hasFlagZombie)
					{
						if (pZombie->Type != ZombieType::FlagZombie || PVZ::Memory::ReadMemory<uint32_t>(pZombie->BaseAddress + IS_PLACED_ZOMBIE_FLAG_OFFSET) == 0x01) return;
						PVZ::Memory::WriteMemory<uint32_t>(pZombie->BaseAddress + IS_PLACED_ZOMBIE_FLAG_OFFSET, 0x01);
						auto oRow = pZombie->Row;
						byte col = (pZombie->X - 8) / 80;
						int upRow = oRow == 0 ? oRow : oRow - 1;
						int downRow = oRow == 4 ? oRow : oRow + 1;
						Creator::CreateZombie(ZombieType::Zombie, upRow, col);
						Creator::CreateZombie(ZombieType::Zombie, downRow, col);
					}
					if (hasScreenDoorZombie)
					{
						if (pZombie->Type != ZombieType::ScreenDoorZombie || PVZ::Memory::ReadMemory<uint32_t>(pZombie->BaseAddress + IS_PLACED_ZOMBIE_FLAG_OFFSET) == 0x01) return;
						PVZ::Memory::WriteMemory<uint32_t>(pZombie->BaseAddress + IS_PLACED_ZOMBIE_FLAG_OFFSET, 0x01);
						PVZ::Memory::WriteMemory<uint32_t>(pZombie->BaseAddress + 0xdc, 40 * 20); // 二类血量
					}
				});
		}

	}

	static int startGame(const std::array<Theme, 25>& themes, const std::array<size_t, 25>& seeds, const std::vector<SmallFeature>& smallFeatures, bool isCrashed = false)
	{//程序主要功能入口
		DWORD pid = ProcessOpener::Open(); // 寻找pvz
		if (!pid)
		{
			std::cout << "未找到pvz!" << std::endl;
			return 1;
		}

		if (!isCrashed) std::cout << "已找到pvz!" << std::endl;
		__pvz = new PVZ(pid);
		auto pvz = __pvz;
		EnableBackgroundRunning(true); // 启用pvz后台运行

		int currentFlag = -1; // ?
		// 先跳到第一关
		pvz->GetMiscellaneous()->Round = 0;

		auto currentAddress = pvz->BaseAddress;
		TimeStruct startTime = TimeStruct::getNow(); // 游戏开始时间
		TimeStruct eatLastBrainTime = TimeStruct::getNow();

		game_start_timesecond = std::time(nullptr);

		double currentScore = 0.0;
		bool hasStarted = isCrashed;


		const bool hasFeature = std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::NIL) == smallFeatures.end();
		// ize最低阳光
		const int lowestSun = std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::MINI_ZOMBIE) != smallFeatures.end() ? 20 : 50;
		bool isPaused = false;
		auto pauseTime = TimeStruct::getNow();

		// 记录已经处理过的僵尸
		std::unordered_set<int> processed_zombie_ids;
		int level_using_sun = 0;


		if (isCrashed)
		{
			pvz->GetMiscellaneous()->Round = levelState->accomplishedFlagNo;
			currentFlag = levelState->accomplishedFlagNo - 1;
			pvz->Sun = levelState->sunNo;
			startTime = TimeStruct::getNow() - levelState->currentTime;
			eatLastBrainTime = TimeStruct::getNow() - levelState->currentTime;
			slotZombieTypes = levelState->zombieTypes;
		}

		GameControl::setInjectors(slotZombieTypes, smallFeatures);

		while (true)
		{
			// 跨关时更新内容
			do {
				if (!pvz->BaseAddress || currentAddress != pvz->BaseAddress || pvz->GetMiscellaneous()->Round == currentFlag || pvz->GameState != PVZGameState::Playing) continue;
				if (pvz->GetMiscellaneous()->Round >= 25)// 通关条件
				{
					std::cout << std::string(ConsoleControl::get_terminal_width(), '-') << std::endl;
					std::cout << std::string("恭喜打通!!!!") << std::endl;
					std::cout << "最后吃脑时间为: " << (eatLastBrainTime - startTime).cnPrint() << std::endl;

					auto timeStr = std::string("Congrats!  ").append((eatLastBrainTime - startTime).enPrint());
					Creator::CreateCaption(timeStr.c_str(), timeStr.size(), CaptionStyle::Lowermiddle);

					return 0;
				}
				currentFlag = pvz->GetMiscellaneous()->Round; //当前关数
				Sleep(1);

				if (levelState)
				{
					levelState->accomplishedFlagNo = currentFlag;
					levelState->sunNo = pvz->Sun;
					levelState->currentTime = TimeStruct::getNow() - startTime;

				}
				GameControl::setLayout(pvz, currentFlag, themes[currentFlag], seeds[currentFlag], smallFeatures); // 按照种子生成植物
				if (!hasStarted) continue;
				std::cout << (TimeStruct::getNow() - startTime).enPrint()
					<< " 已经通过"
					<< currentFlag
					<< "关, 阳光"
					<< pvz->Sun
					<< "，花费"
					<< level_using_sun
					<< std::endl;

				level_using_sun = 0;

			} while (0);
			// 监测放置的僵尸以及计算每关阳光花费
			do {
				// 假设最大僵尸数量为100
				SPT<PVZ::Zombie> zombies[100];
				int zombieCount = pvz->GetAllZombies(zombies);

				std::unordered_set<int> alive_zombie_ids; // 记录当前仍然存在的僵尸

				for (int i = 0; i < zombieCount; i++)
				{
					// 僵尸活着
					if (!zombies[i]->NotExist) {
						// 插入存活列表
						alive_zombie_ids.insert(zombies[i]->Id);

						// 如果是刚放置的僵尸
						if (zombies[i]->ExistedTime >= 1 && zombies[i]->ExistedTime <= 5) {
							// 没有处理过这个僵尸
							if (!processed_zombie_ids.count(zombies[i]->Id)) {
								// 现在正在处理
								processed_zombie_ids.insert(zombies[i]->Id);
								// 判断是否为ize中的僵尸
								if (!ZombieSunCost.count(zombies[i]->Type) && currentFlag != -1 && pvz->GetMiscellaneous()->Round != 0) { // 重开的话会释放几个非ize僵尸
									std::cout << "检测到放置了非ize关卡的僵尸" << std::endl;
									continue;
								}
								// 是ize中的
								if (zombies[i]->Type != ZombieType::BackupDancer) {
									level_using_sun += ZombieSunCost[zombies[i]->Type].first;
									// std::cout << "在" << zombies[i]->Row + 1 << "行放置了" << ZombieSunCost[zombies[i]->Type].second << std::endl;
								}
								else {
									//TODO:如何检测单伴舞，而不是舞王召唤出来的
								}
							}
							else { // 处理过了

							}
						}
					}
					else {
						// 死了的不用处理
					};
				}
			} while (0);
			// 监测磁力菇时间
			//if (pvz->PlantsCount > 0)
			//{
			//	std::cout << "====== 磁力蘑菇状态 (0.1秒刷新) ======\n";

			//	// 简化后的lambda表达式
			//	iterPlants(pvz, [](PVZ* pvz, std::shared_ptr<Plant> pPlant)
			//		{
			//			if (!pPlant->NotExist && pPlant->Type == 31)
			//			{
			//				std::cout << "位置: "
			//					<< std::setw(2) << pPlant->Row + 1 << "-"
			//					<< std::setw(2) << pPlant->Column + 1
			//					<< " | 倒计时: "
			//					<< std::fixed << std::setprecision(2)
			//					<< pPlant->AttributeCountdown / 100.0f
			//					<< "s\n";
			//			}
			//		}, false);  // 保持顺序遍历
			//}

			// 监测每关的反应时间，在levelstate中添加新项目，并且监测到放置第一个僵尸之后，记录反应时间


			// 重开时更新内容
			do {
				if (pvz->BaseAddress == 0 || currentAddress == pvz->BaseAddress || pvz->GameState != PVZGameState::Playing) continue;
				std::cout << currentAddress << "  " << pvz->BaseAddress;
				currentAddress = pvz->BaseAddress;
				currentFlag = -1; // 变成从头开始1
				Sleep(5);
				std::cout << "重开了" << std::endl;
			} while (0);

			// 还没开始运行，进行初始化
			if (!hasStarted)
			{
				// 以下是检测是否处于游戏开始状态
				// (1) 进入pvz，在ize中，并且开始游戏
				if (!pvz->BaseAddress || pvz->LevelId != PVZLevel::I_Zombie_Endless || pvz->GameState != PVZGameState::Playing) continue;
				// (2) 释放了僵尸
				if (pvz->ZombiesCount != 1) continue;

				hasStarted = true;
				startTime = TimeStruct::getNow();
				eatLastBrainTime = TimeStruct::getNow();

				std::cout << "开始游戏! " << " 现实时间为: " << TimeStruct::getCurrentTime() << std::endl;
				std::cout << std::string(ConsoleControl::get_terminal_width(), '-') << std::endl;


				// 按照要求生成关卡
				levelState = new LevelState{ 150, 0, slotZombieTypes, smallFeatures, seeds, themes, {0, 0} };
				continue;
			}

			if (_kbhit()) // shift R
			{
				char c = _getch();
				if (c == 'R')
				{
					currentFlag = pvz->GetMiscellaneous()->Round;
					// 删除场上没收集的阳光：删了，在标记消失
					iterCoins(pvz, [](PVZ* pvz, std::shared_ptr<PVZ::Coin> pCoin)
						{
							if (pCoin->Type == CoinType::NormalSun) {
								pCoin->Collected = false;
								pCoin->NotExist = true;
							}
						});
					//恢复存档阳光
					pvz->Sun = levelState->sunNo;
					level_using_sun = 0;
					GameControl::setLayout(pvz, currentFlag, themes[currentFlag], seeds[currentFlag], smallFeatures);

					iterZombies(pvz, [](PVZ* pvz, std::shared_ptr<Zombie> pZombie)
						{
							pZombie->Remove(); // 清除僵尸
						});
					std::cout << (TimeStruct::getNow() - startTime).enPrint().append(" 重开本关!") << std::endl;


					// TODO: 更改时间
					std::string restart_str = std::string("Restart");
					Creator::CreateCaption(restart_str.c_str(), restart_str.size(), CaptionStyle::Lowermiddle); // 游戏白字，处于靠下居中位置
					continue;
				}
				if (c == 'P')
				{
					if (isPaused)
					{
						startTime = TimeStruct::getNow() + startTime - pauseTime;
						eatLastBrainTime = TimeStruct::getNow() + eatLastBrainTime - pauseTime;
						std::cout << (TimeStruct::getNow() - startTime).enPrint() << " 重新开始计时!" << std::endl;
						Creator::CreateCaption("CONTINUE", 8, CaptionStyle::Lowermiddle);
					}
					else
					{
						pauseTime = TimeStruct::getNow();
						std::cout << (pauseTime - startTime).enPrint() << " 暂停计时!" << std::endl;
						Creator::CreateCaption("PAUSE", 5, CaptionStyle::Lowermiddle);
					}
					isPaused = !isPaused;
					continue;
				}
				if (c == 'Q')
				{
					auto over_time = TimeStruct::getNow() - startTime;
					std::cout << over_time.enPrint().append(" 强制结束游戏! 还剩 ") << (TimeStruct(30 * 60) - over_time).enPrint() << std::endl;
					std::cout << std::string(ConsoleControl::get_terminal_width(), '-') << std::endl;
					std::cout << "游戏结束!, 最终得分为——  " << std::fixed << std::setprecision(1) << pvz->GetMiscellaneous()->Round + GameControl::countEatenBrain() * 0.2 << std::endl;
					std::cout << "最后吃脑时间为: " << (eatLastBrainTime - startTime).cnPrint() << std::endl;
					auto timeStr = (eatLastBrainTime - startTime).enPrint().append("     ").append(std::to_string(pvz->GetMiscellaneous()->Round + GameControl::countEatenBrain() * 0.2));
					Creator::CreateCaption(timeStr.c_str(), timeStr.size() - 5, CaptionStyle::Lowermiddle);
					return 0;
				}
			}

			if (!ProcessOpener::Open())
			{
				std::cout << "游戏关闭!" << std::endl;
				__pvz = nullptr;
				return 2;
			}

			// 整活特性更新函数
			if (hasFeature)
			{
				GameControl::featureUpdate(pvz, smallFeatures, slotZombieTypes);
			}

			// 吃脑时更新内容
			do
			{
				if (!pvz->BaseAddress || currentScore == currentFlag + GameControl::countEatenBrain() * 0.2) continue;
				currentScore = pvz->GetMiscellaneous()->Round + GameControl::countEatenBrain() * 0.2; // pvz->GetMiscellaneous()->Round： 现在是第几关
				eatLastBrainTime = TimeStruct::getNow();
			} while (0);

			// 超时
			if ((TimeStruct::getNow() - startTime).minute >= 30)
			{
				auto timeStr = (eatLastBrainTime - startTime).enPrint().append("     ").append(std::to_string(pvz->GetMiscellaneous()->Round + GameControl::countEatenBrain() * 0.2));
				Creator::CreateCaption(timeStr.c_str(), timeStr.size() - 5, CaptionStyle::Lowermiddle);
				std::cout << "游戏结束!, 最终得分为——  " << std::fixed << std::setprecision(1) << pvz->GetMiscellaneous()->Round + GameControl::countEatenBrain() * 0.2 << std::endl;
				std::cout << "最后吃脑时间为: " << (eatLastBrainTime - startTime).cnPrint() << std::endl;
				return 0;
			}
			Sleep(1);

			// 死亡：僵尸都死完了，并且没阳光放置
			if (pvz->BaseAddress && pvz->ZombiesCount == 0 && pvz->Sun < lowestSun)
			{
				bool isDead = true;
				iterCoins(pvz, [&isDead](PVZ* pvz, std::shared_ptr<PVZ::Coin> pCoin)
					{
						if (pCoin->Type == CoinType::NormalSun) isDead = false;
					});
				if (isDead)
				{
					auto timeStr = (eatLastBrainTime - startTime).enPrint().append("     ").append(std::to_string(pvz->GetMiscellaneous()->Round + GameControl::countEatenBrain() * 0.2));
					Creator::CreateCaption(timeStr.c_str(), timeStr.size() - 5, CaptionStyle::Lowermiddle); // 去掉5个字符
				}
			}
		}
		return 0;
	}

	static void exitGame(PVZ* pvz, const SlotType& zombieTypes, const std::vector<SmallFeature>& smallFeatures)
	{
		const int BASE_ADDRESS = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768);


		if (std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::LEFT_MOVE) != smallFeatures.end())
		{
			PVZ::Memory::WriteMemory<int>(BASE_ADDRESS + 0x63c, 0);
		}
		// 小特性有AVOID_FLOWERS时的处理
		if (std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::AVOID_FLOWERS) != smallFeatures.end())
		{
			PVZ::Memory::WriteMemory<byte>(0x41ba74, 0x29);
			PVZ::Memory::WriteMemory<byte>(0x41ba75, 0xDE);
			AutoCollect(pvz, false);
		}

		// 小特性有MINI_ZOMBIE时的处理
		if (std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::MINI_ZOMBIE) != smallFeatures.end())
		{
			PVZ::Memory::WriteMemory<int>(0x467b60, 50);
			PVZ::Memory::WriteMemory<int>(0x467b60 + 6 * 1, 75);
			PVZ::Memory::WriteMemory<int>(0x467b60 + 6 * 2, 350);
			PVZ::Memory::WriteMemory<int>(0x467b60 + 6 * 3, 125);
			PVZ::Memory::WriteMemory<int>(0x467b60 + 6 * 4, 150);
			PVZ::Memory::WriteMemory<int>(0x467b60 + 6 * 5, 175);
		}

		if (std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::TELEPORT) != smallFeatures.end())
		{
			EnablePortal(pvz, false);
			FixPortal(false);
		}

		for (auto& it : injectors)
		{
			if (it) delete it;
			it = 0;
		}

		slotZombieTypes = { ZombieType::Imp, // ZombieType::Zombie
		ZombieType::ConeheadZombie, ZombieType::PoleVaultingZombie, // ZombieType::ZombieYeti, ZombieType::FlagZombie, ZombieType::NewspaperZombie
		ZombieType::BucketheadZombie, ZombieType::BungeeZombie, // ZombieType::ScreenDoorZombie, ZombieType::JackintheboxZombie
		ZombieType::DiggerZombie, ZombieType::LadderZombie, ZombieType::FootballZombie, // ZombieType::PogoZombie
		ZombieType::DancingZombie };
	}

	static void crashSolution()
	{
		std::cout << std::string(ConsoleControl::get_terminal_width(), '*') << std::endl;
		std::cout << "游戏异常退出" << std::endl;
		std::cout << "此时您通过了"
			<< levelState->accomplishedFlagNo
			<< "关, 用时"
			<< levelState->currentTime.cnPrint()
			<< ", 阳光"
			<< levelState->sunNo << std::endl;
		std::cout << "正在等待重新进入游戏..." << std::endl;
		std::cout << std::string(ConsoleControl::get_terminal_width(), '*') << std::endl;

		while (!ProcessOpener::Open()) { Sleep(1); };
		DWORD pid = ProcessOpener::Open();
		__pvz = new PVZ(pid);
		std::cout << "请进入ize并Restart" << std::endl;
		while (!(__pvz->BaseAddress && __pvz->LevelId == PVZLevel::I_Zombie_Endless)) { Sleep(1); }
		std::cout << "按s开始游戏" << std::endl;
		std::cout << std::string(ConsoleControl::get_terminal_width(), '*') << std::endl;

		while (1)
		{
			Sleep(1);
			if (_kbhit() && _getch() == 's')
			{
				int i = startGame(levelState->themes, levelState->seeds, levelState->smallFeatures, true);
				if (i == 2)
				{
					crashSolution();
				}
				else if (i != 0)
				{
					std::cout << "可能出错了!, 出错代码" << i << std::endl;
				}
				exitGame(__pvz, slotZombieTypes, levelState->smallFeatures);
				return;
			}
		}
	}

};


















void setCards(SlotType& zombies, PVZ* pvz)
{
	pvz->GetCardSlot()->SetCardsCount(zombies.size());
	for (size_t i = 0; i < zombies.size(); i++)
	{
		auto it = zombies[i];
		auto pSlot = pvz->GetCardSlot()->GetCard(i);
		switch (it)
		{
		case ZombieType::Zombie:
			pSlot->ContentCard = CardType::Zombie;
			break;
		case ZombieType::FlagZombie:
			pSlot->ContentCard = CardType::Sunflower;
			break;
		case ZombieType::ConeheadZombie:
			pSlot->ContentCard = CardType::ConeheadZombie;
			break;
		case ZombieType::PoleVaultingZombie:
			pSlot->ContentCard = CardType::PoleVaultingZombie;
			break;
		case ZombieType::BucketheadZombie:
			pSlot->ContentCard = CardType::BucketheadZombie;
			break;
		case ZombieType::NewspaperZombie:
			pSlot->ContentCard = CardType::SnowPea;
			break;
		case ZombieType::ScreenDoorZombie:
			pSlot->ContentCard = CardType::ScreenDoorZombie;
			break;
		case ZombieType::FootballZombie:
			pSlot->ContentCard = CardType::FootballZombie;
			break;
		case ZombieType::DancingZombie:
			pSlot->ContentCard = CardType::DancingZombie;
			break;
		case ZombieType::JackintheboxZombie:
			pSlot->ContentCard = CardType::PogoZombie;
			break;
		case ZombieType::DiggerZombie:
			pSlot->ContentCard = CardType::DiggerZombie;
			break;
		case ZombieType::PogoZombie:
			pSlot->ContentCard = CardType::Gigagargantuar;
			break;
		case ZombieType::ZombieYeti:
			pSlot->ContentCard = CardType::TangleKelp;
			break;
		case ZombieType::BungeeZombie:
			pSlot->ContentCard = CardType::BungeeZombie;
			break;
		case ZombieType::LadderZombie:
			pSlot->ContentCard = CardType::LadderZombie;
			break;
		case ZombieType::Imp:
			pSlot->ContentCard = CardType::Imp;
			break;
		default:
			break;
		}
	}
}









int main()
{
	ConsoleControl console_controler;
	console_controler.main();
	return 0;
}
