#include "pvzclass.h"


//int main() {
//	DWORD pid = ProcessOpener::Open();
//	if (!pid) return 1;
//	PVZ::InitPVZ(pid);
//
//	auto current_address = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768);
//	
//	while (true) {
//		if (current_address != PVZ::Memory::ReadPointer(0x6a9ec0, 0x768)) {
//			// 重开了
//			std::cout << "重开了" << std::endl;
//			// 重新赋值
//			current_address = PVZ::Memory::ReadPointer(0x6a9ec0, 0x768);
//		}
//		Sleep(1);
//	}
//
//	PVZ::QuitPVZ();
//	return 0;
//}