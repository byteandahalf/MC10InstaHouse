#include <Windows.h>
#include <stdint.h>
#include <Psapi.h>
#include <iostream>

// base address
static uintptr_t BaseAddress;
HANDLE process;
uintptr_t SlideAddress(uintptr_t offset) {
	return BaseAddress + offset;
}

typedef void Block;
struct Item
{
	uintptr_t** vtable;
	short id;
};

struct ItemInstance
{
	short count, data;
	uintptr_t* tag;
	Item* item;
	Block* block;
};

struct FullBlock
{
	uint8_t id, data;
};

struct BlockPos
{
	int x, y, z;
};

struct Player
{
	char filler[408];
	uintptr_t* region;
};

static FullBlock&(*getBlock)(uintptr_t*, FullBlock&, const BlockPos&);
static void(*setBlock)(uintptr_t*, const BlockPos&, FullBlock&);
static uintptr_t** VTCreativeMode;

#define SET(region, x, y, z, id, data) setBlock(region, {x, y, z}, FullBlock{id, data})

void placeHouse(uintptr_t* region, const BlockPos& pos)
{
	// I know this could be made so much nicer
	int x = pos.x, y = pos.y, z = pos.z;
	for (int i = y - 1; i < y+3; i++)
	{
		SET(region, x, i, z, 5, 0);
		SET(region, x + 1, i, z, 5, 0);
		SET(region, x + 2, i, z, 5, 0);
		SET(region, x - 1, i, z, 5, 0);
		SET(region, x - 2, i, z, 5, 0);
		SET(region, x + 2, i, z + 1, 5, 0);
		SET(region, x - 2, i, z + 1, 5, 0);
		SET(region, x + 2, i, z + 2, 5, 0);
		SET(region, x - 2, i, z + 2, 5, 0);
		SET(region, x + 2, i, z + 3, 5, 0);
		SET(region, x - 2, i, z + 3, 5, 0);
		SET(region, x + 2, i, z + 4, 5, 0);
		SET(region, x - 2, i, z + 4, 5, 0);
		SET(region, x + 1, i, z + 4, 5, 0);
		SET(region, x - 1, i, z + 4, 5, 0);
		SET(region, x, i, z + 4, 5, 0);
	}
	SET(region, x, y, z, 0, 0);
	SET(region, x, y + 1, z, 0, 0);
	for (int p = y - 1; p < y + 4; p+=4)
	{
		for (int j = x - 1; j < x + 2; j++)
		{
			for (int b = z + 1; b < z + 4; b++)
			{
				SET(region, j, p, b, 5, 0);
				if (p == y - 1)
				{
					SET(region, j, p + 1, b, 171, 14);
				}
			}
		}
	}
}

void(*_useItem)(uintptr_t*, Player&, ItemInstance*, const BlockPos&, signed char, uintptr_t*);
void useItem(uintptr_t* self, Player& player, ItemInstance* item, const BlockPos& pos, signed char side, uintptr_t* vec)
{
	if (getBlock(player.region, FullBlock{ 0, 0 }, pos).id == 6)
	{
		placeHouse(player.region, pos);
		return;
	}

	_useItem(self, player, item, pos, side, vec);
}

bool minecraftH4x0r()
{
	getBlock = (FullBlock&(*)(uintptr_t*, FullBlock&, const BlockPos&))SlideAddress(0x399860);
	setBlock = (void(*)(uintptr_t*, const BlockPos&, FullBlock&))SlideAddress(0x39A970);

	VTCreativeMode = (uintptr_t**)SlideAddress(0x9A6AA0);
	_useItem = (void(*)(uintptr_t*, Player&, ItemInstance*, const BlockPos&, signed char, uintptr_t*))VTCreativeMode[11];
	VTCreativeMode[11] = (uintptr_t*)&useItem;

	return true;
}

// find base ptr dynamically
DWORD_PTR GetProcessBaseAddress(DWORD processID)
{
	DWORD_PTR baseAddress = 0;
	HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
	HMODULE* moduleArray;
	LPBYTE moduleArrayBytes;
	DWORD bytesRequired;

	if(processHandle)
	{
		if(EnumProcessModules(processHandle, NULL, 0, &bytesRequired))
		{
			if(bytesRequired)
			{
				moduleArrayBytes = (LPBYTE)LocalAlloc(LPTR, bytesRequired);

				if(moduleArrayBytes)
				{
					unsigned int moduleCount;

					moduleCount = bytesRequired / sizeof(HMODULE);
					moduleArray = (HMODULE*)moduleArrayBytes;

					if(EnumProcessModules(processHandle, moduleArray, bytesRequired, &bytesRequired))
					{
						baseAddress = (DWORD_PTR)moduleArray[0];
					}

					LocalFree(moduleArrayBytes);
				}
			}
		}

		CloseHandle(processHandle);
	}

	return baseAddress;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	DWORD procId = GetCurrentProcessId();
	process = OpenProcess(PROCESS_ALL_ACCESS | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, procId);
	BaseAddress = (uintptr_t)GetProcessBaseAddress(procId);

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		return minecraftH4x0r();
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
