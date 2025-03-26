#include "pvzclass.h"
#include <conio.h>


// variables
std::deque<uint32_t> pEffects{}; // 所有的被创建的particleSystem对象的指针
constexpr size_t BASE = 66;
constexpr uint32_t PORTAL_FLAG = 0x6b0900;
auto enterTimes = std::vector<TimeStruct>(25, TimeStruct::getNow());
SlotType slotZombieTypes = {ZombieType::Imp, // ZombieType::Zombie
	ZombieType::ConeheadZombie, ZombieType::PoleVaultingZombie, // ZombieType::ZombieYeti, ZombieType::FlagZombie, ZombieType::NewspaperZombie
	ZombieType::BucketheadZombie, ZombieType::BungeeZombie, // ZombieType::ScreenDoorZombie, ZombieType::JackintheboxZombie
	ZombieType::DiggerZombie, ZombieType::LadderZombie, ZombieType::FootballZombie, // ZombieType::PogoZombie
	ZombieType::DancingZombie
};
// 初始值为游戏卡槽, 每一行内是一个随机池子

std::vector<Injector*> injectors = {};

constexpr std::array<std::array<ZombieType::ZombieType, 5>, 4> pool = {{ 
	{ ZombieType::Imp, ZombieType::Zombie, ZombieType::None, ZombieType::None,ZombieType::None },
	{ ZombieType::ConeheadZombie, ZombieType::PoleVaultingZombie, ZombieType::ZombieYeti, ZombieType::FlagZombie, ZombieType::NewspaperZombie },
	{ ZombieType::BucketheadZombie, ZombieType::BungeeZombie, ZombieType::ScreenDoorZombie, ZombieType::JackintheboxZombie, ZombieType::None },
	{ ZombieType::DiggerZombie, ZombieType::LadderZombie, ZombieType::FootballZombie, ZombieType::PogoZombie, ZombieType::None } }};

constexpr std::array<int, 4> poolLimit = { 1, 3, 2, 3 };  // 四个池子分别抽取的个数
constexpr std::array<int, 4> poolMax = { 2, 5, 4, 4 };
std::random_device rd;
std::mt19937_64 gen(rd()); // 全局随机数生成器

constexpr std::array<const char*, 7> SMALL_FEATURE_NAMES = {"【迷你】", "【避花】", "【缝合】", "【传送】", "【大蒜】", "【左移】", "【无特性】"};
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
*****************************************************\n\
欢迎使用第六届手速杯布阵器 = v = \n\
作者: 碳酸 天盟琉璃; 手速杯交流群：1157197563\n\
请在输入功能对应序号后按回车键：\n\
1：使用阵型代码生成阵型\n\
2：生成常规代码\n\
3：生成整活代码\n\
4：整活特性介绍\n\
5：使用说明\n\
0：退出\n\
*****************************************************";
constexpr bool extraFeatures = true;

constexpr wchar_t WINDOW_NAME[] = L"第六届手速杯布阵器";

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

PVZ* __pvz = nullptr; // 全局pvz对象
LevelState* levelState = nullptr;// 全局 切关时游戏状态保存

#define pt PlantType
constexpr std::array<pt::pt, 17> getThemePlantTypes(Theme theme)
{
	switch (theme)
	{
	case Theme::COMPOSITE:
		return {
			pt::Wallnut,
			pt::Torchwood,
			pt::PotatoMine,
			pt::Chomper, pt::Chomper,
			pt::Peashooter,
			pt::SplitPea,
			pt::Kernelpult,
			pt::Threepeater,
			pt::SnowPea,
			pt::Squash,
			pt::Fumeshroom,
			pt::UmbrellaLeaf,
			pt::Starfruit,
			pt::Magnetshroom,
			pt::Spickweed, pt::Spickweed
		};
		break;
	case Theme::CONTROL:
		return {
			pt::Torchwood,
			pt::SplitPea, pt::SplitPea, pt::SplitPea,
			pt::Repeater,
			pt::Kernelpult, pt::Kernelpult, pt::Kernelpult,
			pt::Threepeater,
			pt::SnowPea, pt::SnowPea, pt::SnowPea,
			pt::UmbrellaLeaf,
			pt::Magnetshroom,
			pt::Spickweed, pt::Spickweed, pt::Spickweed
		};	
		break;
	case Theme::INSTANT_KILL:
		return {
			pt::PotatoMine, pt::PotatoMine, pt::PotatoMine, pt::PotatoMine,
			pt::Chomper, pt::Chomper, pt::Chomper,
			pt::Squash, pt::Squash, pt::Squash,
			pt::Fumeshroom, pt::Fumeshroom, pt::Fumeshroom, pt::Fumeshroom,
			pt::Spickweed, pt::Spickweed, pt::Spickweed
		};
		break;
	case Theme::PEAS:
		return {
			pt::SnowPea, pt::SnowPea, pt::SnowPea, pt::SnowPea, pt::SnowPea, pt::SnowPea, pt::SnowPea, pt::SnowPea, pt::SnowPea,
			pt::SplitPea, pt::SplitPea, pt::SplitPea, pt::SplitPea,
			pt::Repeater, pt::Repeater, pt::Repeater, pt::Repeater
		};
		break;
	case Theme::STAR_AND_SPIKE:
		return {
			pt::Spickweed, pt::Spickweed, pt::Spickweed, pt::Spickweed, pt::Spickweed, pt::Spickweed, pt::Spickweed, pt::Spickweed, pt::Spickweed,
			pt::Starfruit, pt::Starfruit, pt::Starfruit, pt::Starfruit, pt::Starfruit, pt::Starfruit, pt::Starfruit,pt::Starfruit
		};
		break;
	case Theme::EXPLODING:
		return {
			pt::PotatoMine, pt::PotatoMine, pt::PotatoMine,pt::PotatoMine, pt::PotatoMine,pt::PotatoMine, pt::PotatoMine,pt::PotatoMine, pt::PotatoMine,
			pt::Chomper, pt::Chomper, pt::Chomper, pt::Chomper, pt::Chomper, pt::Chomper, pt::Chomper, pt::Chomper
		};
		break;
	case Theme::MAGNAT_AND_FUME:
		return {	
			pt::Fumeshroom, pt::Fumeshroom, pt::Fumeshroom, pt::Fumeshroom, pt::Fumeshroom, pt::Fumeshroom, pt::Fumeshroom, pt::Fumeshroom, pt::Fumeshroom,
			pt::Magnetshroom, pt::Magnetshroom, pt::Magnetshroom, pt::Magnetshroom, pt::Magnetshroom, pt::Magnetshroom, pt::Magnetshroom, pt::Magnetshroom
		};
		break;
	case Theme::SCARDY:
		return {
		    pt::Scaredyshroom, pt::Scaredyshroom, pt::Scaredyshroom, pt::Scaredyshroom, pt::Scaredyshroom, pt::Scaredyshroom, pt::Scaredyshroom, pt::Scaredyshroom, pt::Scaredyshroom, pt::Scaredyshroom, pt::Scaredyshroom, pt::Scaredyshroom,
			pt::Sunflower, pt::Sunflower, pt::Sunflower, pt::Sunflower, pt::Sunflower
		};
		break;
	default:
		return std::array<pt::pt, 17>();
		break;
	}
}
#undef pt

std::array<Theme, 25> generateThemeDistribution()
{
	// order of B theme.	
	std::array<Theme, 5> BOrder= {Theme::PEAS, Theme::STAR_AND_SPIKE, Theme::EXPLODING, Theme::MAGNAT_AND_FUME, Theme::SCARDY};
	while (BOrder[0] != Theme::SCARDY && BOrder[1] != Theme::SCARDY) std::shuffle(BOrder.begin(), BOrder.end(), gen);

	// flags of B themes.
	std::uniform_int_distribution<unsigned int> d1(1, 4);
	std::uniform_int_distribution<unsigned int> d2(0, 4);
	std::array<int, 5> tmp1 = {d1(gen), d2(gen), d2(gen), d2(gen), d2(gen)};

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

inline std::pair<int, int> intToPos(int i)
{
	return {i / 5, i % 5};
}

int setFirstPhasePlants(std::array<PlantType::PlantType, 25>& arr, int flowerNumber)
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

std::array<PlantType::PlantType, 25> getFlagPlantTypes(int flag, Theme theme)
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

size_t generateSeed(Theme theme)
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

std::array<int, 25> getShuffledArray(size_t seed)
{
	std::array<int, 25> arr = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 , 20, 21, 22, 23, 24 };
	std::mt19937_64 gen(seed);
	std::shuffle(arr.begin(), arr.end(), gen);
	return arr;
}

void setLayout(PVZ* pvz, int flag, Theme theme, size_t seed, const std::vector<SmallFeature>& smallFeatures)
{
	auto plantTypes = getFlagPlantTypes(flag, theme);
	auto orders = getShuffledArray(seed);
	std::array<bool, 2> garlicDirections = {0, 0}; // 向下0, 向上1
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
			setMixingLayout(plantTypes, seed, theme);
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

	const uint32_t BASE_ADDR = PVZ::Memory::ReadMemory<uint32_t>(0x6a9ec0);

	for (size_t i = 0; i < 25; i++)
	{
		int written[] = { intToPos(orders[i]).first , intToPos(orders[i]).second ,static_cast<int>(plantTypes[i]) };
		PVZ::Memory::WriteArray<int>(PLANT_MEMORY + 12 * i, written, 12);
	}
	Sleep(20);
	PVZ::Memory::WriteMemory<uint32_t>(PLANT_FLAG, 3);
	Sleep(20);

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
					Creator::CreateEffect(arrowType, posToFloat(pos).first + 20+180, posToFloat(pos).second - 10 + 200);
				}
				else Creator::CreateEffect(arrowType, posToFloat(pos).first - 10+180, posToFloat(pos).second - 80 + 200);

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
}

void iterPlants(PVZ* pvz,VoidLambda<Plant> func, bool reverseOrder)
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

// 对每一个seed做对BASE的进制转换到string, 首位加主题对应号码, 用'.'分隔
// 进制转换, 第1位为个位, 第2位为十位, 往上类推
std::string encodeLayoutString(const std::array<Theme, 25>& themes, const std::array<size_t, 25>& seeds)
{
	auto ret = std::string();

	for (size_t i = 0; i < 25; i++)
	{
		ret.append(std::to_string(static_cast<int>(themes[i])));
		auto t = seeds[i];
		while (t)
		{
			ret.append(1, numberToChar(t % BASE));
			t /= BASE;
		}
		if (i != 24) ret.append(1, '.');
	}
	return std::move(ret);
}

bool getFeaturedLayoutString(std::string& originalLayoutString, const SlotType& zombies, const std::vector<SmallFeature>& smallFeatures)
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

bool decodeLayoutString(std::string& layoutString, std::array<Theme, 25>& themes, std::array<size_t, 25>& seeds)
{
	auto ss = std::stringstream(layoutString);
	auto str = std::string();
	auto vec = std::vector<std::string>();

	while (getline(ss, str, '.'))
	{
		vec.push_back(str);
	}

	if (vec.size() <= 25) return false;

	for (size_t i = 0; i < 25; i++)
	{
		themes[i] = static_cast<Theme>(vec[i][0] - '0');

		seeds[i] = 0;
		for (size_t j = 1; j < vec[i].length(); j++)
		{
			seeds[i] += charToNumber(vec[i][j]) * stPow(BASE, j - 1);
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

int decodeFeatureString(std::string& layoutString, SlotType& zombies, std::vector<SmallFeature>& smallFeatures)
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

std::string generateLayoutString()
{
	auto themes1 = generateThemeDistribution();
	auto seeds1 = std::array<size_t, 25>();
	for (size_t i = 0; i < 25; i++)
	{
		seeds1[i] = generateSeed(themes1[i]);
	}
	return encodeLayoutString(themes1, seeds1);
}

int startGame(const std::array<Theme, 25>& themes, const std::array<size_t, 25>& seeds, const std::vector<SmallFeature>& smallFeatures, const bool isCrashed)
{
	DWORD pid = ProcessOpener::Open();
	if (!pid) 
	{
		std::cout << "未找到pvz!" << std::endl;
		return 1;
	}
	if(!isCrashed) std::cout << "已找到pvz!" << std::endl;
	__pvz = new PVZ(pid);
	auto pvz = __pvz;
	EnableBackgroundRunning(true);

	int currentFlag = -1;
	auto currentAddress = pvz->BaseAddress;
	TimeStruct startTime = TimeStruct::getNow();
	TimeStruct eatLastBrainTime = TimeStruct::getNow();
	double currentScore = 0.0;
	bool hasStarted = isCrashed;
	const bool hasFeature = std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::NIL) == smallFeatures.end();
	const int lowestSun = std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::MINI_ZOMBIE) != smallFeatures.end() ? 20 : 50;
	bool isPaused = false;
	auto pauseTime = TimeStruct::getNow();

	if (isCrashed)
	{
		pvz->GetMiscellaneous()->Round = levelState->accomplishedFlagNo;
		currentFlag = levelState->accomplishedFlagNo - 1;
		pvz->Sun = levelState->sunNo;
		startTime = TimeStruct::getNow() - levelState->currentTime;
		eatLastBrainTime = TimeStruct::getNow() - levelState->currentTime;
		slotZombieTypes = levelState->zombieTypes;
	}

	setInjectors(slotZombieTypes, smallFeatures);
	while (true)
	{
		// 跨关时更新内容
		do {
			if (!pvz->BaseAddress || currentAddress != pvz->BaseAddress || pvz->GetMiscellaneous()->Round == currentFlag || pvz->GameState != PVZGameState::Playing) continue;
			if (pvz->GetMiscellaneous()->Round >= 25)
			{
				std::cout << std::string("恭喜打通!!!!") << std::endl;
				std::cout << "最后吃脑时间为: " << (eatLastBrainTime - startTime).cnPrint() << std::endl;

				auto timeStr = std::string("Congrats!  ").append((eatLastBrainTime - startTime).enPrint());
				Creator::CreateCaption(timeStr.c_str(), timeStr.size(), CaptionStyle::Lowermiddle);

				return 0;
			}
			currentFlag = pvz->GetMiscellaneous()->Round;
			Sleep(1);

			if (levelState)
			{
				levelState->accomplishedFlagNo = currentFlag;
				levelState->sunNo = pvz->Sun;
				levelState->currentTime = TimeStruct::getNow() - startTime;
			}
			setLayout(pvz, currentFlag, themes[currentFlag], seeds[currentFlag], smallFeatures);
			if (!hasStarted) continue;
			std::cout << (TimeStruct::getNow() - startTime).enPrint()
				<< " 已经通过"
				<< currentFlag
				<< "关,  阳光"
				<< pvz->Sun << std::endl;
		} while (0);

		// 重开时更新内容
		do {
			if (pvz->BaseAddress == 0 || currentAddress == pvz->BaseAddress || pvz->GameState != PVZGameState::Playing) continue;
			currentAddress = pvz->BaseAddress;
			currentFlag = -1; // 变成从头开始
			Sleep(5);
		} while (0);

		// 初始化
		if (!hasStarted)
		{
			if (!pvz->BaseAddress || pvz->LevelId != PVZLevel::I_Zombie_Endless || pvz->GameState != PVZGameState::Playing) continue;
			if (pvz->ZombiesCount != 1) continue;
			hasStarted = true;
			startTime = TimeStruct::getNow();
			eatLastBrainTime = TimeStruct::getNow();
			std::cout << "开始游戏! " << std::endl;

			levelState = new LevelState{ 150, 0, slotZombieTypes, smallFeatures, seeds, themes, {0, 0} };
			continue;
		}

		if (_kbhit())
		{
			char c = _getch();
			if (c == 'R')
			{
				currentFlag = pvz->GetMiscellaneous()->Round;
				setLayout(pvz, currentFlag, themes[currentFlag], seeds[currentFlag], smallFeatures);
				iterZombies(pvz, [](PVZ* pvz, std::shared_ptr<Zombie> pZombie)
					{
						pZombie->Remove();
					});
				std::cout << "重开本关!" << std::endl;
				Creator::CreateCaption("RESTART", 7, CaptionStyle::Lowermiddle);
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
					std::cout << (pauseTime - startTime).enPrint() << " 暂停计时!"  << std::endl;
					Creator::CreateCaption("PAUSE", 5, CaptionStyle::Lowermiddle);
				}
				isPaused = !isPaused;
				continue;
			}
			if (c == 'Q')
			{
				std::cout << "强制结束游戏!" << std::endl;
				auto timeStr = (eatLastBrainTime - startTime).enPrint().append("     ").append(std::to_string(pvz->GetMiscellaneous()->Round + countEatenBrain(pvz) * 0.2));
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
			featureUpdate(pvz, smallFeatures, slotZombieTypes);
		}

		// 吃脑时更新内容
		do
		{
			if (!pvz->BaseAddress || currentScore == currentFlag + countEatenBrain(pvz) * 0.2) continue;
			currentScore = pvz->GetMiscellaneous()->Round + countEatenBrain(pvz) * 0.2;
			eatLastBrainTime = TimeStruct::getNow();
		} while (0);
				
		// 超时
		if ((TimeStruct::getNow() - startTime).minute >= 30)
		{
			auto timeStr = (eatLastBrainTime - startTime).enPrint().append("     ").append(std::to_string(pvz->GetMiscellaneous()->Round + countEatenBrain(pvz) * 0.2));
			Creator::CreateCaption(timeStr.c_str(), timeStr.size() - 5, CaptionStyle::Lowermiddle);
			std::cout << "游戏结束!, 最终得分为——  " << pvz->GetMiscellaneous()->Round + countEatenBrain(pvz) * 0.2 << std::endl;
			std::cout << "最后吃脑时间为: " << (eatLastBrainTime - startTime).cnPrint() << std::endl;
			return 0;
		}
		Sleep(1);

		// 死亡
		if (pvz->BaseAddress && pvz->ZombiesCount == 0 && pvz->Sun < lowestSun)
		{
			bool isDead = true;
			iterCoins(pvz, [&isDead](PVZ* pvz, std::shared_ptr<PVZ::Coin> pCoin)
				{
					if (pCoin->Type == CoinType::NormalSun) isDead = false;
				});
			if (isDead)
			{
				auto timeStr = (eatLastBrainTime - startTime).enPrint().append("     ").append(std::to_string(pvz->GetMiscellaneous()->Round + countEatenBrain(pvz) * 0.2));
				Creator::CreateCaption(timeStr.c_str(), timeStr.size() - 5, CaptionStyle::Lowermiddle);
			}
		}
	}
	return 0;
}

void copyToClipBoard(const std::string& str)
{
	auto hGlobalMemorry = GlobalAlloc(GPTR, static_cast<DWORD>(str.length()) + 1);
	auto hWnd = FindWindow(NULL, WINDOW_NAME);
	if (OpenClipboard(hWnd))
	{
		EmptyClipboard();
		strcpy_s(static_cast<char*>(hGlobalMemorry), str.length() + 1,str.c_str());
		SetClipboardData(CF_TEXT, hGlobalMemorry);
		CloseClipboard();
		return;
	}
	else return;
}

int countEatenBrain(PVZ* pvz)
{
	int ret = 5;
	if (!pvz->BaseAddress) return -1;
	iterGriditems(pvz, [&ret](PVZ* pvz, std::shared_ptr<PVZ::Griditem> pGriditem)
		{
			if (pGriditem == nullptr) return;
			if (pGriditem->Type == GriditemType::IZBrain) ret -= 1;
		});
	return ret;
}

// 整活特性更新函数，在布阵器更新循环内被调用，用于大小特性的实时检查
void featureUpdate(PVZ* pvz, const std::vector<SmallFeature>& smallFeatures, const SlotType& zombieTypes)
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

// 生成大特性僵尸 givenTypes输入各类索引+1, (或输入0表示无限制).
SlotType generateSlotTypes(std::array<std::vector<int>, 4> givenTypes)
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

std::vector<SmallFeature> generateSmallFeatures(int count, bool hasSpecialFeatures, std::vector<SmallFeature> givenFeatures)
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

bool setMixingLayout(std::array<PlantType::PlantType, 25>& plantTypes, size_t seed, Theme theme)
{
	std::mt19937_64 genMixing(seed + 1);

	std::uniform_int_distribution<unsigned int> rngTheme(1, 7); // [1..7], 为不是胆小的所有主题.
	auto extraTheme = static_cast<Theme>(rngTheme(genMixing));

	// 将两个主题的二期植物数组拼起来shuffle
	auto originalTypes = getThemePlantTypes(theme);
	auto vec = std::vector<PlantType::PlantType>(originalTypes.begin(), originalTypes.end());
	for (auto it : getThemePlantTypes(extraTheme)) vec.push_back(it);
	std::shuffle(vec.begin(), vec.end(), genMixing);

	// 把二期植物换成shuffle后的前17个植物
	for (size_t i = 0; i < 17; i++)
	{
		plantTypes[i + 8] = vec[i];
	}
	return true;
}

bool setTeleport(PVZ* pvz, size_t seed)
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

void exitGame(PVZ* pvz, const SlotType& zombieTypes, const std::vector<SmallFeature>& smallFeatures)
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

void setInjectors(const SlotType& zombieTypes, const std::vector<SmallFeature>& smallFeatures)
{
	bool isMiniZombie = std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::MINI_ZOMBIE) != smallFeatures.end();

	bool isFeatured = std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::NIL) != smallFeatures.end();


	Injector* allCardsAreZombieCard = new Injector{ 0x42b57b };
	allCardsAreZombieCard->addConst<byte>(0xb0).addConst<byte>(0x01); // mov al,01
	injectors.push_back(allCardsAreZombieCard);

	Injector* yetiSun = new Injector{ 0x0069F56C };
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
	injectors.push_back(screenDoorSun);

	Injector* disableMaidCheat = new Injector(0x52dfcb, { 0x68, 0x07, 0x00, 0x00, 0x8b, 0x80, 0x68, 0x55, 0x00, 0x00, 0x99, 0xf7, 0xf9, 0x8b, 0xc2, 0x99, 0xf7, 0xfe, 0x5e, 0xc3 });
	injectors.push_back(disableMaidCheat);

	Injector* noDroppingCoins = new Injector(0x51d79a,{0xb0, 0x01, 0x90});
	injectors.push_back(noDroppingCoins);

	Injector* disablePlantingEffect = new Injector{ 0x40ce60 };
	disablePlantingEffect->ret(0x0004);
	injectors.push_back(disablePlantingEffect);


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

	if (isMiniZombie)
	{
//		PVZ::Memory::WriteMemory<byte>(0x523ed5, 0xeb);
		Injector* miniZombieSwitch = new Injector(0x523ed5, { 0xeb });
		injectors.push_back(miniZombieSwitch);
		PVZ::Memory::WriteMemory<int>(0x467b60, 20);
		PVZ::Memory::WriteMemory<int>(0x467b60 + 6, 30);
		PVZ::Memory::WriteMemory<int>(0x467b60 + 6 * 2, 140);
		PVZ::Memory::WriteMemory<int>(0x467b60 + 6 * 3, 50);
		PVZ::Memory::WriteMemory<int>(0x467b60 + 6 * 4, 60);
		PVZ::Memory::WriteMemory<int>(0x467b60 + 6 * 5, 70);
	}

	if (std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::GARLICS) != smallFeatures.end())
	{
		Injector* garlics1 = new Injector{ 0x52fcf0 };
		(*garlics1).jmp(0x6b0000).nop().nop();
		injectors.push_back(garlics1);

		Injector* garlics2 = new Injector{ 0x6b0000 };
		(*garlics2).push(REG::EAX).mov(REG::EAX, -0x04).ptrAdd(REG::ESI, 0x40, REG::EAX).movPtr(REG::ECX, REG::ESI, 0x40).cmp(REG::ESI, 0x24, 36)
			.pop(REG::EAX).jne(0x52fcf7).push(REG::EAX).movPtr(REG::EAX, REG::ESI, 0x80).ptrMov(REG::EBP, 0x130, REG::EAX).pop(REG::EAX).jmp(0x52fcf7);
		injectors.push_back(garlics2);

		Injector* garlics3 = new Injector{ 0x52b902 };
		(*garlics3).push(REG::EAX).movPtr(REG::EAX, REG::EDI, 0x130).ptrAdd(REG::EDI, 0x1c, REG::EAX).pop(REG::EAX).jmp(0x52b91a);
		for (size_t i = 0; i < 11; i++)
		{
			garlics3->nop();
		}
		injectors.push_back(garlics3);
	}

	if (std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::LEFT_MOVE) != smallFeatures.end())
	{
		Injector* leftMove1 = new Injector{ 0x52af86 };
		(*leftMove1).jmp(0x6b0030);
		injectors.push_back(leftMove1);

		Injector* leftMove2 = new Injector{ 0x6b0030 };
		(*leftMove2).pushad().cmp(REG::ESI, 0x24, 0x11).je(0x6b0077)
			.movPtr(REG::EAX, 0x6a9ec0).movPtr(REG::EAX, REG::EAX, 0x768).cmp(REG::EAX, 0x63c, 0)
			.je(0x6b0077).fld(REG::ESI, 0x2c).cmp(REG::ESI, 0xac, 0).jg(0x6b006e).fsub(0x6b0098).jmp(0x6b0074)
			.fsub(0x6b009c).fstp(REG::ESI, 0x2c).popad().call(0x52BCA0).jmp(0x52af8b);
		injectors.push_back(leftMove2);

		Injector* leftMoveFloats = new Injector{ 0x6b0098 };
		(*leftMoveFloats).addConst<float>(0.1f).addConst(0.05f);
		injectors.push_back(leftMoveFloats);
	}

	if (std::find(smallFeatures.begin(), smallFeatures.end(), SmallFeature::TELEPORT) != smallFeatures.end())
	{
		constexpr uint32_t L_PORTAL_START = 0x42b34a;
		constexpr uint32_t L_PORTAL = 0x6b1850;
		constexpr uint32_t L_DELETE_BLUE_PORTAL = 0x6b1900;
		constexpr uint32_t L_PORTAL_FIRST = 0x6b1b00;
		constexpr uint32_t L_END = 0x6b1C00;

		Injector* lPortalStart = new Injector{ L_PORTAL_START };
		lPortalStart->jmp(L_PORTAL_FIRST);
		injectors.push_back(lPortalStart);

		Injector* lPortalFirst = new Injector{ L_PORTAL_FIRST };
		lPortalFirst->pushad().mov(REG::ESI, 0x6b0910).ptrMov(REG::ESI,0, 0).jmp(L_DELETE_BLUE_PORTAL);
		injectors.push_back(lPortalFirst);

		Injector* lDeleteBluePortal = new Injector{ L_DELETE_BLUE_PORTAL };
		injectors.push_back(lDeleteBluePortal);
		lDeleteBluePortal->movPtr(REG::EDX, 0x6a9ec0).movPtr(REG::EDX, REG::EDX, 0x768).call(0x41cad0)
			.addConst<byte>(0x84).addConst<byte>(0xc0) // test al,al
			.je(L_PORTAL)
			.movPtr(REG::ECX, REG::ESI).movPtr(REG::EAX, REG::ECX, 8).cmp(REG::EAX, 4).jne(L_DELETE_BLUE_PORTAL)
			.push(REG::ESI).mov(REG::ESI, REG::ECX).call(0x44d000)
			.pop(REG::ESI).jmp(L_PORTAL);

		Injector* lPortal = new Injector{ L_PORTAL };
		lPortal->mov(REG::EAX, PORTAL_FLAG).ptrCmp(REG::EAX, 5).je(L_END)
			.movPtr(REG::EDI, 0x6a9ec0).movPtr(REG::EDI, REG::EDI, 0x768).movPtr(REG::EDI, REG::EDI, 0x160).call(0x426fc0)
			.mov(REG::EAX, PORTAL_FLAG).ptrMov(REG::EAX, 0, 5).jmp(L_END);
		injectors.push_back(lPortal);

		Injector* lEnd = new Injector{ L_END };
		lEnd->popad().movPtr(REG::EBX, REG::EDX, 0x5560).jmp(L_PORTAL_START + 6);
		injectors.push_back(lEnd);
	}

	if (std::find(zombieTypes.begin(), zombieTypes.end(), ZombieType::NewspaperZombie) != zombieTypes.end())
	{

		constexpr uint32_t NO_SLOW_L_NOT = 0x6b0150;

		Injector* newspaperNoSlowed1 = new Injector{ 0x52b448 };
		newspaperNoSlowed1->jmp(0x6b0100).nop().nop().nop();
		injectors.push_back(newspaperNoSlowed1);
		
		Injector* newspaperNoSlowed2 = new Injector{ 0x6b0100 };
		newspaperNoSlowed2->cmp(REG::EDI, 0x24, 0x5).jne(NO_SLOW_L_NOT).cmp(REG::EDI, 0x28, 0x1d).je(NO_SLOW_L_NOT)
			.cmp(REG::EDI, 0xac, 0).je(NO_SLOW_L_NOT)
			.ptrMov(REG::EDI, 0xac, 0).push(REG::EAX).push(REG::ECX).push(REG::EDX).call(0x52f050).pop(REG::EDX).pop(REG::ECX).pop(REG::EAX).jmp(0x52b451);
		injectors.push_back(newspaperNoSlowed2);

		Injector* newspaperNoSlowed3 = new Injector{ NO_SLOW_L_NOT };
		newspaperNoSlowed3->add(REG::EAX, -1).ptrMov(REG::EDI, 0xac, REG::EAX).jmp(0x52b451);
		injectors.push_back(newspaperNoSlowed3);

		constexpr uint32_t QUADRULPE_L_NOT = 0x6b0170;

		Injector* newspaperQuadrupleEating1 = new Injector{ 0x52f689 };
		newspaperQuadrupleEating1->jmp(0x6b0200);
		injectors.push_back(newspaperQuadrupleEating1);

		Injector* newspaperQuadrupleEating2 = new Injector{ 0x6b0200 };
		newspaperQuadrupleEating2->cmp(REG::EDI, 0x24, 5).jne(QUADRULPE_L_NOT).cmp(REG::EDI, 0x28, 0x1d).je(QUADRULPE_L_NOT)
			.pushad().mov(REG::EBX, REG::ECX).push(REG::EDI).call(0x52fb40).push(REG::EDI).mov(REG::ECX, REG::EBX).call(0x52fb40).push(REG::EDI).mov(REG::ECX, REG::EBX).call(0x52fb40).popad().jmp(QUADRULPE_L_NOT);
		injectors.push_back(newspaperQuadrupleEating2);

		Injector* newspaperQuadrupleEating3 = new Injector{ QUADRULPE_L_NOT };
		newspaperQuadrupleEating3->call(0x52fb40).jmp(0x52f68e);
		injectors.push_back(newspaperQuadrupleEating3);
	}

	if (std::find(zombieTypes.begin(), zombieTypes.end(), ZombieType::PogoZombie) != zombieTypes.end()
		|| std::find(zombieTypes.begin(), zombieTypes.end(), ZombieType::ZombieYeti) != zombieTypes.end()
		|| std::find(zombieTypes.begin(), zombieTypes.end(), ZombieType::JackintheboxZombie) != zombieTypes.end())
	{
		constexpr uint32_t L12 = 0x6b0300; // 跳跳
		constexpr uint32_t L15 = 0x6b0350; // 小丑
		constexpr uint32_t L19 = 0x6b0400; // 雪人
		constexpr uint32_t L_NOT = 0x6b0450;
		constexpr uint32_t L_EXPLODE = 0x6b0470;
		constexpr uint32_t L_NOT_DIXIAN = 0x6b0490;
		constexpr uint32_t L_NOT_ARRIVE = 0x6b0510;
		constexpr uint32_t L_SWITCH = 0x6b0530;

		Injector* origin = new Injector{ 0x52afca };
		origin->jmp(L_SWITCH).nop().nop().nop();
		injectors.push_back(origin);

		Injector* lSwitch = new Injector{ L_SWITCH };
		lSwitch->cmp(REG::ESI, 0x24, 18).je(L12)
			.cmp(REG::ESI, 0x24, 15).je(L15)
			.cmp(REG::ESI, 0x24, 19).je(L19)
			.jmp(L_NOT);
		injectors.push_back(lSwitch);

		Injector* l12 = new Injector{ L12 };
		l12->cmp(REG::ESI, 0x8, -0x14).jg(L_NOT)
			.push(REG::EAX).push(REG::ECX).push(REG::EDX).push(0).push(REG::ESI).call(0x525350)
			.pop(REG::EDX).pop(REG::ECX).pop(REG::EAX).jmp(L_NOT);
		injectors.push_back(l12);

		Injector* l15 = new Injector{ L15 };
		l15->cmp(REG::ESI, 0x68, 111).jb(L_NOT).bytePtrCmp(REG::ESI, 0xbb, 1).jne(L_EXPLODE)
			.bytePtrCmp(REG::ESI, 0x51, 1).jne(L_NOT_DIXIAN)
			.cmp(REG::ESI, 0x8, 100).jg(L_NOT_DIXIAN)
			.jmp(L_EXPLODE);
		injectors.push_back(l15);

		Injector* lExplode = new Injector{ L_EXPLODE };
		lExplode->ptrMov(REG::ESI, 0x68, 1).jmp(L_NOT);
		injectors.push_back(lExplode);

		Injector* lNotDixian = new Injector{ L_NOT_DIXIAN };
		lNotDixian->ptrMov(REG::ESI, 0x68, 2000).jmp(L_NOT);
		injectors.push_back(lNotDixian);

		Injector* l19 = new Injector{ L19 }; 
		l19->cmp(REG::ESI, 0x8, 40).jg(L_NOT_ARRIVE).bytePtrMov(REG::ESI, 0xbc, 0).jmp(L_NOT);
		injectors.push_back(l19);

		Injector* lNotArrive = new Injector{ L_NOT_ARRIVE };
		lNotArrive->ptrMov(REG::ESI, 0x68, 2000).jmp(L_NOT);
		injectors.push_back(lNotArrive);

		Injector* lNot = new Injector{ L_NOT };
		lNot->ptrMov(REG::ESI, 0x8, REG::EAX).call(0x6397d0).jmp(0x52afd2);
		injectors.push_back(lNot);
	}

	for (size_t i = 0; i < injectors.size(); i++)
	{
		injectors[injectors.size() - i - 1]->effect();
		Sleep(1);
	}
}

void genSlotTypeCli(SlotType& zombies)
{
	std::cout << "大特性设置: 从以下四个池子中选择僵尸" << std::endl;
	std::cout << "以下为池子: "  << std::endl;
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
			zombies = generateSlotTypes();
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

constexpr int getCardOrder(ZombieType::ZombieType zt)
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

void crashSolution()
{
	std::cout << "*****************************************************" << std::endl;
	std::cout << "游戏异常退出" << std::endl;
	std::cout << "此时您通过了"
		<< levelState->accomplishedFlagNo
		<< "关, 用时"
		<< levelState->currentTime.cnPrint()
		<< ", 阳光"
		<< levelState->sunNo << std::endl;
	std::cout << "正在等待重新进入游戏..." << std::endl;
	while (!ProcessOpener::Open()) { Sleep(1); };
	DWORD pid = ProcessOpener::Open();
	__pvz = new PVZ(pid);
	std::cout << "请进入ize并Restart" << std::endl;
	while (!(__pvz->BaseAddress && __pvz->LevelId == PVZLevel::I_Zombie_Endless)) { Sleep(1); }
	std::cout << "按s开始游戏" << std::endl;
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

int main()
{
	setlocale(LC_ALL, ".936");
	SetConsoleTitle(WINDOW_NAME);
	while (true) {
		std::cout << INIT_WORDS << std::endl;
		std::string s;
		std::cin >> s;
		

		if (!s.compare("0"))
		{
			std::cout << "退出." << std::endl;
			return 0;
		}
		else if (!s.compare("1"))
		{
			std::cout << "请输入布阵码：" << std::endl;
			std::string ls;
			std::cin >> ls;
			std::array<Theme, 25> themes{};
			std::array<size_t, 25> seeds{};
			std::vector<SmallFeature> smallFeatures{};
			if (!decodeLayoutString(ls, themes, seeds))
			{
				std::cout <<
					"*****************************************************\n\
输入不合法!\n\
*****************************************************" << std::endl;
				continue;
			}

			std::cout <<
				"*****************************************************\n\
输入成功!\n\
*****************************************************" << std::endl;
			int i = 0;
			if (!ls.compare(""))
			{
				smallFeatures = { SmallFeature::NIL };
			}
			else
			{
				int ret = decodeFeatureString(ls, slotZombieTypes, smallFeatures);
				if (ret == 0) continue;
			}
			i = startGame(themes, seeds, smallFeatures);
			if (i == 2)
			{
				crashSolution();
			}
			else if (i != 0)
			{
				std::cout << "可能出错了!, 出错代码" << i << std::endl;
			}
			exitGame(__pvz, slotZombieTypes, smallFeatures);
		}
		else if (!s.compare("2")) // string::compare()相等返回0
		{
			auto ls = generateLayoutString();
			ls.append(".IhGv"); // “常规”小鹤双拼
			std::cout << ls << std::endl;
			copyToClipBoard(ls);
			std::cout << "已复制到剪贴板" << std::endl;
			continue;
		}
		else if (!s.compare("3"))
		{
			auto zombies = slotZombieTypes;
			genSlotTypeCli(zombies);
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
			auto ls = generateLayoutString();
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
			auto smallFeatures = generateSmallFeatures(number, extraFeatures, givenFeatures);
			std::sort(smallFeatures.begin(), smallFeatures.end());
			getFeaturedLayoutString(ls, zombies, smallFeatures);
			std::cout << ls << std::endl;
			copyToClipBoard(ls);
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

			setLayout(__pvz, 0, Theme::INSTANT_KILL, 465466, { SmallFeature::NIL });


		}
#endif
		else
		{
			std::cout << "输入不合法!" << std::endl;
		}
	}
	return 0;
}
