// DataLoader.cpp - reads the pipe-delimited data files in data/.
// Owner: Al Noor (Member 3).
//
// The parse helpers throw std::invalid_argument with a readable message and
// the file reader catches it and reports the file and line.
#include "DataLoader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "Input.h"

const ClassInfo* GameData::findClass(const std::string& name) const {
    for (const ClassInfo& c : classes) {
        if (c.name == name) return &c;
    }
    return nullptr;
}

const EnemyInfo* GameData::findEnemy(const std::string& name) const {
    for (const EnemyInfo& e : enemies) {
        if (e.name == name) return &e;
    }
    return nullptr;
}

std::vector<Move> GameData::movesOf(const std::string& owner) const {
    std::vector<Move> result;
    for (const Move& m : moves) {
        if (m.owner == owner) result.push_back(m);
    }
    return result;
}

static int toInt(const std::string& text, const std::string& field) {
    int value = 0;
    if (!parseInt(text, value)) {
        throw std::invalid_argument(field + " must be a whole number, got \"" + text + "\"");
    }
    return value;
}

static DmgType toDmgType(const std::string& text) {
    if (text == "None") return DmgType::None;
    if (text == "Blunt") return DmgType::Blunt;
    if (text == "Piercing") return DmgType::Piercing;
    if (text == "Mystic") return DmgType::Mystic;
    throw std::invalid_argument("unknown damage type \"" + text + "\"");
}

static EnemyType toEnemyType(const std::string& text) {
    if (text == "Solid") return EnemyType::Solid;
    if (text == "Heavy") return EnemyType::Heavy;
    if (text == "Spirit") return EnemyType::Spirit;
    throw std::invalid_argument("unknown enemy type \"" + text + "\"");
}

static Target toTarget(const std::string& text) {
    if (text == "one") return Target::One;
    if (text == "all") return Target::All;
    if (text == "adjacent") return Target::Adjacent;
    if (text == "ally") return Target::Ally;
    if (text == "allies") return Target::Allies;
    if (text == "self") return Target::Self;
    throw std::invalid_argument("unknown target \"" + text + "\"");
}

static Effect toEffect(const std::string& text) {
    Effect e;
    std::stringstream parts(text);
    std::string part;
    while (std::getline(parts, part, ',')) {
        std::string key = part, value;
        size_t colon = part.find(':');
        if (colon != std::string::npos) {
            key = part.substr(0, colon);
            value = part.substr(colon + 1);
        }
        if (key == "none") continue;
        if (key == "aim")
            e.aim = true;
        else if (key == "focus")
            e.focus = true;
        else if (key == "inspire")
            e.inspire = true;
        else if (key == "heal")
            e.heal = true;
        else if (key == "poison")
            e.poison = toInt(value, "poison turns");
        else if (key == "burn")
            e.burn = toInt(value, "burn turns");
        else if (key == "bleed")
            e.bleed = toInt(value, "bleed turns");
        else if (key == "taunt")
            e.taunt = toInt(value, "taunt turns");
        else if (key == "shield")
            e.shield = toInt(value, "shield");
        else if (key == "lifesteal")
            e.lifesteal = toInt(value, "lifesteal turns");
        else if (key == "selfheal")
            e.selfHeal = toInt(value, "selfheal");
        else
            throw std::invalid_argument("unknown effect \"" + key + "\"");
    }
    return e;
}

// Like toInt, but the number must be 1 or more (HP, speed and levels).
static int toPositiveInt(const std::string& text, const std::string& field) {
    int value = toInt(text, field);
    if (value < 1) {
        throw std::invalid_argument(field + " must be 1 or more, got " + std::to_string(value));
    }
    return value;
}

static std::vector<std::string> splitLine(const std::string& line, size_t fieldCount) {
    std::vector<std::string> fields;
    std::stringstream parts(line);
    std::string part;
    while (std::getline(parts, part, '|')) fields.push_back(trim(part));
    if (fields.size() != fieldCount) {
        throw std::invalid_argument("expected " + std::to_string(fieldCount) +
                                    " fields but found " + std::to_string(fields.size()));
    }
    return fields;
}

// Calls parseLine for every data line (blank lines and # comments are skipped).
template <typename Parser>
static bool readFile(const std::string& path, Parser parseLine) {
    std::ifstream file(path);
    if (!file) {
        std::cout << "Cannot open " << path << ".\n";
        return false;
    }
    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        try {
            parseLine(line);
        } catch (const std::exception& error) {
            std::cout << path << " line " << lineNumber << ": " << error.what() << "\n";
            return false;
        }
    }
    return true;
}

bool loadGameData(GameData& data, const std::string& folder) {
    bool ok = readFile(folder + "/classes.txt", [&](const std::string& line) {
        std::vector<std::string> f = splitLine(line, 4);
        data.classes.push_back(
            {f[0], toPositiveInt(f[1], "HP"), toPositiveInt(f[2], "Speed"), f[3]});
    });
    ok = ok && readFile(folder + "/enemies.txt", [&](const std::string& line) {
             std::vector<std::string> f = splitLine(line, 7);
             EnemyInfo e;
             e.name = f[0];
             e.type = toEnemyType(f[1]);
             e.hp = toPositiveInt(f[2], "HP");
             e.baseLevel = toPositiveInt(f[3], "Base level");
             e.speed = toPositiveInt(f[4], "Speed");
             e.boss = toInt(f[5], "Boss") == 1;
             e.description = f[6];
             data.enemies.push_back(e);
         });
    ok = ok && readFile(folder + "/moves.txt", [&](const std::string& line) {
             std::vector<std::string> f = splitLine(line, 9);
             Move m;
             m.owner = f[0];
             m.name = f[1];
             m.power = toInt(f[2], "Power");
             m.type = toDmgType(f[3]);
             m.mp = toInt(f[4], "MP");
             m.cooldown = toInt(f[5], "Cooldown");
             m.learnLevel = toInt(f[6], "Learned at level");
             m.target = toTarget(f[7]);
             m.effect = toEffect(f[8]);
             data.moves.push_back(m);
         });
    if (!ok) return false;
    if (data.classes.empty() || data.enemies.empty() || data.moves.empty()) {
        std::cout << "A data file has no entries.\n";
        return false;
    }
    return true;
}
