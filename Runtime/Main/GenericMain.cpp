#include "Main/EngineLoop.h"

#include "Core/Core.h"
#include "Core/Misc/OutputDevice.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/CommandLine.h"
#include "Core/Memory/Malloc.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

#include "Application/ApplicationInterface.h"

#include "Engine/Engine.h"

FEngineLoop GEngineLoop;

FORCEINLINE bool EngineLoadCoreModules()
{
    return GEngineLoop.LoadCoreModules();
}

FORCEINLINE bool EnginePreInit()
{
    return GEngineLoop.PreInit();
}

FORCEINLINE bool EngineInit()
{
    return GEngineLoop.Init();
}

FORCEINLINE void EngineTick()
{
    GEngineLoop.Tick();
}

FORCEINLINE bool EngineRelease()
{
    return GEngineLoop.Release();
}


struct FDebuggerOutputDevice
    : public IOutputDevice
{
    virtual void Log(const FString& Message) 
    {
        FPlatformMisc::OutputDebugString(Message.GetCString());
        FPlatformMisc::OutputDebugString("\n");
    }

    virtual void Log(ELogSeverity Severity, const FString& Message)
    {
        Log(Message);
    }

    virtual void Flush() override final { }
};

// NOTE: OutputDevice for the debugger
IOutputDevice* GDebugOutput = nullptr;


// Application EntryPoint
int32 GenericMain(const CHAR** Args, int32 NumArgs)
{
    struct FGenericMainGuard
    {
        ~FGenericMainGuard()
        {
            if (!EngineRelease())
            {
                FPlatformApplicationMisc::MessageBox("ERROR", "FEngineLoop::Release Failed");
            }

            // Only report the leaking to the debugger output device
            if (FPlatformMisc::IsDebuggerPresent())
            {
                FMalloc::Get().DumpAllocations(GDebugOutput);
            }
        }
    };

    {
        // Make sure that the engine is released if the main function exits early
        FGenericMainGuard GenericMainGuard;

        // Only report the leaking to the debugger output device
        if (FPlatformMisc::IsDebuggerPresent())
        {
            GDebugOutput = new FDebuggerOutputDevice();
            FOutputDeviceLogger::Get()->AddOutputDevice(GDebugOutput);
        }

        if (!FCommandLine::Initialize(Args, NumArgs))
        {
            LOG_WARNING("Invalid CommandLine");
        }

        if (!EnginePreInit())
        {
            FPlatformApplicationMisc::MessageBox("ERROR", "FEngineLoop::PreInit Failed");
            return -1;
        }

        FMalloc::Get().DumpAllocations(GDebugOutput);

        if (!EngineInit())
        {
            FPlatformApplicationMisc::MessageBox("ERROR", "FEngineLoop::Init Failed");
            return -1;
        }

        // Run loop
        while (FApplicationInterface::Get().IsRunning())
        {
            EngineTick();
        }
    }

    return 0;
}
