#include "OutputDeviceLogger.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FOutputDeviceLogger

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
    TScopedLock Lock(OutputDevicesCS);

    if (!OutputDevices.IsEmpty())
    {
        for (FOutputDevice* OutputDevice : OutputDevices)
        {
            OutputDevice->Log(Message);
        }
    }
}

void FOutputDeviceLogger::Log(ELogSeverity Severity, const FString& Message)
{
    TScopedLock Lock(OutputDevicesCS);

    if (!OutputDevices.IsEmpty())
    {
        for (FOutputDevice* OutputDevice : OutputDevices)
        {
            OutputDevice->Log(Severity, Message);
        }
    }
}

void FOutputDeviceLogger::Flush()
{
    TScopedLock Lock(OutputDevicesCS);

    if (!OutputDevices.IsEmpty())
    {
        for (FOutputDevice* OutputDevice : OutputDevices)
        {
            OutputDevice->Flush();
        }
    }
}
