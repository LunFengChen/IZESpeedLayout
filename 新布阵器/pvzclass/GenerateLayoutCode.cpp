#include "GenerateLayoutCode.h"
#include <algorithm>
#include <random>

std::mt19937_64 GenerateLayoutCode::gen(std::random_device{}());

GenerateLayoutCode::GenerateLayoutCode()
    : type_iztStr_dict({
        {PlantType::Peashooter, "1"},
        {PlantType::Sunflower, "h"},
        {PlantType::Wallnut, "o"},
        {PlantType::PotatoMine, "t"},
        {PlantType::SnowPea, "b"},
        {PlantType::Chomper, "z"},
        {PlantType::Repeater, "2"},
        {PlantType::Puffshroom, "p"},
        {PlantType::Fumeshroom, "d"},
        {PlantType::Scaredyshroom, "x"},
        {PlantType::Squash, "w"},
        {PlantType::Threepeater, "3"},
        {PlantType::Torchwood, "j"},
        {PlantType::SplitPea, "l"},
        {PlantType::Starfruit, "5"},
        {PlantType::Magnetshroom, "c"},
        {PlantType::Kernelpult, "y"},
        {PlantType::Spickweed, "_"},
        {PlantType::UmbrellaLeaf, "s"}
        }),
    iztStr_type_dict({
      {"1", PlantType::Peashooter},
      {"h", PlantType::Sunflower},
      {"o", PlantType::Wallnut},
      {"t", PlantType::PotatoMine},
      {"b", PlantType::SnowPea},
      {"z", PlantType::Chomper},
      {"2", PlantType::Repeater},
      {"p", PlantType::Puffshroom},
      {"d", PlantType::Fumeshroom},
      {"x", PlantType::Scaredyshroom},
      {"w", PlantType::Squash},
      {"3", PlantType::Threepeater},
      {"j", PlantType::Torchwood},
      {"l", PlantType::SplitPea},
      {"5", PlantType::Starfruit},
      {"c", PlantType::Magnetshroom},
      {"y", PlantType::Kernelpult},
      {"_", PlantType::Spickweed},
      {"s", PlantType::UmbrellaLeaf}
        }),
    ssb6_flower_num_distribution({
      8, 7, 6, 5, 5,
      4, 4, 3, 3, 2,
      1, 1, 2, 2, 3,
      3 ,3 ,3 ,3, 3,
      3, 3, 3, 3, 3
        }) {
}
const std::array<int, 25>& GenerateLayoutCode::getSsb6FlowerNumDistribution() {
    return GenerateLayoutCode::ssb6_flower_num_distribution;
}

int GenerateLayoutCode::getRandomInRange(int min, int max) {
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}

int GenerateLayoutCode::get_LevelRush_flower_num_distribution(int flag) {
    if (flag == 0) return 8;
    if (flag == 1) return 7;
    if (flag == 2) return getRandomInRange(4, 6);
    if (flag == 3) return getRandomInRange(4, 5);
    if (flag >= 4 && flag <= 5) return getRandomInRange(3, 5);
    if (flag >= 6 && flag <= 9) return getRandomInRange(2, 4);
    return getRandomInRange(1, 3);
}

std::array<PlantType::PlantType, 25> GenerateLayoutCode::get_theme_plants(int flower_num, Theme theme) {
    std::array<PlantType::PlantType, 25> ret = {};
    for (size_t i = 0; i < flower_num; i++) ret[i] = PlantType::Sunflower;
    for (size_t i = flower_num; i < 8; i++) ret[i] = PlantType::Puffshroom;
    // 实现获取主题植物的逻辑（需要补充）
    for (size_t i = 0; i < 17; i++) ret[i + 8] = getThemePlantTypes(theme)[i];
    return ret;
}

std::array<int, 25> GenerateLayoutCode::get_shuffled_array(size_t seed) {
    std::array<int, 25> arr = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24 };
    std::mt19937_64 local_gen(seed);
    std::shuffle(arr.begin(), arr.end(), local_gen);
    return arr;
}

std::pair<int, int> GenerateLayoutCode::get_plant_row_col(int order) {
    return { order / 5, order % 5 };
}

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
            tmp1[i] = getRandomInRange(0, 1); 
        }
        else 
        {
            // 其他情况保持原有逻辑
            tmp1[i] = (i == 0) ? getRandomInRange(1, 4) : getRandomInRange(0, 4);
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

std::pair<std::array<int, 25>, size_t> GenerateLayoutCode::generate_arr_seed(Theme theme) {
    switch (theme) {
    case Theme::COMPOSITE:
        while (true) {
            size_t seed = gen();
            auto arr = get_shuffled_array(seed);
            if (get_plant_row_col(arr[8]).second >= 3 && get_plant_row_col(arr[9]).second >= 3) {
                return { arr, seed };
            }
        }
    case Theme::CONTROL:
        while (true) {
            size_t seed = gen();
            auto arr = get_shuffled_array(seed);
            if (get_plant_row_col(arr[8]).second >= 3) {
                return { arr, seed };
            }
        }
    default:
        size_t seed = gen();
        return { get_shuffled_array(seed), seed };
    }
}

std::string GenerateLayoutCode::generate_ssb6_code() {
    std::string layout_code;
    auto themes = generate_ssb6_theme_distribution();
    for (size_t flag = 0; flag < 25; flag++) {
        auto theme = themes[flag];
        auto result = generate_arr_seed(theme);
        auto plant_order_arr = result.first;
        auto seed = result.second;
        int flower_num = ssb6_flower_num_distribution[flag];
        auto plants_types_dict = get_theme_plants(flower_num, theme);
        std::array<PlantType::PlantType, 25> plants_types_arr;
        for (size_t i = 0; i < 25; i++) {
            plants_types_arr[i] = plants_types_dict[plant_order_arr[i]];
        }
        layout_code += std::to_string(static_cast<int>(theme)) + "/" + std::to_string(seed) + ".";
    }
    return layout_code.substr(0, layout_code.size()-1);
}

std::string GenerateLayoutCode::generate_LevelRush_code(int flag) {
    int theme_index = generate_LevelRush_theme_index(flag);
    auto theme = static_cast<Theme>(theme_index);
    auto flower_num = get_LevelRush_flower_num_distribution(flag);
    auto result = generate_arr_seed(theme);
    return std::to_string(theme_index) + "/" + std::to_string(result.second);
}



// 25进制加密函数
 std::string GenerateLayoutCode::encrypt_to_base25(const std::array<int, 25>& positions) {
     std::string encrypted = "";

     for (int i = 0; i < 25; ++i) {
         // 将数字0-24映射到字符Y-A
         encrypted += char('Y' - positions[i]);
     }

     return encrypted;
 }

// 25进制解密函数
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


