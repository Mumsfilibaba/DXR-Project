#include "Core/Mac/MacPlatformProcess.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

FMacPlatformProcessHandle::FMacPlatformProcessHandle(pid_t InProcessId, int32 InOutputPipe)
    : IPlatformProcessHandle()
    , ProcessId(InProcessId)
    , OutputPipe(InOutputPipe)
    , ExitCode(0)
    , bHasExited(false)
{
}

FMacPlatformProcessHandle::~FMacPlatformProcessHandle()
{
    if (OutputPipe >= 0)
    {
        ::close(OutputPipe);
    }
}

void FMacPlatformProcessHandle::UpdateExitStatus(bool bBlock)
{
    if (bHasExited)
    {
        return;
    }

    int32 Status = 0;
    if (::waitpid(ProcessId, &Status, bBlock ? 0 : WNOHANG) == ProcessId)
    {
        bHasExited = true;
        ExitCode   = WIFEXITED(Status) ? WEXITSTATUS(Status) : -1;
    }
}

bool FMacPlatformProcessHandle::ReadOutput(String& OutText)
{
    const bool bHasExitedBeforeRead = !IsRunning();
    if (OutputPipe >= 0)
    {
        CHAR Buffer[4096];
        while (true)
        {
            const ssize_t NumRead = ::read(OutputPipe, Buffer, sizeof(Buffer));
            if (NumRead <= 0)
            {
                break;
            }

            OutText.Append(Buffer, static_cast<int32>(NumRead));
        }
    }

    return !bHasExitedBeforeRead;
}

bool FMacPlatformProcessHandle::IsRunning()
{
    UpdateExitStatus(false);
    return !bHasExited;
}

bool FMacPlatformProcessHandle::Wait(int32 TimeoutMilliseconds)
{
    if (TimeoutMilliseconds < 0)
    {
        UpdateExitStatus(true);
        return bHasExited;
    }

    for (int32 Elapsed = 0; Elapsed <= TimeoutMilliseconds; ++Elapsed)
    {
        if (!IsRunning())
        {
            return true;
        }

        ::usleep(1000);
    }

    return !IsRunning();
}

bool FMacPlatformProcessHandle::GetExitCode(int32& OutExitCode)
{
    if (IsRunning())
    {
        return false;
    }

    OutExitCode = ExitCode;
    return true;
}

void FMacPlatformProcessHandle::Kill()
{
    ::kill(ProcessId, SIGKILL);
}

TUniquePtr<IPlatformProcessHandle> FMacPlatformProcess::LaunchProcess(const FProcessDesc& Desc)
{
    TArray<CHAR*> Arguments;
    Arguments.Add(const_cast<CHAR*>(*Desc.Executable));
    for (const String& Argument : Desc.Arguments)
    {
        Arguments.Add(const_cast<CHAR*>(*Argument));
    }

    Arguments.Add(nullptr);

    posix_spawn_file_actions_t FileActions;
    ::posix_spawn_file_actions_init(&FileActions);

    int32 Pipe[2] = { -1, -1 };
    if (Desc.bCaptureOutput)
    {
        if (::pipe(Pipe) != 0)
        {
            LOG_ERROR("[FMacPlatformProcess]: Failed to create the output pipe for '%s'", *Desc.Executable);
            ::posix_spawn_file_actions_destroy(&FileActions);
            return nullptr;
        }

        ::posix_spawn_file_actions_adddup2(&FileActions, Pipe[1], STDOUT_FILENO);
        ::posix_spawn_file_actions_adddup2(&FileActions, Pipe[1], STDERR_FILENO);
        ::posix_spawn_file_actions_addclose(&FileActions, Pipe[0]);
        ::posix_spawn_file_actions_addclose(&FileActions, Pipe[1]);
    }

    if (!Desc.WorkingDirectory.IsEmpty())
    {
        ::posix_spawn_file_actions_addchdir_np(&FileActions, *Desc.WorkingDirectory);
    }

    pid_t ProcessId = 0;
    const int32 Result = ::posix_spawnp(&ProcessId, *Desc.Executable, &FileActions, nullptr, Arguments.Data(), environ);
    ::posix_spawn_file_actions_destroy(&FileActions);

    if (Pipe[1] >= 0)
    {
        ::close(Pipe[1]);
    }

    if (Result != 0)
    {
        LOG_ERROR("[FMacPlatformProcess]: Failed to start '%s' with error %d '%s'", *Desc.Executable, Result, ::strerror(Result));

        if (Pipe[0] >= 0)
        {
            ::close(Pipe[0]);
        }

        return nullptr;
    }

    if (Pipe[0] >= 0)
    {
        ::fcntl(Pipe[0], F_SETFL, ::fcntl(Pipe[0], F_GETFL) | O_NONBLOCK);
    }

    return TUniquePtr<IPlatformProcessHandle>(new FMacPlatformProcessHandle(ProcessId, Pipe[0]));
}
