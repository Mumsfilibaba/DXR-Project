#pragma once
#include "Core/Core.h"
#include "Core/Containers/Function.h"

struct FProgramLoop
{
    /**
     * @brief Opens the console, runs the body once, and closes down again.
     *
     * Nothing is pumped or ticked around the body, so a program that opens windows runs its own frame loop
     * inside the body and ends it when the exit flag is raised.
     *
     * @param ConsoleTitle Title for the console window.
     * @param Body         The program itself, whose return value becomes the exit code.
     * @return The value returned by the body, or 1 when the console could not be opened.
     */
    static int32 Run(const CHAR* ConsoleTitle, const TFunction<int32()>& Body);

    /**
     * @brief Raises the exit flag, from the console close handler or from the program itself.
     *
     * @param ExitReason Logged when given, and left out when the console is already going away.
     */
    static void RequestExit(const CHAR* ExitReason);

    /** @brief Checks whether anything has asked the program to stop. */
    static bool IsExitRequested();

private:
    static bool bExitRequested;
};
