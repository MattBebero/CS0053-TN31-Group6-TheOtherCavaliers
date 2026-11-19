// Random.h - the game's random number helpers.
// Owner: Matt (Member 1).
#pragma once

#include <random>

// The shared random engine. Set the environment variable DBND_SEED to a number
// to get the same enemies and targets every run (used for tests).
std::mt19937& rng();

// A random whole number from 0 to 99, used for percent chances.
int randomPercent();
