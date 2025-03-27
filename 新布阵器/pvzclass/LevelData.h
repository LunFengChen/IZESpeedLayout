#pragma once

#include <vector>

#include "TimeStruct.h"

class LevelData {
public:
    int initial_sun;               // 每关起始阳光
    int released_zombies_count;    // 每关释放的僵尸数量
    int zombie_cost;               // 本关僵尸花费
    float score;                   // 更新分数：当前 flag + 吃的脑子数
    float kernel_count;              // 每一局游戏的玉米数
    float butter_count;
    float kernelpult_butter_rate;  // 玉米投手的黄油率, 没有则为0.00
    

    // 时间相关数据
    TimeStruct setlayout_time = TimeStruct::getNow();             // (1) 每关的开始时间：布阵时间
    TimeStruct first_zombie_release_time = TimeStruct::getNow();    // (2) 每关的放置时间：第一个僵尸释放的时间
    TimeStruct reaction_time = TimeStruct::getNow();                // (3) 计算反应时间：第一个僵尸释放时间 - 进入关卡的时间
    TimeStruct last_brain_eaten_time = TimeStruct::getNow();        // (4) 最后吃脑时间
    std::vector<TimeStruct> brain_eaten_times;                      // 吃脑时间集合

    // 可选：如果需要构造函数、成员函数等，可以在此处声明
    LevelData() = default;
};
