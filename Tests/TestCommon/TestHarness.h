#pragma once
#include <Core/CoreTypes.h>

#include "TestConsoleOutputDevice.h"

struct TestHarness
{
public:

    /** @brief Register the stdout output device and reset the counters. */
    static void Initialize();

    /** @brief Unregister the stdout output device. */
    static void Shutdown();

    static void AddPass();
    static void AddFail();
    static void AddResult(bool bPassed);

    static int32 GetNumPassed();
    static int32 GetNumFailed();

    /**
     * @brief Log a summary of the results.
     * @return 0 if every test passed, 1 otherwise (suitable as a process exit code).
     */
    static int32 Report();

private:
    static FTestConsoleOutputDevice ConsoleDevice;
    static int32 NumPassed;
    static int32 NumFailed;
};
