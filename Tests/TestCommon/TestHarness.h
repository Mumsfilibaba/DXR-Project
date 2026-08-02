#pragma once
#include <Core/CoreTypes.h>

#include "TestConsoleOutputDevice.h"

struct TestHarness
{
public:
    static void Initialize(const CHAR* LogFileName = "TestResults.log");
    static void Shutdown();

    static void AddPass();
    static void AddFail();
    static void AddResult(bool bPassed);

    static int32 GetNumPassed();
    static int32 GetNumFailed();

    static int32 Report();

private:
    static FTestConsoleOutputDevice ConsoleDevice;
    static int32 NumPassed;
    static int32 NumFailed;
};
