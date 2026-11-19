// Input.cpp - safe keyboard input.
// Owner: Al Noor (Member 3).
//
// We read a whole line with getline and convert it ourselves. "cin >> number"
// fails on text like "abc" and leaves it in the buffer, which easily causes an
// endless loop.
#include "Input.h"

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "Types.h"

static std::string readLine() {
    std::string line;
    if (!std::getline(std::cin, line)) {
        std::cout << "\nInput ended. Thank you for playing.\n";
        std::exit(0);  // never loop forever when the input is gone
    }
    return line;
}

std::string trim(const std::string& text) {
    size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) ++start;
    size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) --end;
    return text.substr(start, end - start);
}

std::string toLower(std::string text) {
    for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

bool parseInt(const std::string& text, int& value) {
    try {
        size_t used = 0;
        value = std::stoi(text, &used);
        return used == text.size();  // "12abc" is rejected, not read as 12
    } catch (const std::exception&) {
        return false;  // empty, not a number, or out of int range
    }
}

int readInt(int min, int max) {
    while (true) {
        std::cout << "> ";
        std::string line = trim(readLine());
        bool digitsOnly = !line.empty() && line.size() <= 9;  // 9 digits cannot overflow int
        for (char c : line) {
            if (!std::isdigit(static_cast<unsigned char>(c))) digitsOnly = false;
        }
        if (digitsOnly) {
            int value = std::stoi(line);
            if (value >= min && value <= max) return value;
        }
        std::cout << "Invalid Input. Enter an input corresponding to a valid option\n";
    }
}

bool readYesNo() {
    while (true) {
        std::cout << "> ";
        std::string line = toLower(trim(readLine()));
        if (line == "y" || line == "yes") return true;
        if (line == "n" || line == "no") return false;
        std::cout << "Invalid Input. Enter Y or N\n";
    }
}

std::string checkName(const std::string& name, const std::vector<std::string>& takenNames) {
    if (name.empty()) return "Name cannot be empty.";
    if (static_cast<int>(name.size()) > MAX_NAME_LENGTH) {
        return "Name must be " + std::to_string(MAX_NAME_LENGTH) + " characters or fewer.";
    }
    if (name.find('|') != std::string::npos) return "Name cannot contain the | character.";
    for (const std::string& taken : takenNames) {
        if (toLower(taken) == toLower(name)) return "Name already taken. Enter a different name.";
    }
    return "";
}

std::string readName(int playerNumber, const std::vector<std::string>& takenNames) {
    while (true) {
        std::cout << "Enter player " << playerNumber << " name: ";
        std::string name = trim(readLine());
        std::string problem = checkName(name, takenNames);
        if (problem.empty()) return name;
        std::cout << problem << "\n";
    }
}

void pressEnter(const std::string& prompt) {
    std::cout << prompt << "\n";
    readLine();
}
