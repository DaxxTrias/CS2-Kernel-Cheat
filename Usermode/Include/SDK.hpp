#pragma once

#include <cstdint>
#include <array>
#include <Windows.h>

#include "Driver.hpp"
#include "offsets.hpp"
#include "client.dll.hpp"

class CHandle {
public:
	uint32_t GetEntryIndex() const noexcept {
		return index & 0x7FFF;
	}

	bool IsValid() const noexcept {
		return index < 0 && index != 0xFFFFFFFF;
	}

	bool operator==(const CHandle other) const noexcept {
		return index == other.index;
	}

	bool operator!=(const CHandle other) const noexcept {
		return index != other.index;
	}

private:
	uint32_t index;
};

class CEntityIdentity {
public:
	uint64_t entity_ptr;
	uint8_t padding_0[0x8];
	CHandle entity_handle;
	uint8_t padding_1[0x64];
};

class CEntityIdentities {
public:
	std::array<CEntityIdentity, 512> ids(HANDLE driver) const noexcept {
		return driver::read_memory<std::array<CEntityIdentity, 512>>(driver, reinterpret_cast<uint64_t>(this));
	}
};

class EntitySystem {
public:
	explicit EntitySystem(uintptr_t client, HANDLE driver) noexcept {
		const auto ptr = driver::read_memory<uintptr_t>(driver, client + client_dll::dwEntityList);

		identity_segments = driver::read_memory<std::array<CEntityIdentities*, 512>>(driver, ptr + 0x10);
	}

	uintptr_t CCSPlayerController(uintptr_t index, HANDLE driver) const noexcept {
		if (index >= 512 - 1) return 0;

		uintptr_t index_segment = index / 512;

		if (index_segment >= 512) return 0;

		CEntityIdentities* segment = identity_segments[index_segment];

		CEntityIdentity identity = segment->ids(driver)[index & 512];

		return identity.entity_handle.GetEntryIndex() == index ? identity.entity_ptr : 0;
	}

	uint64_t PlayerPawn(uintptr_t player_controller, HANDLE driver) const noexcept {
		CHandle PawnHandle = driver::read_memory<CHandle>(driver, player_controller + CBasePlayerController::m_hPawn);

		if (!PawnHandle.IsValid()) {
			return 0;
		}

		uint32_t pawnIndex = PawnHandle.GetEntryIndex();

		if (pawnIndex >= 512 - 1) return 0;

		uint32_t SegmentIndex = pawnIndex / 512;

		if (SegmentIndex >= 512) return 0;

		CEntityIdentities* segment = identity_segments[SegmentIndex];

		CEntityIdentity identity = segment->ids(driver)[pawnIndex & 512];

		return identity.entity_handle.GetEntryIndex() == pawnIndex ? identity.entity_ptr : 0;
	}

private:
	std::array<CEntityIdentities*, 512> identity_segments;
};