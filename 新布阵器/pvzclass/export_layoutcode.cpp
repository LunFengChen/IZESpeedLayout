#include "pvzclass.h"

#include <unordered_map>
#include <vector>
#include <random>
#include <algorithm>

#include "GenerateLayoutCode.h"



std::array<int, 27> export_plant_order() {
    std::array<int, 27> positions = {};               // 对应位置

    //if (!PVZ::Memory::ReadPointer(0x6a9ec0, 0x768)) return 1;

    // 1.判断当前主题
    // 1.0 先获取当前board所有存活植物的编号作为字典
    auto board = PVZ::GetBoard();
    // 建立一个字典来存储植物类型和对应的出现次数
    std::unordered_map<PlantType::PlantType, int> plantCount;  // 假设 PlantType 是植物类型 int默认值为0
    for (auto plant : board->GetAllPlants()) {
        if (plant->NotExist) continue;
        // 增加植物类型的计数
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
        std::cerr << "主题不合规, 请检查!" << std::endl;
    }
    std::cout << theme_index << std::endl;


    // 2. 依次获取植物顺序
    std::array<PlantType::PlantType, 25> plant_types = GenerateLayoutCode::get_theme_plants(plantCount[PlantType::Sunflower], static_cast<Theme>(theme_index));

    // 使用vector代替map来保持顺序
    std::vector<std::pair<PlantType::PlantType, std::vector<int>>> plants_position;

    for (auto plant : board->GetAllPlants()) {
        if (plant->NotExist) continue;
        bool found = false;
        for (auto& plant_pair : plants_position) {
            if (plant_pair.first == plant->Type) {
                plant_pair.second.push_back(plant->Row * 5 + plant->Column);
                found = true;
                break;
            }
        }
        if (!found) {
            plants_position.push_back({ plant->Type, {plant->Row * 5 + plant->Column} });
        }
    }

    // 在循环外初始化随机数生成器（只需初始化一次）
    std::random_device rd;
    std::mt19937 rng(rd());  // 使用随机设备种子


    int order = 0;
    positions[order++] = theme_index;
    positions[order++] = plantCount[PlantType::Sunflower];
    bool shuffled = false;

    for (auto& plant_type_position : plants_position) {
        // 随机打乱plant_type_position.second
        std::shuffle(
            plant_type_position.second.begin(),
            plant_type_position.second.end(),
            rng  // 传入随机数生成器
        );

        // 拿数
        for (auto it : plant_type_position.second) {
            positions[order++] = it;
        }
    }

    for (int i = 0; i < 25; i++) {
        std::cout << positions[i + 2] << " ";
    }

    return positions;
}



int main() {

    DWORD pid = ProcessOpener::Open();
    if (!pid) return 1;

    PVZ::InitPVZ(pid);

    auto board = PVZ::GetBoard();
    std::array<int, 27> plant_positon = export_plant_order();



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

    int theme_index = plant_positon[0]; int flower_num = plant_positon[1];
    std::array<PlantType::PlantType, 25> plantTypes = GenerateLayoutCode::get_theme_plants(flower_num, static_cast<Theme>(theme_index));
    for (int i = 0; i < 25; i++) {
        Creator::CreatePlant(plantTypes[i], plant_positon[i + 2] / 5, plant_positon[i + 2] % 5)->SetStatic();
    }

    PVZ::QuitPVZ();





	return 0;
}

