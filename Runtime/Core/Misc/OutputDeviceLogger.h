#pragma once
#include "OutputDevice.h"

#include "Core/Core.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Threading/Platform/CriticalSection.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Logging macros

#define LOG_ERROR(...)                                                                              \
    do                                                                                              \
    {                                                                                               \
        FOutputDeviceLogger::Get()->Log(ELogSeverity::Error, FString::CreateFormated(__VA_ARGS__)); \
    } while (false)

#define LOG_WARNING(...)                                                                              \
    do                                                                                                \
    {                                                                                                 \
        FOutputDeviceLogger::Get()->Log(ELogSeverity::Warning, FString::CreateFormated(__VA_ARGS__)); \
    } while (false)

#define LOG_INFO(...)                                                                              \
    do                                                                                             \
    {                                                                                              \
        FOutputDeviceLogger::Get()->Log(ELogSeverity::Info, FString::CreateFormated(__VA_ARGS__)); \
    } while (false)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FOutputDeviceLogger

class CORE_API FOutputDeviceLogger
    : public FOutputDevice
{
    FOutputDeviceLogger();
    ~FOutputDeviceLogger();

public:

    /** @return: Returns the Logger singleton */
    static FOutputDeviceLogger* Get();

    /** @brief: Log a simple message to all output devices */
    virtual void Log(const FString& Message) override final;

    /** @brief: Log a message with severity to all output devices */
    virtual void Log(ELogSeverity Severity, const FString& Message) override final;

    /** @brief: Flush all output devices */
    virtual void Flush() override final;

    void AddOutputDevice(FOutputDevice* InOutputDevice)
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

    void RemoveOutputDevice(FOutputDevice* InOutputDevice)
    {
        if (this != InOutputDevice)
        {
            TScopedLock Lock(OutputDevicesCS);
            OutputDevices.Remove(InOutputDevice);
        }
    }

private:
    // TODO: Support multiple output devices;
    TArray<FOutputDevice*> OutputDevices;
    FCriticalSection       OutputDevicesCS;
};