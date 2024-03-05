#include <iostream>

#include "Utils.hpp"
#include "Driver.hpp"
#include "client.dll.hpp"
#include "offsets.hpp"
#include "SDK.hpp"


void bunny_hop(HANDLE driver, uintptr_t client) {
	const auto local_player_pawn = driver::read_memory<std::uintptr_t>(driver, client + client_dll::dwLocalPlayerPawn);

	if (local_player_pawn == 0) return;

	const auto flags = driver::read_memory<std::uint32_t>(driver, local_player_pawn + C_BaseEntity::m_fFlags);

	const bool in_air = flags & (1 << 0);
	const bool space_pressed = GetAsyncKeyState(VK_SPACE);
	const auto force_jump = driver::read_memory<DWORD>(driver, client + client_dll::dwForceJump);

	if (space_pressed && in_air) {
		Sleep(50);
		driver::write_memory(driver, client + client_dll::dwForceJump, 65537);
	}
	else if (space_pressed && !in_air) {
		driver::write_memory(driver, client + client_dll::dwForceJump, 256);
	}
	else if (!space_pressed && force_jump == 65537) {
		driver::write_memory(driver, client + client_dll::dwForceJump, 256);
	}
}

int main() {
	
	const DWORD cs2_pid = get_process_id(L"cs2.exe");
	Assert(cs2_pid != 0, "Unable to find running cs2.exe process", "Obtained cs2.exe process");

	const HANDLE driver = CreateFile(L"\\\\.\\CS2-Driver", GENERIC_READ, 0, nullptr, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, nullptr);
	Assert(driver != INVALID_HANDLE_VALUE, "Unable to create driver", "Created driver");
	
	Assert(driver::attach_to_process(driver, cs2_pid), "Unable to attach driver to process", "Attached driver to cs2.exe");

	std::uintptr_t client = get_module_base(cs2_pid, L"client.dll");
	Assert(client != 0, "Client.dll not found", "Client.dll successfully found");

	EntitySystem entitySystem(client, driver);

	while (true) {
		if (GetAsyncKeyState(VK_END)) break;

		for (int i = 1; i < 64; i++) {
			uintptr_t CCSPlayerController = entitySystem.CCSPlayerController(i, driver);

			uint64_t Player = entitySystem.PlayerPawn(CCSPlayerController, driver);

			uintptr_t game_scene = driver::read_memory<uintptr_t>(driver, CCSPlayerController + C_BaseEntity::m_pGameSceneNode);
			uintptr_t bone_array = driver::read_memory<uintptr_t>(driver, game_scene + CBodyComponentSkeletonInstance::m_skeletonInstance); //+ skeleton vector Y????)

			int health = driver::read_memory<int>(driver, CCSPlayerController + C_BaseEntity::m_iHealth);
		}
	}
	
	CloseHandle(driver);
	std::cin.get();
	return 0;
}