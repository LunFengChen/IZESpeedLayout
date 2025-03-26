#pragma once
namespace PlantType
{

	enum PlantType
	{
		None = -1, // 无
		Peashooter, // 豌豆射手
		Sunflower, // 向日葵
		CherryBomb, // 樱桃炸弹
		Wallnut, // 坚果墙
		PotatoMine, // 土豆地雷
		SnowPea, // 寒冰射手
		Chomper, // 大嘴花
		Repeater, // 双发射手
		Puffshroom, // 小喷菇
		Sunshroom, // 阳光菇
		Fumeshroom, // 大喷菇
		CraveBuster, // 咀嚼者
		Hypnoshroom, // 催眠菇
		Scaredyshroom, // 胆小菇
		Iceshroom, // 寒冰菇
		Doomshroom, // 毁灭菇
		LilyPad, // 荷叶
		Squash, // 倭瓜
		Threepeater, // 三发射手
		TangleKelp, // 缠绕海藻
		Jalapeno, // 火爆辣椒
		Spickweed, // 地刺
		Torchwood, // 火炬树桩
		Tallnut, // 高坚果
		Seashroom, // 海蘑菇
		Plantern, // 灯笼草
		Cactus, // 仙人掌
		Blover, // 三叶草
		SplitPea, // 分裂豌豆
		Starfruit, // 杨桃
		Pumpkin, // 南瓜
		Magnetshroom, // 磁力菇
		Cabbagepult, // 卷心菜投手
		FlowerPot, // 花盆
		Kernelpult, // 玉米投手
		CoffeeBean, // 咖啡豆
		Garlic, // 大蒜
		UmbrellaLeaf, // 伞叶
		Marigold, // 金盏花
		Melonpult, // 西瓜投手
		GatlingPea, // 加特林豌豆
		TwinSunflower, // 双子向日葵
		Gloomshroom, // 恶魔菇
		Cattail, // 香蒲
		WinterMelon, // 冬瓜投手
		GoldMagnet, // 金磁菇
		Spikerock, // ?
		CobCannon, // 玉米加农炮
		Imitater, // 模仿者
		Explodenut, // 爆炸坚果
		GiantWallnut, // 巨型坚果
		Sprout, // 幼苗
		LeftRepeater, // 左向双发射手
	};

	extern const char* ToString(PlantType plantt); // 将植物类型转换为字符串的函数声明

}