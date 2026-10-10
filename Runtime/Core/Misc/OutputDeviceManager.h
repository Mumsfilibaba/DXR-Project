#pragma once
#include "Core/Threading/ScopedLock.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Misc/IOutputDevice.h"

#define LOG_FORMAT(Format) TEXT("" Format)

#define LOG_ERROR_CRITICAL(Format, ...) \
    do \
    { \
        FOutputDeviceManager::Get()->Log(ELogSeverity::Error, FString::Printf(LOG_FORMAT(Format) __VA_OPT__(,) __VA_ARGS__)); \
        DEBUG_BREAK(); \
    } while (false)

#define LOG_ERROR(Format, ...) \
    do \
    { \
        FOutputDeviceManager::Get()->Log(ELogSeverity::Error, FString::Printf(LOG_FORMAT(Format) __VA_OPT__(,) __VA_ARGS__)); \
    } while (false)

#define LOG_WARNING(Format, ...) \
    do \
    { \
        FOutputDeviceManager::Get()->Log(ELogSeverity::Warning, FString::Printf(LOG_FORMAT(Format) __VA_OPT__(,) __VA_ARGS__)); \
    } while (false)

#define LOG_INFO(Format, ...) \
    do \
    { \
        FOutputDeviceManager::Get()->Log(ELogSeverity::Info, FString::Printf(LOG_FORMAT(Format) __VA_OPT__(,) __VA_ARGS__)); \
    } while (false)

class CORE_API FOutputDeviceManager : public IOutputDevice
{
public:

    /** @return Returns the OutputDeviceManager singleton */
    static FOutputDeviceManager* Get();

public:
    /** @brief Log a simple message to all output devices */
    virtual void Log(const FString& Message) override final;

    /** @brief Log a message with severity to all output devices */
    virtual void Log(ELogSeverity Severity, const FString& Message) override final;

    /** @brief Flush all output devices */
    virtual void Flush() override final;

    /** @brief Replays what was logged before any device existed, then drops the backlog */
    void FlushPendingLines();

    void RegisterOutputDevice(IOutputDevice* InOutputDevice)
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

    void UnregisterOutputDevice(IOutputDevice* InOutputDevice)
    {
        if (this != InOutputDevice)
        {
            TScopedLock Lock(OutputDevicesCS);
            OutputDevices.Remove(InOutputDevice);
        }
    }

private:
    struct FPendingLine
    {
        FString      Message;
        ELogSeverity Severity;
        bool         bHasSeverity;
    };

    FOutputDeviceManager();
    ~FOutputDeviceManager();

    void QueuePendingLine(const FString& Message, ELogSeverity Severity, bool bHasSeverity);

    TArray<IOutputDevice*> OutputDevices;
    TArray<FPendingLine>   PendingLines;
    FCriticalSection       OutputDevicesCS;
};
