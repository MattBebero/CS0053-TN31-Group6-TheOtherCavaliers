// Types.h - shared data types and game constants.
// Owner: Matt (Member 1).
#pragma once

#include <map>
#include <random>
#include <string>
#include <vector>

// ---- Game constants (the numbers from the design reference) ----
const int TOTAL_STAGES = 10;
const int MAX_MP = 6;
const int DOT_DAMAGE = 3;          // Poison, Burn and Bleed damage per turn
const int LIFESTEAL_PERCENT = 20;  // of the damage dealt, per hit
const int PLAYER_HP_PER_LEVEL = 3;
const int ENEMY_HP_PER_LEVEL = 2;
const int STAGE_HEAL_FLAT = 7;
const int STAGE_HEAL_PERCENT = 10;   // of max HP
const int HIGHER_COUNT_CHANCE = 40;  // % chance of one extra enemy
const int MAX_NAME_LENGTH = 16;
const int NEW_MOVE_LEVEL = 5;  // level at which the 4th move is learned

// Stage cleared -> new level. Stages 4, 6, 8 and 10 give no level up.
const std::map<int, int> LEVEL_AFTER_STAGE = {{1, 2}, {2, 3}, {3, 4}, {5, 5}, {7, 6}, {9, 7}};

enum class DmgType { None, Blunt, Piercing, Mystic };
enum class EnemyType { Solid, Heavy, Spirit };
enum class Target { One, All, Adjacent, Ally, Allies, Self };

// Everything a move can do besides plain damage.
struct Effect {
    int poison = 0, burn = 0, bleed = 0;  // turns applied to the target
    int taunt = 0, shield = 0;            // Taunt: turns and Shield gained
    int lifesteal = 0, selfHeal = 0;      // turns of Lifesteal, HP healed on self
    bool aim = false, focus = false, inspire = false, heal = false;
};

struct Move {
    std::string owner, name;
    int power = 0;
    DmgType type = DmgType::None;
    int mp = 0, cooldown = 0, learnLevel = 1;
    Target target = Target::One;
    Effect effect;
};

struct ClassInfo {
    std::string name;
    int hp = 0, speed = 0;
    std::string description;
};

struct EnemyInfo {
    std::string name;
    EnemyType type = EnemyType::Solid;
    int hp = 0, baseLevel = 1, speed = 0;
    bool boss = false;
    std::string description;
};

struct GameData {
    std::vector<ClassInfo> classes;
    std::vector<EnemyInfo> enemies;
    std::vector<Move> moves;

    const ClassInfo* findClass(const std::string& name) const;
    const EnemyInfo* findEnemy(const std::string& name) const;
    std::vector<Move> movesOf(const std::string& owner) const;
};

// A player or an enemy in battle.
struct Unit {
    std::string name;  // player name, or enemy name such as "Slime 2"
    std::string kind;  // class name or enemy name
    bool isEnemy = false;
    int index = 0;  // position in its group (0 = Player 1 / first enemy)
    EnemyType enemyType = EnemyType::Solid;
    int maxHp = 0, hp = 0, speed = 0, mp = 0, shield = 0;
    int poison = 0, burn = 0, bleed = 0, taunt = 0, lifesteal = 0;
    int tauntedBy = -1;  // player index that taunted this enemy
    bool aim = false, focus = false, inspire = false;
    std::map<std::string, int> cooldown;
    std::vector<Move> moves;

    bool alive() const { return hp > 0; }
};

// What the game remembers between stages (also what is saved to disk).
struct RunState {
    int stage = 1;  // the next stage to play
    int level = 1;
    std::vector<Unit> players;
};

std::mt19937& rng();
int randomPercent();  // 0..99
