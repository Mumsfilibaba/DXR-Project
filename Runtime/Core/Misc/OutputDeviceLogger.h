#pragma once
#include "OutputDevice.h"
#include "Core/Core.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Platform/CriticalSection.h"

#define LOG_ERROR(...)                                                                               \
    do                                                                                               \
    {                                                                                                \
        FOutputDeviceLogger::Get()->Log(ELogSeverity::Error, FString::CreateFormatted(__VA_ARGS__)); \
    } while (false)

#define LOG_WARNING(...)                                                                               \
    do                                                                                                 \
    {                                                                                                  \
        FOutputDeviceLogger::Get()->Log(ELogSeverity::Warning, FString::CreateFormatted(__VA_ARGS__)); \
    } while (false)

#define LOG_INFO(...)                                                                               \
    do                                                                                              \
    {                                                                                               \
        FOutputDeviceLogger::Get()->Log(ELogSeverity::Info, FString::CreateFormatted(__VA_ARGS__)); \
    } while (false)


class CORE_API FOutputDeviceLogger : public IOutputDevice
{
    FOutputDeviceLogger();
    ~FOutputDeviceLogger();

public:

    /** @return - Returns the Logger singleton */
    static FOutputDeviceLogger* Get();

    /** @brief - Log a simple message to all output devices */
    virtual void Log(const FString& Message) override final;

    /** @brief - Log a message with severity to all output devices */
    virtual void Log(ELogSeverity Severity, const FString& Message) override final;

    /** @brief - Flush all output devices */
    virtual void Flush() override final;

    void AddOutputDevice(IOutputDevice* InOutputDevice)
    {
        if (this != InOutputDevice)
        {
            TScopedLock Lock(OutputDevicesCS);
            if (!OutputDevices.Contains(InOutputDevice))
            {
                OutputDevices.Emplace(InOutputDevice);
            }
        }
    }

    void RemoveOutputDevice(IOutputDevice* InOutputDevice)
    {
        if (this != InOutputDevice)
        {
            TScopedLock Lock(OutputDevicesCS);
            OutputDevices.Remove(InOutputDevice);
        }
    }

private:
    TArray<IOutputDevice*> OutputDevices;
    FCriticalSection       OutputDevicesCS;
};
