// SaveManager.cpp - saves and loads the run in progress and the run history.
// Owner: Al Noor (Member 3).
//
// savegame.txt looks like this (plain text):
//   DBND1
//   stage|3
//   level|3
//   player|Lee|Mage|46|39|0       (name, class, max HP, HP, Shield)
#include "SaveManager.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "Input.h"

bool saveExists() {
    std::ifstream file(SAVE_FILE);
    return file.good();
}

void deleteSave() { std::remove(SAVE_FILE.c_str()); }

bool saveRun(const RunState& run) {
    std::ofstream file(SAVE_FILE);
    if (!file) return false;
    file << "DBND1\n";
    file << "stage|" << run.stage << "\n";
    file << "level|" << run.level << "\n";
    for (const Unit& p : run.players) {
        file << "player|" << p.name << "|" << p.kind << "|" << p.maxHp << "|" << p.hp << "|"
             << p.shield << "\n";
    }
    return file.good();
}

static int toInt(const std::string& text) {
    size_t used = 0;
    int value = std::stoi(text, &used);
    if (used != text.size()) throw std::invalid_argument("bad number");
    return value;
}

bool loadRun(RunState& run, const GameData& data) {
    std::ifstream file(SAVE_FILE);
    if (!file) {
        std::cout << "No saved game found. Start a new game first.\n";
        return false;
    }
    try {
        std::string line;
        std::getline(file, line);
        if (trim(line) != "DBND1") throw std::invalid_argument("not a save file");
        RunState loaded;
        loaded.players.clear();
        bool haveStage = false, haveLevel = false;
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty()) continue;
            std::vector<std::string> f;
            std::stringstream parts(line);
            std::string part;
            while (std::getline(parts, part, '|')) f.push_back(part);
            if (f[0] == "stage" && f.size() == 2) {
                loaded.stage = toInt(f[1]);
                haveStage = true;
            } else if (f[0] == "level" && f.size() == 2) {
                loaded.level = toInt(f[1]);
                haveLevel = true;
            } else if (f[0] == "player" && f.size() == 6) {
                Unit p;
                p.name = f[1];
                p.kind = f[2];
                p.maxHp = toInt(f[3]);
                p.hp = toInt(f[4]);
                p.shield = toInt(f[5]);
                const ClassInfo* info = data.findClass(p.kind);
                if (info == nullptr) throw std::invalid_argument("unknown class");
                if (p.maxHp < 1 || p.hp < 0 || p.hp > p.maxHp || p.shield < 0) {
                    throw std::invalid_argument("HP out of range");
                }
                p.speed = info->speed;
                p.moves = data.movesOf(p.kind);
                p.index = static_cast<int>(loaded.players.size());
                loaded.players.push_back(p);
            } else {
                throw std::invalid_argument("unknown line");
            }
        }
        size_t count = loaded.players.size();
        if (!haveStage || !haveLevel || count < 1 || count > 3)
            throw std::invalid_argument("missing data");
        if (loaded.stage < 1 || loaded.stage > TOTAL_STAGES || loaded.level < 1 ||
            loaded.level > 7) {
            throw std::invalid_argument("stage or level out of range");
        }
        run = loaded;
        return true;
    } catch (const std::exception&) {
        std::cout << "The save file is damaged and could not be loaded.\n";
        return false;
    }
}

void addHistory(const RunState& run, bool won) {
    std::ofstream file(HISTORY_FILE, std::ios::app);
    if (!file) return;
    std::string who;
    for (const Unit& p : run.players) {
        if (!who.empty()) who += ", ";
        who += p.name + " (" + p.kind + ")";
    }
    file << who << " | " << (won ? "WON" : "LOST") << " | Stage " << run.stage << "/"
         << TOTAL_STAGES << "\n";
}

void showHistory() {
    std::ifstream file(HISTORY_FILE);
    std::string line;
    int count = 0;
    std::cout << "\nRun History\n";
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::cout << ++count << ". " << line << "\n";
    }
    if (count == 0) std::cout << "No finished runs yet.\n";
    pressEnter();
}
