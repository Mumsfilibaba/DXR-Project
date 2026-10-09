#pragma once
#include "Core/Windows/Windows.h"
#include "Core/PlatformInterface/IPlatformProcess.h"

class CORE_API FWindowsPlatformProcessHandle final : public IPlatformProcessHandle
{
public:
    FWindowsPlatformProcessHandle(HANDLE InProcess, HANDLE InOutputPipe);
    virtual ~FWindowsPlatformProcessHandle();

    virtual bool ReadOutput(String& OutText) override final;
    virtual bool IsRunning() override final;
    virtual bool Wait(int32 TimeoutMilliseconds = -1) override final;
    virtual bool GetExitCode(int32& OutExitCode) override final;
    virtual void Kill() override final;

private:
    HANDLE Process;
    HANDLE OutputPipe;
};

struct CORE_API FWindowsPlatformProcess final : public IPlatformProcess
{
    static TUniquePtr<IPlatformProcessHandle> LaunchProcess(const FProcessDesc& Desc);
};
