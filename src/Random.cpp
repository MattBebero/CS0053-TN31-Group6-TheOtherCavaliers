// Random.cpp - the game's random number helpers.
// Owner: Matt (Member 1).
#include "Random.h"

#include <cstdlib>

std::mt19937& rng() {
    static std::mt19937 engine([] {
        const char* seed = std::getenv("DBND_SEED");  // fixed seed makes tests repeatable
        if (seed != nullptr) return static_cast<std::mt19937::result_type>(std::atoi(seed));
        return static_cast<std::mt19937::result_type>(std::random_device{}());
    }());
    return engine;
}

int randomPercent() { return static_cast<int>(rng()() % 100); }
