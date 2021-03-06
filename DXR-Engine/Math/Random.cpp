#include "PreCompiled.h"
#include "Random.h"

#include <random>

std::random_device gRandomDevice;
std::mt19937       gGenerator(gRandomDevice());

std::uniform_int_distribution<> IntDistribution(INT32_MIN, INT32_MAX);

UInt32 Random::Integer()
{
    return IntDistribution(gGenerator);
}
