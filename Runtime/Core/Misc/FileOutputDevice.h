#pragma once
#include "Core/Threading/ScopedLock.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Misc/IOutputDevice.h"
#include "Core/PlatformInterface/IPlatformFileSystem.h"

class CORE_API FFileOutputDevice : public IOutputDevice
{
public:
    FFileOutputDevice(const FString& FilePath);
    virtual ~FFileOutputDevice();

    virtual void Log(const FString& Message) override final;
    virtual void Log(ELogSeverity Severity, const FString& Message) override final;
    virtual void Flush() override final;

    bool IsValid() const
    {
        return FileHandle.IsValid();
    }

private:
    void QueueWrite(const FString& Line);
    void FlushAsync();
    void FlushBlocking();
    void SubmitPendingLines();
    bool WriteLines(const TArray<FString>& Lines);
    void DropOverflowingLines();

    TFileRef<IPlatformAsyncFile> FileHandle;
    FCriticalSection             FileCS;
    TArray<FString>              PendingLines;
    FCriticalSection             PendingLinesCS;
    bool                         bIsInsideHandle;
};
