// SaveManager.h - saves and loads the run in progress and the run history.
// Owner: Al Noor (Member 3).
#pragma once

#include <string>

#include "Types.h"

const std::string SAVE_FILE = "savegame.txt";
const std::string HISTORY_FILE = "history.txt";

// Saves the run (called after every stage). Returns false if the file cannot be written.
bool saveRun(const RunState& run);

// Loads the saved run. Returns false (with a message) if there is none or it is damaged.
bool loadRun(RunState& run, const GameData& data);

bool saveExists();
void deleteSave();

// One line per finished run: players, classes, result.
void addHistory(const RunState& run, bool won);
void showHistory();
