#pragma once
#include "Core/PlatformInterface/IPlatformProcess.h"
#include <sys/types.h>

class CORE_API FMacPlatformProcessHandle final : public IPlatformProcessHandle
{
public:
    FMacPlatformProcessHandle(pid_t InProcessId, int32 InOutputPipe);
    virtual ~FMacPlatformProcessHandle();

    virtual bool ReadOutput(String& OutText) override final;
    virtual bool IsRunning() override final;
    virtual bool Wait(int32 TimeoutMilliseconds = -1) override final;
    virtual bool GetExitCode(int32& OutExitCode) override final;
    virtual void Kill() override final;

private:
    void UpdateExitStatus(bool bBlock);

    pid_t ProcessId;
    int32 OutputPipe;
    int32 ExitCode;
    bool  bHasExited;
};

struct CORE_API FMacPlatformProcess final : public IPlatformProcess
{
    static TUniquePtr<IPlatformProcessHandle> LaunchProcess(const FProcessDesc& Desc);
};
