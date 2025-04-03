#pragma once
#include "ProcessOpener.h"
#include "PVZ.h"
#include "Classes.hpp"
#include "Creators.h"
#include "Draw.h"
#include "Extensions.h"
#include "utils.h"
#include "Sexy.h"



#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <limits>
#include <iomanip>
#include <numeric>
#include<unordered_set>

#include <intrin.h>
#include <iphlpapi.h>
#include <VersionHelpers.h>
#include <wincrypt.h>
#include <windows.h>
#include <tlhelp32.h>

#include <comdef.h>
#include <Wbemidl.h>
#include <fstream>
#include <ctime>
#include <conio.h>
#include <memory> // 需要包含此头文件

#include <thread>
#include <atomic>
#include <chrono>

#include <condition_variable>
#include <queue>
#include <chrono>
#include <atomic>
#include "mutex"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "wbemuuid.lib")





// 布阵器描述
constexpr wchar_t WINDOW_NAME[] = L"IZE竞速布阵器";

constexpr char INIT_WORDS[] = "\
欢迎使用IZE竞速玩法布阵器 = v = \n\
作者: 解不出积分的小风; \n\
github地址:https://github.com/LunFengChen/IZESpeedLayout/\n\
请在输入功能对应序号后按回车键：\n\
1：布阵\n\
2：生成25关随机阵型代码\n\
3: (不可用)限时残局玩法\n\
4：冲关\n\
5: 生成当前电脑机器码【比赛模式】\n\
6：生成随机阵型代码【比赛模式】\n\
7：布阵【比赛模式】\n\
8：锁主题锁花数练习，带单关计时\n\
a：导出本关ize阵型代码\n\
b. 连续布阵\n\
c：(不可用)弹出工具-磁铁倒计时\n\
0：使用说明";
	

constexpr char USE_GUIDES[] = "1.布阵: 根据拿到的25关布阵码进行布阵，限时30min，最高25关；提供日志记录，log文件, 在exe文件目录下IZESpeedLayoutDatas/\n\
	(1)使用前先重开确保栈位为0-24，这样第一关栈位才对;\n\
	(2)输入合法后会提示主题序号；\n\
	(3)快捷键：a)shift+Q/q 强制结束 (2)shift+R/r重开当前关卡\n\
2.生成随机的25关布阵码:  \n\
	(1)除去B类阵分布外，其余与原版完全一致；\n\
	(2)1-8花A类与1-7花B类均有可能出现（六届不会出现1花胆小）\n\
	(4)对于B类，每5关出现一个，前3个B类出现一个胆小【调整后现在可出现1花胆小】；\n\
	(4)花数分布与六届手速杯规则一致,为876554433211223+；\n\
3.【还没写好】残局玩法:\n\
4.不限时冲关：与正常冲关无异，旨在提供一些数据便于玩家后续复盘；\n\
	(1)快捷键：a)shift+A 切换自动收集 2)shift+D 切换加速，每关开始会恢复原速 3)shift+Q 强制退出 4)shift+J 跳关，用于卡礼物然后无法过关的情况\n\
5.生成本台电脑机器码: 把机器码提交给裁判；两台电脑不可能出现一样的；但不保证本电脑会不会变（，不过正常玩家也不会随便变动啊\n\
6.生成仅供两台电脑使用的布阵码：使用流程如下\n\
	(1)裁判从玩家除拿到两个机器码 \n\
	(2)生成加密布阵码后，粘贴发给玩家\n\
	(3)再拿到解密密钥后，粘贴发给玩家\n\
7.比赛模式布阵：额外开启反作弊检测；也会提示主题序号；使用流程如下\n\
	(1)拿到加密布阵码输入布阵器；\n\
	(2)输入密钥解密布阵码；\n\
	后正常游戏\n\
8.锁主题锁花数，带单关计时，其实8应该是娱乐模式：什么消消乐模式（，考验眼力和反应力\n\
a.导出本关ize阵型代码：方便捏码\n\
b.连续布阵\n\
b.【还没写好】弹出磁铁倒计时: 类似雪线那种";

