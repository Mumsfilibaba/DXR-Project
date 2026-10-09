#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Containers/UniquePtr.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FProcessDesc
{
    /** @brief Path of the executable to start */
    String Executable;

    /** @brief One entry per argument. Quoting for the platform is added when the process is started. */
    TArray<String> Arguments;

    /** @brief The directory the process starts in, or empty to inherit the current one */
    String WorkingDirectory;

    /** @brief Collect standard output and standard error into one stream, read with IPlatformProcessHandle::ReadOutput */
    bool bCaptureOutput = false;

    /** @brief Start a console program without opening a console window for it */
    bool bHidden = true;
};

struct IPlatformProcessHandle
{
    virtual ~IPlatformProcessHandle() = default;

    /**
     * @brief Append whatever captured output is available to OutText without waiting for more
     * @return Returns false once the process has exited and all of its output has been read
     */
    virtual bool ReadOutput(String& OutText) = 0;

    /** @return Returns true until the process has exited */
    virtual bool IsRunning() = 0;

    /**
     * @brief Wait for the process to exit
     * @param TimeoutMilliseconds How long to wait, or a negative value to wait for as long as it takes
     * @return Returns true if the process has exited
     */
    virtual bool Wait(int32 TimeoutMilliseconds = -1) = 0;

    /** @return Returns false while the process is still running, otherwise true with the exit code in OutExitCode */
    virtual bool GetExitCode(int32& OutExitCode) = 0;

    /** @brief End the process immediately */
    virtual void Kill() = 0;
};

struct IPlatformProcess
{
    /** @return Returns a handle to the started process, or nullptr if it could not be started */
    static FORCEINLINE TUniquePtr<IPlatformProcessHandle> LaunchProcess(const FProcessDesc& Desc)
    {
        return nullptr;
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
