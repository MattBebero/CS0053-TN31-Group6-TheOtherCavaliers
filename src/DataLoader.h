// DataLoader.h - reads the data files in data/.
// Owner: Al Noor (Member 3).
#pragma once

#include <string>

#include "Types.h"

// Loads classes.txt, enemies.txt and moves.txt from the folder.
// Returns false and prints a readable message if anything is wrong.
bool loadGameData(GameData& data, const std::string& folder);
