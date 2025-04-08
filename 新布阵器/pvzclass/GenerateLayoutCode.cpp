#include "GenerateLayoutCode.h"
#include <algorithm>
#include <random>
#include <cassert>

// 种子
std::mt19937_64 GenerateLayoutCode::gen(std::random_device{}());

// 构造函数
GenerateLayoutCode::GenerateLayoutCode() {};

// 获取六届手速杯花数分布
int GenerateLayoutCode::get_ssb6_flowerNum_distribution(int flag) {
    std::array<int, 25> tmp = { 8, 7, 6, 5, 5, 4, 4, 3, 3, 2, 1, 1, 2, 2, 3, 3 ,3 ,3 ,3, 3, 3, 3, 3, 3, 3 };
    return  tmp[flag];
}

// 获取冲关花数分布
int GenerateLayoutCode::get_LevelRush_flower_num_distribution(int flag) {
    if (flag == 0) return 8;
    if (flag == 1) return 7;
    if (flag == 2) return get_random_in_range(4, 6);
    if (flag == 3) return get_random_in_range(4, 5);
    if (flag >= 4 && flag <= 5) return get_random_in_range(3, 5);
    if (flag >= 6 && flag <= 9) return get_random_in_range(2, 4);
    return get_random_in_range(1, 3);
}

// 获取min-max的随机数，闭区间
int GenerateLayoutCode::get_random_in_range(int min, int max) {
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}


// 根据主题获取对应植物类型（包含顺序）: 花数+主题
std::array<PlantType::PlantType, 25> GenerateLayoutCode::get_theme_plants(int flower_num, Theme theme) {
    std::array<PlantType::PlantType, 25> ret = {};
    for (size_t i = 0; i < flower_num; i++) ret[i] = PlantType::Sunflower;
    for (size_t i = flower_num; i < 8; i++) ret[i] = PlantType::Puffshroom;
    // 实现获取主题植物的逻辑（需要补充）
    for (size_t i = 0; i < 17; i++) ret[i + 8] = getThemePlantTypes(theme)[i];
    return ret;
}


//根据种子获取0-24的随机序列
std::array<int, 25> GenerateLayoutCode::get_shuffled_array(size_t seed) {
    std::array<int, 25> arr = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24 };
    std::mt19937_64 local_gen(seed);
    std::shuffle(arr.begin(), arr.end(), local_gen);
    return arr;
}

// 根据植物种植位置获取对应行和列 
std::pair<int, int> GenerateLayoutCode::get_plant_row_col(int order) {
    return { order / 5, order % 5 };
}

// 获取六届手速杯主题分布【修改后】: 每5关必出一个B类，2-12必出胆小
std::array<Theme, 25> GenerateLayoutCode::generate_ssb6_theme_distribution() {
    std::array<Theme, 5> BOrder = {
        Theme::PEAS,
        Theme::STAR_AND_SPIKE,
        Theme::EXPLODING,
        Theme::MAGNAT_AND_FUME,
        Theme::SCARDY
    };

    while (BOrder[0] != Theme::SCARDY && BOrder[1] != Theme::SCARDY && BOrder[2] != Theme::SCARDY) {
        std::shuffle(BOrder.begin(), BOrder.end(), gen);
    }

    std::array<Theme, 25> ret = {};
    // 动态生成位置索引
    std::array<int, 5> tmp1;
    for (size_t i = 0; i < 5; ++i) {
        // 特殊处理 BOrder[2] 为 SCARDY 的情况
        if (BOrder[i] == Theme::SCARDY && i == 2) { // 胆小是第三个B类阵，只能生成在11-12
            tmp1[i] = get_random_in_range(0, 1); 
        }
        else 
        {
            // 其他情况保持原有逻辑
            tmp1[i] = (i == 0) ? get_random_in_range(1, 4) : get_random_in_range(0, 4);
        }
    }

    for (size_t i = 0; i < 5; i++) {
        ret[tmp1[i] + 5 * i] = BOrder[i];
    }

    std::uniform_int_distribution<unsigned> d3(0, 5);
    for (auto& it : ret) {
        if (it != Theme::NIL) continue;
        int i = d3(gen);
        if (i == 5) it = Theme::INSTANT_KILL;
        else if (i == 4 || i == 3) it = Theme::CONTROL;
        else if (i <= 2) it = Theme::COMPOSITE;
    }
    return ret;
}

// 冲关主题分布
int GenerateLayoutCode::generate_LevelRush_theme_index(int flag) {
    if (flag == 0) {
        std::discrete_distribution<> dist({ 50, 33, 17 });// 1/2 1/3 1/6
        return dist(gen) + 1;
    }
    else {
        std::discrete_distribution<> dist({ 40, 27, 13, 4, 4, 4, 4, 4 });
        return dist(gen) + 1;
    }
}

// 生成六届手速杯阵型代码
std::string GenerateLayoutCode::generate_ssb6_code() {
    std::string layout_code;
    auto themes = generate_ssb6_theme_distribution();
    for (size_t flag = 0; flag < 25; flag++) {
        auto theme = themes[flag];
        auto result = generate_arr_seed(theme);
        auto plant_order_arr = result.first;
        auto seed = result.second;
        int flower_num = get_ssb6_flowerNum_distribution(flag);
        auto plants_types_dict = get_theme_plants(flower_num, theme);
        std::array<PlantType::PlantType, 25> plants_types_arr;
        for (size_t i = 0; i < 25; i++) {
            plants_types_arr[i] = plants_types_dict[plant_order_arr[i]];
        }
        layout_code += std::to_string(static_cast<int>(theme))
            + std::to_string(flower_num)
            + "00"
            + encode_seed(seed)
            + ".";
    }
    return layout_code.substr(0, layout_code.size() - 1);
}

// 生成冲关阵型代码
std::string GenerateLayoutCode::generate_LevelRush_code(int flag) {
    int theme_index = generate_LevelRush_theme_index(flag);
    Theme theme = static_cast<Theme>(theme_index);
    auto flower_num = get_LevelRush_flower_num_distribution(flag);
    auto result = generate_arr_seed(theme);
    return std::to_string(theme_index)
        + std::to_string(flower_num)
        + "00"
        + encode_seed(result.second);
}

// 生成残局阵型代码
std::string GenerateLayoutCode::generate_incompleteLevel_code() {
    std::string layout_code;
    for (size_t flag = 0; flag < 15; flag++) {
        int theme_index = generate_LevelRush_theme_index(flag); // 原版冲关概率生成
        Theme theme = static_cast<Theme>(theme_index);
        int flower_num = get_random_in_range(1, 3);
        int sun = 0;
        //TODO: 根据不同主题设置合适阳光
        // 要求 sun*25+flower_num*200在300->675之间
        do {
            sun = get_random_in_range(3, 12); // 直接赋值给外层变量
        } while (
            (sun * 25 + flower_num * 200) < 500 || // 总和太小
            (sun * 25 + flower_num * 200) > 800    // 或太大时继续循环
            );
        // 格式化sun为两位: 9->09
        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << sun;
        std::string sun_str = oss.str();

        auto result = generate_IncompleteLevel_arr_seed(theme);
        layout_code += std::to_string(theme_index)
            + std::to_string(flower_num)
            + sun_str
            + encode_seed(result.second)
            + ".";
    }
    return layout_code.substr(0, layout_code.size() - 1);
}


// 获取火炬坚果合规种子
std::pair<std::array<int, 25>, size_t> GenerateLayoutCode::generate_arr_seed(Theme theme) {
    switch (theme) {
    case Theme::COMPOSITE:
        while (true) {
            size_t seed = gen();
            auto arr = get_shuffled_array(seed);
            if (get_plant_row_col(arr[8]).second >= 2 && get_plant_row_col(arr[9]).second >= 2) {
                return { arr, seed };
            }
        }
    case Theme::CONTROL:
        while (true) {
            size_t seed = gen();
            auto arr = get_shuffled_array(seed);
            if (get_plant_row_col(arr[8]).second >= 2) {
                return { arr, seed };
            }
        }
    default:
        size_t seed = gen();
        return { get_shuffled_array(seed), seed };
    }
}


std::pair<std::array<int, 25>, size_t> GenerateLayoutCode::generate_IncompleteLevel_arr_seed(Theme theme) {
    switch (theme) {
    case Theme::COMPOSITE:
        while (true) {
            size_t seed = gen();
            auto arr = get_shuffled_array(seed);
            // 添加 arr[0] >= 3 的校验
            if (get_plant_row_col(arr[8]).second >= 2 &&
                get_plant_row_col(arr[9]).second >= 2 &&
                get_plant_row_col(arr[0]).second >= 3) { // 新增条件
                return { arr, seed };
            }
        }
    case Theme::CONTROL:
        while (true) {
            size_t seed = gen();
            auto arr = get_shuffled_array(seed);
            // 添加 arr[0] >= 3 的校验
            if (get_plant_row_col(arr[8]).second >= 2 &&
                get_plant_row_col(arr[0]).second >= 3) { // 新增条件
                return { arr, seed };
            }
        }
    default:
        // 默认主题也需要循环校验
        while (true) {
            size_t seed = gen();
            auto arr = get_shuffled_array(seed);
            if (get_plant_row_col(arr[0]).second >= 3) { // 仅校验 arr[0]
                return { arr, seed };
            }
        }
    }
}


// 把植物的种植位置映射到25进制
std::string GenerateLayoutCode::encrypt_to_base25(const std::array<int, 25>& positions) {
     std::string encrypted = "";

     for (int i = 0; i < 25; ++i) {
         // 将数字0-24映射到字符Y-A
         encrypted += char('Y' - positions[i]);
     }

     return encrypted;
 }

// 把植物的种植位置从25进制反映射到种植位置数组
std::array<int, 25> GenerateLayoutCode::decrypt_from_base25(const std::string& encrypted) {
     std::array<int, 25> positions = {};
     if (encrypted.length() != 25) {
         std::cerr << "错误：加密字符串长度应为25" << std::endl;
         return positions;  // 返回一个空的数组
     }
     for (int i = 0; i < 25; ++i) {
         // 将字符Y-A映射回数字0-24
         positions[i] = 'Y' - encrypted[i];
     }
     return positions;
 }


// 指定ascii码转换为[0, BASE)范围内的数字
size_t GenerateLayoutCode::char_to_number(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c <= 'Z') return c + 10 - 'A';
	return c + 8 - 'A';
}

// [0, BASE)范围内的数字转换为指定ascii码
 char GenerateLayoutCode::number_to_char(size_t i)
{
    if (i >= 0 && i <= 9) return i + '0';
    if (i <= 35) return i - 10 + 'A';
    return i - 8 + 'A';
}

// size_t范围内的整数指数幂
size_t GenerateLayoutCode::calc_power(size_t base, size_t power)
{
    size_t ret = 1;
    for (size_t i = 0; i < power; i++) ret *= base;
    return ret;
}

// 用66进制压缩种子（加密）
std::string GenerateLayoutCode::encode_seed(const size_t& seed)
{
    auto ret = std::string();
    size_t t = seed;
    while (t)
    {
        ret += number_to_char(t % BASE);
        t /= BASE;
    }
    return ret;
}


// 根据种类解压布阵码得到4个信息：(1)主题(2)花数(3)阳光(4)种植位置
bool GenerateLayoutCode::decode_layout_string(const std::string& ls, int& theme_index, int& flower_num, int& sun, std::array<int, 25>& orders) {
    // 布阵码有三类: (1)六届常规(2)捏码(3)残局
    theme_index = static_cast<int>(ls[0] - '0');
    flower_num = static_cast<int>(ls[1] - '0');
    sun = std::stoi(ls.substr(2, 2)) * 25;
    std::string order_str = ls.substr(4);

    if (order_str.size() == 25) { // 代表捏码, 转为将数字0-24映射到字符Y-A
        orders = decrypt_from_base25(order_str);
    }
    else {
        size_t seed = 0; // 第二位代表每一关的种子，不同码是变动的
        for (size_t j = 0; j < order_str.size(); j++)
        {
            seed += char_to_number(order_str[j]) * calc_power(BASE, j); // 布阵码,每一个字符按照66的j-1次幂进行计算，后数字对66取模获取对应的数字
        }
        orders = get_shuffled_array(seed);
    }
    return true;
}