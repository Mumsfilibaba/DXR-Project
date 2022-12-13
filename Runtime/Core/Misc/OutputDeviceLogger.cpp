#include "OutputDeviceLogger.h"

FOutputDeviceLogger::FOutputDeviceLogger()
    : OutputDevices()
    , OutputDevicesCS()
{ }

FOutputDeviceLogger::~FOutputDeviceLogger()
{
    TScopedLock Lock(OutputDevicesCS);
    OutputDevices.Clear();
}

FOutputDeviceLogger* FOutputDeviceLogger::Get()
{
    static FOutputDeviceLogger Instance;
    return &Instance;
}

void FOutputDeviceLogger::Log(const FString& Message)
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

void FOutputDeviceLogger::Log(ELogSeverity Severity, const FString& Message)
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
