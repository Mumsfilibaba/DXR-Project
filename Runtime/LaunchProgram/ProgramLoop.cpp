#include "LaunchProgram/ProgramLoop.h"
#include "LaunchProgram/ProgramEntry.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Modules/ModuleManager.h"
#include "CoreApplication/Platform/PlatformConsoleWindow.h"

IMPLEMENT_ENGINE_MODULE(IModule, LaunchProgram);

const CHAR*        GProgramTitle = "Program";
TFunction<int32()> GProgramBody;

bool FProgramLoop::bExitRequested = false;

void FProgramLoop::RequestExit(const CHAR* ExitReason)
{
    if (bExitRequested)
    {
        return;
    }

    bExitRequested = true;
    if (ExitReason && (*ExitReason != '\0'))
    {
        LOG_INFO("[Program] Exit requested: %s", ExitReason);
    }
}

bool FProgramLoop::IsExitRequested()
{
    return bExitRequested;
}

int32 FProgramLoop::Run(const CHAR* ConsoleTitle, const TFunction<int32()>& Body)
{
    bExitRequested = false;

    TUniquePtr<IPlatformConsoleWindow> Console(FPlatformConsoleWindow::Create());
    if (!Console)
    {
        LOG_ERROR("[Program] Failed to create the console window");
        return 1;
    }

    const CHAR* Title = (ConsoleTitle && (*ConsoleTitle != '\0')) ? ConsoleTitle : "Program";
    Console->SetTitle(Title);
    Console->Show(true);
    Console->SetOnClosed([]()
    {
        FProgramLoop::RequestExit(nullptr);
    });
    
    FOutputDeviceLogger::Get()->RegisterOutputDevice(Console.Get());

    int32 Result = 0;
    if (Body)
    {
        TFunction<int32()> BodyCopy = Body;
        Result = BodyCopy();
    }

    FOutputDeviceLogger::Get()->UnregisterOutputDevice(Console.Get());
    Console->Show(false);
    return Result;
}
