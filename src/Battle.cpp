// Battle.cpp - one stage of combat: turns, damage, statuses and enemy AI.
// Owner: Lee Jedrek (Member 2).
#include "Battle.h"

#include <algorithm>
#include <iostream>

#include "Input.h"

namespace {

struct Action {
    Unit* actor = nullptr;
    const Move* move = nullptr;  // nullptr means Recharge (players) or "enemy decides later"
    int target = -1;
    bool recharge = false;
};

struct Weighted {
    const char* name;
    int weight;
};

// Who can appear on each group of stages (percent chances).
const std::vector<Weighted> EARLY = {{"Slime", 60}, {"Wasp", 40}};
const std::vector<Weighted> MIDDLE = {{"Slime", 35}, {"Wasp", 35}, {"Bandit", 30}};
const std::vector<Weighted> LATE = {{"Slime", 30}, {"Wasp", 30}, {"Bandit", 40}};

// Enemy HP multiplier (x10) for 1, 2 and 3 players.
const int ENEMY_HP_MULT10[] = {10, 20, 30};

std::string label(const Unit& u) { return u.isEnemy ? "Enemy " + u.name : u.name; }

std::string hpText(const Unit& u) {
    std::string text = "HP: " + std::to_string(u.hp) + "/" + std::to_string(u.maxHp);
    if (u.shield > 0) text += ". Shield: " + std::to_string(u.shield);
    return text;
}

// Applies damage to the Shield first, then HP. Returns the damage that got past the Shield.
int applyDamage(Unit& u, int damage) {
    int absorbed = std::min(u.shield, damage);
    u.shield -= absorbed;
    int through = std::min(u.hp, damage - absorbed);  // excess damage is removed
    u.hp -= through;
    return through;
}

// Heals up to max HP. Returns the HP actually recovered (excess is removed).
int healUnit(Unit& u, int amount) {
    int gained = std::min(amount, u.maxHp - u.hp);
    u.hp += gained;
    return gained;
}

std::string statusText(const Unit& u, const std::vector<Unit>& players) {
    std::string text;
    if (!u.alive()) return " [Down]";
    if (u.shield > 0) text += " [Shield " + std::to_string(u.shield) + "]";
    if (u.poison > 0) text += " [Poison " + std::to_string(u.poison) + "]";
    if (u.burn > 0) text += " [Burn " + std::to_string(u.burn) + "]";
    if (u.bleed > 0) text += " [Bleed " + std::to_string(u.bleed) + "]";
    if (u.taunt > 0 && u.tauntedBy >= 0) {
        text += " [Taunted by " + players[u.tauntedBy].name + " " + std::to_string(u.taunt) + "]";
    }
    if (u.aim) text += " [Aim]";
    if (u.focus) text += " [Mana Focus]";
    if (u.inspire) text += " [Inspire]";
    if (u.lifesteal > 0) text += " [Lifesteal " + std::to_string(u.lifesteal) + "]";
    return text;
}

void printSummary(int turn, const std::vector<Unit>& players, const std::vector<Unit>& enemies) {
    std::cout << "\nTurn " << turn << ":\n";
    for (const Unit& u : players) {
        std::cout << u.name << " - HP: " << u.hp << "/" << u.maxHp << ". MP: " << u.mp << "/"
                  << MAX_MP << "." << statusText(u, players) << "\n";
    }
    for (const Unit& u : enemies) {
        if (!u.alive()) continue;
        std::cout << label(u) << " - HP: " << u.hp << "/" << u.maxHp << ". MP: " << u.mp << "/"
                  << MAX_MP << "." << statusText(u, players) << "\n";
    }
    std::cout << "\n";
}

const char* typeName(DmgType t) {
    switch (t) {
        case DmgType::Blunt:
            return "Blunt";
        case DmgType::Piercing:
            return "Piercing";
        case DmgType::Mystic:
            return "Mystic";
        default:
            return "";
    }
}

// Blunt beats Solid, Piercing beats Heavy, Mystic beats Spirit.
bool superEffective(DmgType attack, EnemyType defender) {
    return (attack == DmgType::Blunt && defender == EnemyType::Solid) ||
           (attack == DmgType::Piercing && defender == EnemyType::Heavy) ||
           (attack == DmgType::Mystic && defender == EnemyType::Spirit);
}

bool anyAlive(const std::vector<Unit>& group) {
    for (const Unit& u : group) {
        if (u.alive()) return true;
    }
    return false;
}

int firstAlive(const std::vector<Unit>& group) {
    for (const Unit& u : group) {
        if (u.alive()) return u.index;
    }
    return -1;
}

int countAlive(const std::vector<Unit>& group) {
    int n = 0;
    for (const Unit& u : group) n += u.alive() ? 1 : 0;
    return n;
}

// ---------------------------------------------------------------- spawning

int pickWeighted(const std::vector<Weighted>& table, int& outIndex) {
    int roll = randomPercent();
    int total = 0;
    for (size_t i = 0; i < table.size(); ++i) {
        total += table[i].weight;
        if (roll < total) {
            outIndex = static_cast<int>(i);
            return outIndex;
        }
    }
    outIndex = static_cast<int>(table.size()) - 1;
    return outIndex;
}

Unit makeEnemy(const GameData& data, const std::string& kind, int level, int partySize) {
    const EnemyInfo* info = data.findEnemy(kind);
    Unit e;
    e.kind = kind;
    e.name = kind;
    e.isEnemy = true;
    e.enemyType = info->type;
    e.speed = info->speed;
    int base = info->hp + std::max(0, level - info->baseLevel) * ENEMY_HP_PER_LEVEL;
    e.maxHp = std::max(1, base * ENEMY_HP_MULT10[partySize - 1] / 10);  // rounded down
    e.hp = e.maxHp;
    e.moves = data.movesOf(kind);
    return e;
}

std::vector<Unit> spawnEnemies(const GameData& data, int stage, int level, int partySize) {
    std::vector<Unit> enemies;
    if (stage == TOTAL_STAGES) {
        std::vector<std::string> bosses;
        for (const EnemyInfo& e : data.enemies) {
            if (e.boss) bosses.push_back(e.name);
        }
        enemies.push_back(
            makeEnemy(data, bosses[randomPercent() % bosses.size()], level, partySize));
    } else {
        const std::vector<Weighted>& table = stage <= 3 ? EARLY : (stage <= 6 ? MIDDLE : LATE);
        int count = partySize + (randomPercent() < HIGHER_COUNT_CHANCE ? 1 : 0);
        for (int i = 0; i < count; ++i) {
            int pick = 0;
            pickWeighted(table, pick);
            enemies.push_back(makeEnemy(data, table[pick].name, level, partySize));
        }
        // Same-name enemies get numbers: Slime 1, Slime 2.
        for (Unit& e : enemies) {
            int same = 0;
            for (const Unit& other : enemies) same += other.kind == e.kind ? 1 : 0;
            if (same > 1) {
                int number = 1;
                for (const Unit& other : enemies) {
                    if (&other == &e) break;
                    number += other.kind == e.kind ? 1 : 0;
                }
                e.name += " " + std::to_string(number);
            }
        }
    }
    for (size_t i = 0; i < enemies.size(); ++i) enemies[i].index = static_cast<int>(i);
    return enemies;
}

void resetForStage(Unit& p) {
    p.mp = 0;
    p.cooldown.clear();
    p.poison = p.burn = p.bleed = p.taunt = p.lifesteal = 0;
    p.tauntedBy = -1;
    p.aim = p.focus = p.inspire = false;
    // Shield is kept: it never expires.
}

// ---------------------------------------------------------------- acting

void applyStatus(Unit& target, const Effect& fx) {
    if (fx.poison > 0) {
        target.poison = fx.poison;
        std::cout << label(target) << " is now poisoned for " << fx.poison << " turn/s.\n";
    }
    if (fx.burn > 0) {
        target.burn = fx.burn;
        std::cout << label(target) << " is now burning for " << fx.burn << " turn/s.\n";
    }
    if (fx.bleed > 0) {
        target.bleed = fx.bleed;
        std::cout << label(target) << " is now bleeding for " << fx.bleed << " turn/s.\n";
    }
}

void announceDefeat(const Unit& u) {
    if (u.isEnemy)
        std::cout << label(u) << " has been defeated.\n";
    else
        std::cout << u.name << " has fallen.\n";
}

void dealDamage(Unit& actor, const Move& mv, std::vector<Unit*>& targets) {
    // Move multipliers (Aim, Mana Focus, Inspire) come first, type effectiveness after.
    int percent = 100;
    if (actor.aim && mv.name == "Shoot") percent *= 2;
    if (actor.focus) percent *= 2;
    if (actor.inspire) percent = percent * 125 / 100;

    bool anySuper = false;
    for (Unit* t : targets) {
        if (t->isEnemy && superEffective(mv.type, t->enemyType)) anySuper = true;
    }
    if (anySuper) std::cout << "Super effective!\n";

    for (Unit* t : targets) {
        if (!t->alive()) continue;
        int typeMult = (t->isEnemy && superEffective(mv.type, t->enemyType)) ? 2 : 1;
        int damage = std::max(1, mv.power * percent * typeMult / 100);  // rounded down, at least 1
        int through = applyDamage(*t, damage);
        std::cout << label(actor) << " dealt " << through << " DMG on " << label(*t) << " ("
                  << hpText(*t) << ").\n";
        if (through > 0 && t->alive()) applyStatus(*t, mv.effect);
        if (actor.lifesteal > 0 && through > 0) {
            int back = healUnit(actor, through * LIFESTEAL_PERCENT / 100);
            if (back > 0)
                std::cout << label(actor) << " recovered " << back << " HP from Lifesteal.\n";
        }
        if (!t->alive()) announceDefeat(*t);
    }
    // The buffs are used up by a damaging move.
    actor.focus = false;
    actor.inspire = false;
    if (mv.name == "Shoot") actor.aim = false;
}

void performMove(Unit& actor, const Move& mv, int targetIndex, std::vector<Unit>& players,
                 std::vector<Unit>& enemies) {
    std::vector<Unit>& foes = actor.isEnemy ? players : enemies;
    std::vector<Unit>& friends = actor.isEnemy ? enemies : players;

    actor.mp -= mv.mp;
    if (mv.cooldown > 0) actor.cooldown[mv.name] = mv.cooldown;
    if (mv.mp > 0) {
        std::cout << label(actor) << " spent " << mv.mp << " MP to use " << mv.name << ".\n";
    } else {
        std::cout << label(actor) << " used " << mv.name << ".\n";
    }

    const Effect& fx = mv.effect;
    if (mv.power > 0 && !fx.heal) {
        std::vector<Unit*> targets;
        if (mv.target == Target::All) {
            for (Unit& t : foes) {
                if (t.alive()) targets.push_back(&t);
            }
        } else if (mv.target == Target::Adjacent) {
            std::vector<Unit*> alive;
            for (Unit& t : foes) {
                if (t.alive()) alive.push_back(&t);
            }
            for (size_t i = 0; i < alive.size(); ++i) {
                if (alive[i]->index == targetIndex) {
                    if (i > 0) targets.push_back(alive[i - 1]);
                    targets.push_back(alive[i]);
                    if (i + 1 < alive.size()) targets.push_back(alive[i + 1]);
                }
            }
        } else {
            targets.push_back(&foes[targetIndex]);
        }
        dealDamage(actor, mv, targets);
    }
    if (fx.taunt > 0 && !actor.isEnemy) {
        actor.shield = fx.shield;  // never stacks, always refreshes
        std::cout << actor.name << " gained " << fx.shield << " Shield.\n";
        Unit& t = foes[targetIndex];
        t.taunt = fx.taunt;  // the latest Taunt replaces the earlier one
        t.tauntedBy = actor.index;
        std::cout << label(t) << " is now targeting " << actor.name << " for " << fx.taunt
                  << " turn/s.\n";
    }
    if (fx.aim) {
        actor.aim = true;
        std::cout << actor.name << "'s next \"Shoot\" will deal x2 DMG.\n";
    }
    if (fx.focus) {
        actor.focus = true;
        std::cout << actor.name << "'s next damaging move will deal x2 DMG.\n";
    }
    if (fx.inspire) {
        Unit& t = friends[targetIndex];
        t.inspire = true;  // refreshes, never stacks
        std::cout << t.name << "'s next damaging move will deal x1.25 DMG.\n";
    }
    if (fx.heal) {
        int amount = mv.power;
        std::vector<Unit*> targets;
        if (mv.target == Target::Allies) {
            for (Unit& f : friends) {
                if (f.alive()) targets.push_back(&f);
            }
        } else if (friends[targetIndex].alive()) {
            targets.push_back(&friends[targetIndex]);
        }
        for (Unit* t : targets) {
            int gained = healUnit(*t, amount);
            std::cout << t->name << " recovered " << gained << " HP (HP: " << t->hp << "/"
                      << t->maxHp << ").\n";
        }
    }
    if (fx.selfHeal > 0) {
        int gained = healUnit(actor, fx.selfHeal);
        std::cout << label(actor) << " recovered " << gained << " HP (HP: " << actor.hp << "/"
                  << actor.maxHp << ").\n";
    }
    if (fx.lifesteal > 0) {
        actor.lifesteal = fx.lifesteal;
        std::cout << label(actor) << " gained Lifesteal for " << fx.lifesteal << " turn/s.\n";
    }
}

// ---------------------------------------------------------------- enemy AI

// The enemy uses the move with the highest MP cost it can afford. Recover is
// skipped while the enemy is at or above 50% HP.
const Move* chooseEnemyMove(const Unit& e) {
    const Move* best = nullptr;
    for (const Move& m : e.moves) {
        auto cd = e.cooldown.find(m.name);
        if (m.mp > e.mp || (cd != e.cooldown.end() && cd->second > 0)) continue;
        if (m.effect.selfHeal > 0 && e.hp * 2 >= e.maxHp) continue;
        if (best == nullptr || m.mp > best->mp) best = &m;
    }
    return best;
}

int chooseEnemyTarget(const Unit& e, const std::vector<Unit>& players) {
    if (e.taunt > 0 && e.tauntedBy >= 0 && players[e.tauntedBy].alive()) return e.tauntedBy;
    std::vector<int> alive;
    for (const Unit& p : players) {
        if (p.alive()) alive.push_back(p.index);
    }
    return alive[static_cast<size_t>(randomPercent()) % alive.size()];
}

// ---------------------------------------------------------------- player input

void printMoveInfo(const Unit& p, int level) {
    for (const Move& m : p.moves) {
        if (m.learnLevel <= level) std::cout << describeMove(m) << "\n\n";
    }
    pressEnter();
}

void printEnemyInfo(const GameData& data, const std::vector<Unit>& enemies) {
    std::vector<std::string> shown;
    for (const Unit& e : enemies) {
        if (!e.alive() || std::find(shown.begin(), shown.end(), e.kind) != shown.end()) continue;
        shown.push_back(e.kind);
        const EnemyInfo* info = data.findEnemy(e.kind);
        const char* type = info->type == EnemyType::Solid
                               ? "Solid"
                               : (info->type == EnemyType::Heavy ? "Heavy" : "Spirit");
        std::cout << e.kind << " - " << info->description << " " << type << " type\n";
        std::cout << "HP: " << e.maxHp << "\nSpeed: " << e.speed << "\nMoves:\n";
        for (const Move& m : e.moves) std::cout << describeMove(m) << "\n";
        std::cout << "\n";
    }
    pressEnter();
}

// Returns the index of the chosen target, or -1 if the player went back.
int chooseTarget(const Move& mv, const Unit& actor, const std::vector<Unit>& players,
                 const std::vector<Unit>& enemies) {
    if (mv.target == Target::One || mv.target == Target::Adjacent) {
        if (countAlive(enemies) == 1) return firstAlive(enemies);
        std::cout << "Choose a target:\n";
        std::vector<int> ids;
        for (const Unit& e : enemies) {
            if (!e.alive()) continue;
            ids.push_back(e.index);
            std::cout << "[" << ids.size() << "] " << label(e) << " (" << hpText(e) << ")\n";
        }
        return ids[static_cast<size_t>(readInt(1, static_cast<int>(ids.size()))) - 1];
    }
    if (mv.target == Target::Ally) {
        if (countAlive(players) == 1) return actor.index;
        std::cout << "Choose an ally:\n";
        std::vector<int> ids;
        for (const Unit& p : players) {
            if (!p.alive()) continue;
            ids.push_back(p.index);
            std::cout << "[" << ids.size() << "] " << p.name << " (" << hpText(p) << ")\n";
        }
        return ids[static_cast<size_t>(readInt(1, static_cast<int>(ids.size()))) - 1];
    }
    return actor.index;  // All, Allies and Self need no choice
}

Action chooseAction(const GameData& data, Unit& actor, int turn, int level,
                    const std::vector<Unit>& players, const std::vector<Unit>& enemies) {
    while (true) {
        std::cout << "Turn " << turn << ": " << actor.name << "'s Turn.\n"
                  << "[1] Attack\n[2] Recharge\n[3] Move Info\n[4] Enemy Info\n";
        int choice = readInt(1, 4);
        if (choice == 2) {
            Action a;
            a.actor = &actor;
            a.recharge = true;
            return a;
        }
        if (choice == 3) {
            printMoveInfo(actor, level);
            continue;
        }
        if (choice == 4) {
            printEnemyInfo(data, enemies);
            continue;
        }
        // Attack: list the learned moves.
        std::vector<const Move*> usable;
        for (const Move& m : actor.moves) {
            if (m.learnLevel <= level) usable.push_back(&m);
        }
        std::cout << "Choose a move:\n";
        for (size_t i = 0; i < usable.size(); ++i) {
            int cd = actor.cooldown.count(usable[i]->name) ? actor.cooldown.at(usable[i]->name) : 0;
            std::cout << "[" << i + 1 << "] " << usable[i]->name;
            if (cd > 0) std::cout << " (Cooldown: " << cd << " turns)";
            std::cout << "\n";
        }
        std::cout << "[" << usable.size() + 1 << "] Back\n";
        int pick = readInt(1, static_cast<int>(usable.size()) + 1);
        if (pick == static_cast<int>(usable.size()) + 1) continue;
        const Move& mv = *usable[static_cast<size_t>(pick) - 1];
        int cd = actor.cooldown.count(mv.name) ? actor.cooldown.at(mv.name) : 0;
        if (cd > 0) {
            std::cout << "Can't use '" << mv.name << "'. It is in cooldown for " << cd
                      << " turns.\n";
            continue;
        }
        if (mv.mp > actor.mp) {
            std::cout << "Not enough MP to use '" << mv.name << "' (needs " << mv.mp
                      << ", you have " << actor.mp << ").\n";
            continue;
        }
        Action a;
        a.actor = &actor;
        a.move = &mv;
        a.target = chooseTarget(mv, actor, players, enemies);
        return a;
    }
}

// ---------------------------------------------------------------- end of turn

void endOfTurn(std::vector<Unit>& players, std::vector<Unit>& enemies) {
    auto tick = [&](Unit& u) {
        if (u.alive()) {
            const char* names[] = {"Poison", "Burn", "Bleed"};
            int* timers[] = {&u.poison, &u.burn, &u.bleed};
            for (int i = 0; i < 3; ++i) {
                if (*timers[i] <= 0 || !u.alive()) continue;
                int lost = applyDamage(u, DOT_DAMAGE);  // Shield absorbs damage over time
                std::cout << label(u) << " lost " << lost << " HP due to " << names[i] << ".\n";
                if (!u.alive()) announceDefeat(u);
            }
        }
        if (u.poison > 0) --u.poison;
        if (u.burn > 0) --u.burn;
        if (u.bleed > 0) --u.bleed;
        if (u.lifesteal > 0) --u.lifesteal;
        if (u.taunt > 0 && --u.taunt == 0) u.tauntedBy = -1;
    };
    for (Unit& p : players) tick(p);
    for (Unit& e : enemies) tick(e);
}

void resolveAction(const Action& a, std::vector<Unit>& players, std::vector<Unit>& enemies,
                   int level) {
    Unit& actor = *a.actor;
    if (!actor.alive()) return;  // went down before its turn: the action is skipped
    if (!actor.isEnemy) {
        if (a.recharge) {
            int gained = actor.mp < MAX_MP ? 1 : 0;  // excess MP is removed
            actor.mp += gained;
            std::cout << actor.name << " Recharged.\n"
                      << actor.name << " gained " << gained << " MP.\n";
            return;
        }
        int target = a.target;
        bool targetsEnemy = a.move->target == Target::One || a.move->target == Target::Adjacent;
        if (targetsEnemy && !enemies[target].alive()) target = firstAlive(enemies);  // retarget
        if (target < 0) return;
        performMove(actor, *a.move, target, players, enemies);
        return;
    }
    const Move* mv = chooseEnemyMove(actor);
    if (mv == nullptr) return;
    int target = (mv->target == Target::Self) ? actor.index : chooseEnemyTarget(actor, players);
    performMove(actor, *mv, target, players, enemies);
    (void)level;
}

}  // namespace

std::string describeMove(const Move& m) {
    std::string text = m.name + " - ";
    const Effect& fx = m.effect;
    switch (m.target) {
        case Target::One:
            text += "Hits 1 target. ";
            break;
        case Target::All:
            text += "Hits all enemies. ";
            break;
        case Target::Adjacent:
            text += "Hits the target and the players beside it. ";
            break;
        case Target::Ally:
            text += "Targets 1 ally. ";
            break;
        case Target::Allies:
            text += "Targets all allies. ";
            break;
        case Target::Self:
            text += "Targets itself. ";
            break;
    }
    if (fx.heal)
        text += "Heals " + std::to_string(m.power) + " HP. ";
    else if (m.power > 0) {
        text += "Deals " + std::to_string(m.power);
        text +=
            m.type == DmgType::None ? " DMG. " : std::string(" ") + typeName(m.type) + " Damage. ";
    }
    if (fx.poison) text += "Applies Poison for " + std::to_string(fx.poison) + " TURN/S. ";
    if (fx.burn) text += "Applies Burn for " + std::to_string(fx.burn) + " TURN/S. ";
    if (fx.bleed) text += "Applies Bleed for " + std::to_string(fx.bleed) + " TURN/S. ";
    if (fx.shield) text += "Gain " + std::to_string(fx.shield) + " Shield. ";
    if (fx.taunt)
        text += "The target attacks only you for " + std::to_string(fx.taunt) + " TURN/S. ";
    if (fx.aim) text += "Skips the turn. Your next \"Shoot\" deals x2 DMG. ";
    if (fx.focus) text += "Skips the turn. Your next damaging move deals x2 DMG. ";
    if (fx.inspire) text += "The ally's next damaging move deals x1.25 DMG. ";
    if (fx.lifesteal) text += "Lifesteal for " + std::to_string(fx.lifesteal) + " TURN/S. ";
    if (fx.selfHeal) text += "Heals itself for " + std::to_string(fx.selfHeal) + " HP. ";
    text += "Costs " + std::to_string(m.mp) + " MP. Cooldown: ";
    text += m.cooldown > 0 ? std::to_string(m.cooldown) + " TURNS" : "No CD";
    return text;
}

bool playStage(const GameData& data, RunState& run, int partySize) {
    std::vector<Unit>& players = run.players;
    for (Unit& p : players) resetForStage(p);
    std::vector<Unit> enemies = spawnEnemies(data, run.stage, run.level, partySize);

    std::cout << "\nStage " << run.stage << "/" << TOTAL_STAGES << "\n";
    for (const Unit& p : players)
        std::cout << p.name << " (LVL " << run.level << ") has arrived!\n";
    for (const Unit& e : enemies)
        std::cout << label(e) << " (LVL " << run.level << ") has appeared!\n";

    for (int turn = 1;; ++turn) {
        // Start of turn: +1 MP (max 6), cooldowns tick down, then the summary.
        auto startTurn = [](Unit& u) {
            if (!u.alive()) return;
            u.mp = std::min(MAX_MP, u.mp + 1);
            for (auto& entry : u.cooldown) {
                if (entry.second > 0) --entry.second;
            }
        };
        for (Unit& p : players) startTurn(p);
        for (Unit& e : enemies) startTurn(e);
        printSummary(turn, players, enemies);

        // Every player chooses first (Player 1, 2, 3), then speed decides who acts first.
        std::vector<Action> actions;
        for (Unit& p : players) {
            if (p.alive())
                actions.push_back(chooseAction(data, p, turn, run.level, players, enemies));
        }
        for (Unit& e : enemies) {
            if (e.alive()) {
                Action a;
                a.actor = &e;
                actions.push_back(a);
            }
        }
        std::stable_sort(actions.begin(), actions.end(), [](const Action& a, const Action& b) {
            if (a.actor->speed != b.actor->speed) return a.actor->speed > b.actor->speed;
            if (a.actor->isEnemy != b.actor->isEnemy) return !a.actor->isEnemy;
            return a.actor->index < b.actor->index;
        });
        std::cout << "\n";
        for (const Action& a : actions) {
            if (!anyAlive(enemies) || !anyAlive(players)) break;
            resolveAction(a, players, enemies, run.level);
        }
        if (anyAlive(enemies) && anyAlive(players)) endOfTurn(players, enemies);
        if (!anyAlive(enemies)) return true;
        if (!anyAlive(players)) return false;
    }
}