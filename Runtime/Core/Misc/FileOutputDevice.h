#pragma once
#include "Core/Threading/ScopedLock.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Misc/IOutputDevice.h"
#include "Core/Generic/GenericPlatformFile.h"

class CORE_API FFileOutputDevice : public IOutputDevice
{
public:
    FFileOutputDevice(const String& FilePath);
    virtual ~FFileOutputDevice();

    virtual void Log(const String& Message) override final;
    virtual void Log(ELogSeverity Severity, const String& Message) override final;
    virtual void Flush() override final;

    bool IsValid() const
    {
        return FileHandle.IsValid();
    }

private:
    void QueueWrite(const String& Line);
    void FlushAsync();
    void FlushBlocking();

    TFileRef<IPlatformAsyncFile> FileHandle;
    TArray<String>               PendingLines;
    FCriticalSection             PendingLinesCS;
};
