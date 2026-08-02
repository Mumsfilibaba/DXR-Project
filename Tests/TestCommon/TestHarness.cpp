#include "TestHarness.h"
#include "TestConsoleOutputDevice.h"

#include <Core/CoreGlobals.h>
#include <Core/Misc/OutputDeviceLogger.h>

FTestConsoleOutputDevice TestHarness::ConsoleDevice;
int32 TestHarness::NumPassed = 0;
int32 TestHarness::NumFailed = 0;

void TestHarness::Initialize(const CHAR* LogFileName)
{
    // No one is watching a test run, so a failed assert has to fail the run rather than block on a dialog
    GIsUnattended = true;

    ConsoleDevice.OpenLogFile(LogFileName);
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
