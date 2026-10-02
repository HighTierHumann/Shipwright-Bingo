#pragma once

#include <nlohmann/json_fwd.hpp>

// Builds the seed-deterministic set of rooms that should deal heat damage this seed.
// Must be called once per seed generation, after the RNG has been seeded with the seed hash.
void ShuffleHeatedRooms_GenerateHeatedRooms();
void ShuffleHeatedRooms_WriteSpoilerLog(nlohmann::ordered_json& spoilerFileJson);
void ShuffleHeatedRooms_ParseSpoilerLog(const nlohmann::json& spoilerFileJson);

