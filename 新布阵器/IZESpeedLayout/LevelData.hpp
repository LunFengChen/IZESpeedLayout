#pragma once

#include <vector>

#include "TimeStruct.h"

class LevelData {
public:
    int initial_sun;               // 每关起始阳光
    float score;                   // 更新分数：当前 flag + 吃的脑子数
    int eaten_brain_count; // 吃脑数
    int lost_sun;
    int flower_num;
    
    // ------------------------ 游戏数据统计: 黄油率, 时间，初始阳光，收集阳光, 解法 ----------------------
    int kernel_count;              // 每一局游戏的玉米数
    int butter_count;
    float kernelpult_butter_rate;  // 玉米投手的黄油率, 没有则为0.00
    int collected_sun; // 收集的阳光
    int released_zombies_count;    // 每关释放的僵尸数量
    int zombie_cost;               // 本关僵尸花费
    std::vector<std::tuple<ZombieType::ZombieType, int, TimeStruct>> solve_info;

    // 时间相关数据
    TimeStruct setlayout_time = TimeStruct::getNow();             // (1) 每关的开始时间：布阵时间
    TimeStruct current_use_time = TimeStruct::getNow();             // (1) 每关的开始时间：布阵时间
    TimeStruct first_zombie_release_time = TimeStruct::getNow();    // (2) 每关的放置时间：第一个僵尸释放的时间
    TimeStruct reaction_time = TimeStruct::getNow();                // (3) 计算反应时间：第一个僵尸释放时间 - 进入关卡的时间
    TimeStruct last_brain_eaten_time = TimeStruct::getNow();        // (4) 最后吃脑时间
    std::vector<TimeStruct> brain_eaten_times;                      // 吃脑时间集合

    // 可选：如果需要构造函数、成员函数等，可以在此处声明
    LevelData() {
        init();
    }

    // 初始化函数：重置所有成员变量到默认状态
    void init() {
        initial_sun = 0;
        released_zombies_count = 0;
        zombie_cost = 0;
        //score = 0.0f;
        kernel_count = 0.0f;
        butter_count = 0.0f;
        kernelpult_butter_rate = 0.0f;
        eaten_brain_count = 0;
        collected_sun = 0;
        lost_sun = 0;
        flower_num = 0;

        // 时间相关重置为当前时间
        current_use_time = TimeStruct(0).getNow();
        reaction_time = TimeStruct(0).getNow();
        brain_eaten_times.clear();  // 清空吃脑时间记录
        solve_info.clear();
    }
};
