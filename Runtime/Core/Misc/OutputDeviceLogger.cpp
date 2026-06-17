#include "Core/Misc/OutputDeviceLogger.h"

FOutputDeviceLogger::FOutputDeviceLogger()
    : OutputDevices()
    , OutputDevicesCS()
{
}

FOutputDeviceLogger::~FOutputDeviceLogger()
{
    TScopedLock Lock(OutputDevicesCS);
    OutputDevices.Clear();
}

FOutputDeviceLogger* FOutputDeviceLogger::Get()
{
    static FOutputDeviceLogger StaticOutputDeviceLogger;
    return &StaticOutputDeviceLogger;
}

void FOutputDeviceLogger::Log(const String& Message)
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
            OutputDevice->Log(Message);
        }
    }
}

void FOutputDeviceLogger::Log(ELogSeverity Severity, const String& Message)
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
            OutputDevice->Log(Severity, Message);
        }
    }
}

void FOutputDeviceLogger::Flush()
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
