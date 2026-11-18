// Battle.h - one stage of combat: turns, damage, statuses and enemy AI.
// Owner: Lee Jedrek (Member 2).
#pragma once

#include "Types.h"

// Resets the players for a new stage, spawns enemies and fights until the
// stage is cleared (returns true) or the whole party is down (returns false).
bool playStage(const GameData& data, RunState& run, int partySize);

// A readable one-line description of a move (used by Move Info and the tutorial).
std::string describeMove(const Move& move);