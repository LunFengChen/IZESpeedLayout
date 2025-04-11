#pragma once
#include "IZESpeedLayout.h"


constexpr size_t BASE = 66;

// IZE主题类型
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

// 根据主题获取植物类型
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
			pt::Spickweed, pt::Spickweed,
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


class GenerateLayoutCode {
private:
    static std::mt19937_64 gen;
    //std::unordered_map<PlantType::PlantType, std::string> type_iztStr_dict;
    //std::unordered_map<std::string, PlantType::PlantType> iztStr_type_dict;

	// 获取手速杯主题分布
    std::array<Theme, 25> generate_ssb6_theme_distribution();

public:
    GenerateLayoutCode();
	// 获取冲关主题分布
	int generate_LevelRush_theme_index(int flag);
	static int get_ssb6_flowerNum_distribution(int flag);

	std::pair<std::array<int, 25>, size_t> generate_arr_seed(Theme theme);
	std::pair<std::array<int, 25>, size_t> generate_IncompleteLevel_arr_seed(Theme theme);
    static int get_random_in_range(int min, int max);
    static int get_LevelRush_flower_num_distribution(int flag);
    static std::array<PlantType::PlantType, 25> get_theme_plants(int flower_num, Theme theme);
    static std::array<int, 25> get_shuffled_array(size_t seed);
    static std::pair<int, int> get_plant_row_col(int order);

	// 生成布阵码
    std::string generate_ssb_code();
	std::string generate_LevelRush_code(int flag);
	std::string generate_incompleteLevel_one_code(int flag);
	std::string generate_incompleteLevel_code();

	// 植物种植顺序转为25进制字符串
	static std::string encrypt_to_base25(const std::array<int, 25>& positions);
	// 25进制字符串解析成植物种植顺序
	static std::array<int, 25> decrypt_from_base25(const std::string& encrypted);
	// 指定ascii码转换为[0, BASE)范围内的数字
	static size_t char_to_number(char c);
	// [0, BASE)范围内的数字转换为指定ascii码
	static char number_to_char(size_t i);
	// size_t范围内的整数指数幂
	static size_t calc_power(size_t base, size_t power);
	// 把种子转为66进制字符串
	static std::string encode_seed(const size_t& seed);
	// 解析布阵码
	static bool decode_layout_string(const std::string& ls, int& theme_index, int& flower_num, int& sun, std::array<int, 25>& orders);
};