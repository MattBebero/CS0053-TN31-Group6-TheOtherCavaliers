# Dungeons No Dragons

A turn-based battle game for the console, written in C++17 for the CS0053 (TN31) final group project, Group 6
"The Other Cavaliers". One to three players form a party, pick a class and fight through 10 stages that end in a boss.

## Features

- **Main menu:** Play, Continue, Tutorial, Run History, Exit.
- **1 to 3 players** on one keyboard. Duplicate class picks are allowed, duplicate names are not.
- **4 classes** (Swordsman, Ranger, Mage, Priest) with 4 moves each. The 4th move is learned at LVL 5.
- **Types:** Blunt beats Solid, Piercing beats Heavy, Mystic beats Spirit (2x damage).
- **MP, cooldowns and statuses:** Poison, Burn, Bleed, Shield, Taunt, Aim, Mana Focus, Inspire and Lifesteal.
- **Turn order by speed:** every player chooses first, then speed decides who acts first.
- **Enemy AI:** each enemy uses the affordable move with the highest MP cost and attacks a random player unless taunted.
- **Party scaling:** more players means more enemies, more enemy HP and weaker Cure and Heal All
  (60% with 2 players, 40% with 3).
- **Save and load:** the run saves to `savegame.txt` after every cleared stage. Continue reloads it. Finished runs
  are added to `history.txt` and shown in Run History.
- **Input validation:** every menu and name prompt rejects bad input and asks again.
- **Data files:** classes, enemies and moves live in plain text files in `data/`.

The full rules, numbers and screens are in the Game Design Reference document.

## Requirements

A C++17 compiler: `g++` (MinGW-w64 on Windows) 9 or newer.

## Build and run

Run these from this folder (the one with `src/` and `data/`), because the game reads `data/` from the current folder.

Windows (Command Prompt or PowerShell):

```
g++ -std=c++17 src\*.cpp -o battle_sim.exe
battle_sim.exe
```

macOS / Linux:

```
g++ -std=c++17 src/*.cpp -o battle_sim
./battle_sim
```

Optional: set the environment variable `DBND_SEED` to a number to get the same random enemies every run (used for tests).
Test keystroke files are in `tests/`.

## Files

| File | Job |
| --- | --- |
| `src/main.cpp`, `Game.cpp` | Entry point, menus, party setup, stage loop |
| `src/Battle.cpp` | Turns, damage, statuses, enemy AI |
| `src/Input.cpp` | Safe input and validation |
| `src/DataLoader.cpp` | Reads `data/*.txt` |
| `src/SaveManager.cpp` | Save, Continue and history |