#pragma once
// ---------------------- pvzclass 框架 ----------------------------
#include "../pvzclass/Classes.hpp"
#include "../pvzclass/Creators.h"
#include "../pvzclass/Draw.h"
#include "../pvzclass/Extensions.h"
#include "../pvzclass/ProcessOpener.h"
#include "../pvzclass/PVZ.h"
#include "../pvzclass/Sexy.h"
#include "../pvzclass/utils.h"


// ---------------------- 自带头文件 -------------------------------
#include <algorithm>
#include <array>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <intrin.h>
#include <iphlpapi.h>
#include <tlhelp32.h>
#include <VersionHelpers.h>
#include <wincrypt.h>
#include <windows.h>

#include <comdef.h>
#include <conio.h>
#include <ctime>
#include <fstream>
#include <memory> // 需要包含此头文件
#include <Wbemidl.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "mutex"
#include <condition_variable>
#include <dbghelp.h>
#include <psapi.h>
#include <queue>


#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "Psapi.lib")



// 布阵器窗口名
constexpr wchar_t WINDOW_NAME[] = L"IZE竞速布阵器v1.1.1";
// 日志文件存放文件夹
constexpr const char* FILE_DIR = "IZESpeedLayoutDatas";
// 签名校验
const std::unordered_map<std::string, std::string> signatureMap = {
		{"1.0.0", "182110e338a554bac0a8b94490ba9c792b80f914f0c3ea9d7d287765b3e48fb0"},
		{"1.1.1", "signature2"},
};

constexpr char INIT_WORDS[] = "欢迎使用IZE竞速布阵器v1.1.1 = v = \n\
作者: 解不出积分的小风; github地址:LunFengChen/IZESpeedLayout\n\
(参考IZE六届手速杯布阵器，作者：碳酸 天盟琉璃 qq交流群:1157197563)\n\
请在输入功能对应序号后按回车键：\n\
1：布阵\n\
2：生成随机阵型代码\n\
3: 生成当前设备机器码【比赛模式】\n\
4：生成加密布阵码【比赛模式】\n\
5：布阵【比赛模式】\n\
6: 残局练习\n\
7：冲关\n\
8：自定义捏码\n\
9. 多关布阵\n\
a：导出本关ize阵型代码\n\
0：使用说明";
	

constexpr char USE_GUIDES[] = "1.布阵: 根据拿到的25关布阵码进行布阵，限时30min，最高25关；\n\
打完可以找到log日志文件, 在布阵器的exe文件目录下IZESpeedLayoutDatas/\n\
	(1)使用前先重开确保植物栈位为0-24，这样第一关栈位才正确;\n\
	(2)输入合法后会提示主题序号，避免加密布阵码导致的无法看主题;\n\
	(3)快捷键：a)shift+Q/q 强制结束 b)shift+R/r 对当前关卡重新布阵 c)shift+S/s pvz关闭后恢复存档 d)shift+P 暂停游戏，暂停计时; 再按一次就是恢复\n\
2.生成随机的25关布阵码:  \n\
	(1)除去B类阵分布外，其余与原版完全一致；\n\
	(2)1-8花A类与1-7花B类均有可能出现（六届不会出现1花胆小）\n\
	(4)对于B类，每5关出现一个，前3个B类出现一个胆小【调整后现在可出现1花胆小】；\n\
	(4)花数分布与六届手速杯规则一致,为876554433211223+；\n\
3.生成本台电脑机器码: 把机器码提交给裁判；两台电脑不可能出现一样的；但不保证本电脑会不会变（，不过正常玩家也不会随便变动啊\n\
4.生成限定机器码与有效期的布阵码：使用流程如下\n\
	(1)裁判从玩家除拿到多个机器码与有效期 \n\
	(2)生成加密布阵码后，粘贴发给玩家即可\n\
5.比赛模式布阵：额外开启反作弊检测；也会提示主题序号；使用流程如下\n\
	(1)拿到加密布阵码输入布阵器；\n\
	(2)输入密钥解密布阵码；\n\
	后与1的功能一样\n\
6.残局布阵:\n\
	(1) 游戏内的restart和lose会变成跳到下一关；\n\
	(2) 玩法：每关阳光随机，只需要尽可能拿到更多脑子即可；\n\
	(3) 阳光数: 算上向日葵500-800\n\
	(4) 向日葵位置分布: 必定有一个向日葵在45列\n\
7. 不限时冲关：与正常冲关无异，旨在提供一些数据便于玩家后续复盘；\n\
	(1)快捷键：a)shift+A 切换自动收集 2)shift+D 切换加速，每关开始会恢复原速 3)shift+Q 强制退出 4)shift+J 跳关，用于卡礼物然后无法过关的情况 5)shift+r 重新布阵\n\
8. 自定义捏码：包括以下功能\n\
	(1) 锁主题\n\
	(2) 锁花数\n\
	(3) 自定义主题或自定义花数\n\
	(4) 不定关数布阵码\n\
	(5) 残局与正常局\n\
9. 多关布阵\n\
a：导出本关ize阵型代码";



