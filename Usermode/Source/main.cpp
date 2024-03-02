#include <iostream>

#include "Utils.hpp"
#include "Driver.hpp"
#include "client.dll.hpp"
#include "offsets.hpp"

int main() {
	
	const DWORD pid = get_process_id(L"cs2.exe");

	if (pid == 0) {
		std::cout << "Failed to find CS2\n";
		std::cin.get();
		return 1;
	}

	const HANDLE driver = CreateFile(L"\\\\.\\CS2-Driver", GENERIC_READ, 0, nullptr, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, nullptr);

	if (driver == INVALID_HANDLE_VALUE) {
		std::cout << "Failed to create our driver handle.\n";
		std::cin.get();
		return 1;
	}

	if (driver::attach_to_process(driver, pid)) {
		std::cout << "Attachment successful.\n";

		if (const std::uintptr_t client = get_module_base(pid, L"client.dll"); client != 0) {
			std::cout << "Client found\n";

			while (true) {
				if (GetAsyncKeyState(VK_END)) break;

				const auto local_player_pawn = driver::read_memory<std::uintptr_t>(driver, client + client_dll::dwLocalPlayerPawn);

				if (local_player_pawn == 0) continue;

				const auto flags = driver::read_memory<std::uint32_t>(driver, local_player_pawn + C_BaseEntity::m_fFlags);

				const bool in_air = flags & (1 << 0);
				const bool space_pressed = GetAsyncKeyState(VK_SPACE);
				const auto force_jump = driver::read_memory<DWORD>(driver, client + client_dll::dwForceJump);

				if (space_pressed && in_air) {
					Sleep(5);
					driver::write_memory(driver, client + client_dll::dwForceJump, 65537);
				}
				else if (space_pressed && !in_air) {
					driver::write_memory(driver, client + client_dll::dwForceJump, 256);
				}
				else if (!space_pressed && force_jump == 65537) {
					driver::write_memory(driver, client + client_dll::dwForceJump, 256);
				}
			}
		}
		else std::cout << "Client was not found";
	}

	CloseHandle(driver);
	std::cin.get();
	return 0;
}