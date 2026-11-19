// Game.cpp - main menu, tutorial, party setup and the stage loop.
// Owner: Matt (Member 1).
#include "Game.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

#include "Battle.h"
#include "Input.h"
#include "Random.h"
#include "SaveManager.h"

namespace {

// The level after clearing this stage (unchanged if the stage gives no level up).
int levelAfterStage(int stage, int current) {
    auto entry = LEVEL_AFTER_STAGE.find(stage);
    return entry == LEVEL_AFTER_STAGE.end() ? current : entry->second;
}

Unit makePlayer(const GameData& data, const std::string& name, const ClassInfo& info, int index) {
    Unit p;
    p.name = name;
    p.kind = info.name;
    p.index = index;
    p.maxHp = p.hp = info.hp;
    p.speed = info.speed;
    p.moves = data.movesOf(info.name);
    return p;
}

// Asks for the class of one player. Returns -1 if the player chose Back.
int chooseClass(const GameData& data, const std::string& name) {
    std::cout << "Welcome, " << name << ". Choose your class.\n";
    for (size_t i = 0; i < data.classes.size(); ++i) {
        std::cout << "[" << i + 1 << "] " << data.classes[i].name << "\n";
    }
    int back = static_cast<int>(data.classes.size()) + 1;
    std::cout << "[" << back << "] Back\n";
    int pick = readInt(1, back);
    return pick == back ? -1 : pick - 1;
}

// Party setup for 1 to 3 players. Returns false if the player backed out.
bool setupParty(const GameData& data, int count, RunState& run) {
    std::vector<std::string> names;
    std::vector<int> classes;
    for (int i = 0; i < count;) {
        if (static_cast<int>(names.size()) == i) names.push_back(readName(i + 1, names));
        int c = chooseClass(data, names[static_cast<size_t>(i)]);
        if (c < 0) {
            names.pop_back();  // Back returns to this player's name entry
            if (i == 0) return false;
            continue;
        }
        if (static_cast<int>(classes.size()) > i)
            classes[static_cast<size_t>(i)] = c;
        else
            classes.push_back(c);
        ++i;
        if (i == count) {
            std::cout << "\nParty Setup:\n";
            for (int k = 0; k < count; ++k) {
                std::cout << names[static_cast<size_t>(k)] << " - "
                          << data.classes[static_cast<size_t>(classes[static_cast<size_t>(k)])].name
                          << "\n";
            }
            std::cout << "Confirm?\n[Y/N]\n";
            if (!readYesNo()) {  // N: everybody picks a class again, names are kept
                i = 0;
                classes.clear();
            }
        }
    }
    run = RunState();
    for (int k = 0; k < count; ++k) {
        run.players.push_back(
            makePlayer(data, names[static_cast<size_t>(k)],
                       data.classes[static_cast<size_t>(classes[static_cast<size_t>(k)])], k));
    }
    return true;
}

// Heals everybody (this revives downed players), then levels up.
void clearStage(RunState& run) {
    std::cout << "\nStage " << run.stage << "/" << TOTAL_STAGES << " cleared.\n";
    for (Unit& p : run.players) {
        int heal = STAGE_HEAL_FLAT + p.maxHp * STAGE_HEAL_PERCENT / 100;
        bool wasDown = !p.alive();
        int gained = std::min(heal, p.maxHp - p.hp);
        p.hp += gained;
        std::cout << p.name << (wasDown ? " was revived and recovered " : " recovered ") << gained
                  << " HP (HP: " << p.hp << "/" << p.maxHp << ")\n";
    }
    int next = levelAfterStage(run.stage, run.level);
    if (next != run.level) {
        run.level = next;
        for (Unit& p : run.players) {
            p.maxHp += PLAYER_HP_PER_LEVEL;
            p.hp += PLAYER_HP_PER_LEVEL;
            std::cout << p.name << " leveled up to LVL " << run.level << "! " << p.name
                      << " gained " << PLAYER_HP_PER_LEVEL << " MAX HP.\n";
        }
        if (run.level == NEW_MOVE_LEVEL) std::cout << "Everyone learned their 4th move!\n";
    }
}

// Plays from run.stage to the end. Saves after every cleared stage.
void playRun(const GameData& data, RunState& run) {
    int partySize = static_cast<int>(run.players.size());
    while (run.stage <= TOTAL_STAGES) {
        bool cleared = playStage(data, run, partySize);
        if (!cleared) {
            std::cout << "\nYour party has fallen on Stage " << run.stage << "/" << TOTAL_STAGES
                      << ".\n";
            addHistory(run, false);
            deleteSave();
            pressEnter("Press Enter to return to the main menu.");
            return;
        }
        clearStage(run);
        if (run.stage == TOTAL_STAGES) {
            std::cout << "You win!\n";
            addHistory(run, true);
            deleteSave();
            pressEnter("Press Enter to return to the main menu.");
            return;
        }
        ++run.stage;
        if (!saveRun(run)) std::cout << "Warning: the game could not be saved.\n";
        while (true) {
            std::cout << "Move to next stage? (Y/N)\n";
            if (readYesNo()) break;
            std::cout << "Return to the main menu? Your progress is saved. Use Continue to resume. "
                         "(Y/N)\n";
            if (readYesNo()) return;
        }
    }
}

void playMenu(const GameData& data) {
    while (true) {
        std::cout << "\nSelect Play Mode\n[1] Single-player\n[2] Two-Player\n[3] Multiplayer "
                     "(3-player)\n[4] Back\n";
        int mode = readInt(1, 4);
        if (mode == 4) return;
        if (saveExists()) {
            std::cout << "A saved game exists and will be replaced. Continue? (Y/N)\n";
            if (!readYesNo()) continue;
        }
        RunState run;
        if (!setupParty(data, mode, run)) continue;
        playRun(data, run);
        return;
    }
}

void tutorial(const GameData& data) {
    while (true) {
        std::cout << "\nWhat do you want to know?\n[1] Types\n[2] Classes\n[3] Back\n";
        int pick = readInt(1, 3);
        if (pick == 3) return;
        if (pick == 1) {
            std::cout << "\nAbout Types:\n"
                         "Blunt moves deal 2x damage to Solid-type enemies.\n"
                         "Piercing moves deal 2x damage to Heavy-type enemies.\n"
                         "Mystic moves deal 2x damage to Spirit-type enemies.\n"
                         "Otherwise, moves deal the normal 1x damage.\n\n";
            pressEnter();
            continue;
        }
        while (true) {
            std::cout << "\nWhich class do you want to know about?\n";
            for (size_t i = 0; i < data.classes.size(); ++i) {
                std::cout << "[" << i + 1 << "] " << data.classes[i].name << "\n";
            }
            int back = static_cast<int>(data.classes.size()) + 1;
            std::cout << "[" << back << "] Back\n";
            int c = readInt(1, back);
            if (c == back) break;
            const ClassInfo& info = data.classes[static_cast<size_t>(c) - 1];
            std::cout << "\n"
                      << info.description << "\nStats (LVL 1):\nHP: " << info.hp
                      << "\nSpeed: " << info.speed << "\nMovesets:\n";
            for (const Move& m : data.movesOf(info.name)) {
                std::cout << describeMove(m);
                if (m.learnLevel > 1) std::cout << " (learned at LVL " << m.learnLevel << ")";
                std::cout << "\n";
            }
            std::cout << "\n";
            pressEnter();
        }
    }
}

}  // namespace

void mainMenu(const GameData& data) {
    while (true) {
        std::cout << "\nWelcome to Dungeons But No Dragons!\n"
                     "[1] Play\n[2] Continue\n[3] Tutorial\n[4] Run History\n[5] Exit\n";
        int pick = readInt(1, 5);
        if (pick == 1)
            playMenu(data);
        else if (pick == 2) {
            RunState run;
            if (loadRun(run, data)) {
                std::cout << "Game loaded. Resuming at Stage " << run.stage << "/" << TOTAL_STAGES
                          << ".\n";
                playRun(data, run);
            }
        } else if (pick == 3)
            tutorial(data);
        else if (pick == 4)
            showHistory();
        else {
            std::cout << "Thank you for playing!\n";
            return;
        }
    }
}
