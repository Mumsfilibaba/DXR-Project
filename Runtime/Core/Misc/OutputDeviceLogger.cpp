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
    TArray<FOutputDevice*> CurrentOutputDevices;
    {
        TScopedLock Lock(OutputDevicesCS);
        CurrentOutputDevices = OutputDevices;
    }

    if (!CurrentOutputDevices.IsEmpty())
    {
        for (FOutputDevice* OutputDevice : CurrentOutputDevices)
        {
            OutputDevice->Log(Message);
        }
    }
}

void FOutputDeviceLogger::Log(ELogSeverity Severity, const FString& Message)
{
    TArray<FOutputDevice*> CurrentOutputDevices;
    {
        TScopedLock Lock(OutputDevicesCS);
        CurrentOutputDevices = OutputDevices;
    }

    if (!CurrentOutputDevices.IsEmpty())
    {
        for (FOutputDevice* OutputDevice : CurrentOutputDevices)
        {
            OutputDevice->Log(Severity, Message);
        }
    }
}

void FOutputDeviceLogger::Flush()
{
    TArray<FOutputDevice*> CurrentOutputDevices;
    {
        TScopedLock Lock(OutputDevicesCS);
        CurrentOutputDevices = OutputDevices;
    }

    if (!CurrentOutputDevices.IsEmpty())
    {
        for (FOutputDevice* OutputDevice : CurrentOutputDevices)
        {
            OutputDevice->Flush();
        }
    }
}
