#include "Core/Misc/OutputDeviceManager.h"

static constexpr int32 MaxPendingLines = 1024;

FOutputDeviceManager::FOutputDeviceManager()
    : OutputDevices()
    , PendingLines()
    , OutputDevicesCS()
{
}

FOutputDeviceManager::~FOutputDeviceManager()
{
    TScopedLock Lock(OutputDevicesCS);
    OutputDevices.Clear();
    PendingLines.Clear();
}

FOutputDeviceManager* FOutputDeviceManager::Get()
{
    static FOutputDeviceManager StaticOutputDeviceManager;
    return &StaticOutputDeviceManager;
}

void FOutputDeviceManager::Log(const FString& Message)
{
    TArray<IOutputDevice*> CurrentOutputDevices;
    {
        TScopedLock Lock(OutputDevicesCS);
        if (OutputDevices.IsEmpty())
        {
            QueuePendingLine(Message, ELogSeverity::Info, false);
            return;
        }

        CurrentOutputDevices = OutputDevices;
    }

    for (IOutputDevice* OutputDevice : CurrentOutputDevices)
    {
        OutputDevice->Log(Message);
    }
}

void FOutputDeviceManager::Log(ELogSeverity Severity, const FString& Message)
{
    TArray<IOutputDevice*> CurrentOutputDevices;
    {
        TScopedLock Lock(OutputDevicesCS);
        if (OutputDevices.IsEmpty())
        {
            QueuePendingLine(Message, Severity, true);
            return;
        }

        CurrentOutputDevices = OutputDevices;
    }

    for (IOutputDevice* OutputDevice : CurrentOutputDevices)
    {
        OutputDevice->Log(Severity, Message);
    }
}

void FOutputDeviceManager::QueuePendingLine(const FString& Message, ELogSeverity Severity, bool bHasSeverity)
{
    PendingLines.Emplace(FPendingLine{ Message, Severity, bHasSeverity });

    if (PendingLines.Size() > MaxPendingLines)
    {
        PendingLines.RemoveAt(0, PendingLines.Size() - MaxPendingLines);
    }
}

void FOutputDeviceManager::FlushPendingLines()
{
    TArray<IOutputDevice*> CurrentOutputDevices;
    TArray<FPendingLine>   LinesToReplay;
    {
        TScopedLock Lock(OutputDevicesCS);
        if (OutputDevices.IsEmpty())
        {
            return;
        }

        CurrentOutputDevices = OutputDevices;
        LinesToReplay        = Move(PendingLines);
        PendingLines.Clear();
    }

    for (const FPendingLine& Line : LinesToReplay)
    {
        for (IOutputDevice* OutputDevice : CurrentOutputDevices)
        {
            if (Line.bHasSeverity)
            {
                OutputDevice->Log(Line.Severity, Line.Message);
            }
            else
            {
                OutputDevice->Log(Line.Message);
            }
        }
    }
}

void FOutputDeviceManager::Flush()
{
    TArray<IOutputDevice*> CurrentOutputDevices;
    {
        TScopedLock Lock(OutputDevicesCS);
        CurrentOutputDevices = OutputDevices;
    }

    if (!CurrentOutputDevices.IsEmpty())
    {
        for (IOutputDevice* OutputDevice : CurrentOutputDevices)
        {
            OutputDevice->Flush();
        }
    }
}
