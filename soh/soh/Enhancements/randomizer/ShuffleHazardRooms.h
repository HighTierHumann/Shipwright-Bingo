#pragma once

#include <nlohmann/json_fwd.hpp>

// Builds the seed-deterministic set of rooms that should deal heat damage this seed.
// Must be called once per seed generation, after the RNG has been seeded with the seed hash.
void ShuffleRoomHazards_GenerateHazardRooms();
void ShuffleRoomHazards_WriteSpoilerLog(nlohmann::ordered_json& spoilerFileJson);
void ShuffleRoomHazards_ParseSpoilerLog(const nlohmann::json& spoilerFileJson);

