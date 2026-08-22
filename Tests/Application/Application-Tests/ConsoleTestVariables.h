#pragma once
#include <Core/CoreTypes.h>

/** @brief The number of variables RegisterConsoleCrowdVariables registers under the crowd prefix. */
constexpr int32 GNumConsoleCrowdVariables = 40;

/** @brief The prefix every crowd variable shares, so one typed character matches all of them. */
constexpr const CHAR* GConsoleCrowdPrefix = "Zulu";

void RegisterConsoleTestVariables();
void RegisterConsoleCrowdVariables();
