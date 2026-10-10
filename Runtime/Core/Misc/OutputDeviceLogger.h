#pragma once
#include "Core/Threading/ScopedLock.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Misc/IOutputDevice.h"

#define LOG_FORMAT(Format) TEXT("" Format)

#define LOG_ERROR_CRITICAL(Format, ...) \
    do \
    { \
        FOutputDeviceLogger::Get()->Log(ELogSeverity::Error, String::Printf(LOG_FORMAT(Format) __VA_OPT__(,) __VA_ARGS__)); \
        DEBUG_BREAK(); \
    } while (false)

#define LOG_ERROR(Format, ...) \
    do \
    { \
        FOutputDeviceLogger::Get()->Log(ELogSeverity::Error, String::Printf(LOG_FORMAT(Format) __VA_OPT__(,) __VA_ARGS__)); \
    } while (false)

#define LOG_WARNING(Format, ...) \
    do \
    { \
        FOutputDeviceLogger::Get()->Log(ELogSeverity::Warning, String::Printf(LOG_FORMAT(Format) __VA_OPT__(,) __VA_ARGS__)); \
    } while (false)

#define LOG_INFO(Format, ...) \
    do \
    { \
        FOutputDeviceLogger::Get()->Log(ELogSeverity::Info, String::Printf(LOG_FORMAT(Format) __VA_OPT__(,) __VA_ARGS__)); \
    } while (false)

class CORE_API FOutputDeviceLogger : public IOutputDevice
{
public:

    /** @return Returns the Logger singleton */
    static FOutputDeviceLogger* Get();

public:
    /** @brief Log a simple message to all output devices */
    virtual void Log(const String& Message) override final;

    /** @brief Log a message with severity to all output devices */
    virtual void Log(ELogSeverity Severity, const String& Message) override final;

    /** @brief Flush all output devices */
    virtual void Flush() override final;

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
    FOutputDeviceLogger();
    ~FOutputDeviceLogger();

    TArray<IOutputDevice*> OutputDevices;
    FCriticalSection       OutputDevicesCS;
};
