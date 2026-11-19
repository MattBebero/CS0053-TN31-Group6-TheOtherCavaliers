// main.cpp - program entry point. The work happens in the other files: main
// only loads the data and opens the main menu.
// Owner: Matt (Member 1).
#include <iostream>

#include "DataLoader.h"
#include "Game.h"
#include "Types.h"

int main() {
    GameData data;
    if (!loadGameData(data, "data")) {
        std::cout << "The game data could not be loaded. Check the data folder and try again.\n";
        return 1;
    }
    mainMenu(data);
    return 0;
}
