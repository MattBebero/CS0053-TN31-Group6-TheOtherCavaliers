// Input.h - every keyboard read goes through these functions so invalid
// input is handled the same way everywhere.
// Owner: Al Noor (Member 3).
#pragma once

#include <string>
#include <vector>

std::string trim(const std::string& text);
std::string toLower(std::string text);

// Reads a whole number from min to max. Re-asks until the input is valid.
int readInt(int min, int max);

// Reads Y or N (also yes/no, any capitals). Returns true for yes.
bool readYesNo();

// Asks for a player's name until it is valid and not already taken.
std::string readName(int playerNumber, const std::vector<std::string>& takenNames);

// Returns an error message, or "" if the name is allowed.
std::string checkName(const std::string& name, const std::vector<std::string>& takenNames);

// Prints "Press Enter to return." and waits.
void pressEnter(const std::string& prompt = "Press Enter to return.");
