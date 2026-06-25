#include "TestHarness.h"
#include "TestConsoleOutputDevice.h"

#include <Core/Misc/OutputDeviceLogger.h>

FTestConsoleOutputDevice TestHarness::ConsoleDevice;
int32 TestHarness::NumPassed = 0;
int32 TestHarness::NumFailed = 0;

void TestHarness::Initialize()
{
    ConsoleDevice.OpenLogFile("TestResults.log");
    FOutputDeviceLogger::Get()->RegisterOutputDevice(&ConsoleDevice);
    
    NumPassed = 0;
    NumFailed = 0;
}

void TestHarness::Shutdown()
{
    FOutputDeviceLogger::Get()->UnregisterOutputDevice(&ConsoleDevice);
    ConsoleDevice.CloseLogFile();
}

void TestHarness::AddPass()
{
    ++NumPassed;
}

void TestHarness::AddFail()
{
    ++NumFailed;
}

void TestHarness::AddResult(bool bPassed)
{
    if (bPassed)
    {
        ++NumPassed;
    }
    else
    {
        ++NumFailed;
    }
}

int32 TestHarness::GetNumPassed()
{
    return NumPassed;
}

int32 TestHarness::GetNumFailed()
{
    return NumFailed;
}

int32 TestHarness::Report()
{
    const int32 Total = NumPassed + NumFailed;
    if (NumFailed == 0)
    {
        LOG_INFO("[SUMMARY] All %d test(s) passed", Total);
        return 0;
    }

    LOG_ERROR("[SUMMARY] %d of %d test(s) FAILED", NumFailed, Total);
    return 1;
}
