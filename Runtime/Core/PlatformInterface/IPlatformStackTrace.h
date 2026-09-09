#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

#define MAX_STACK_DEPTH (128)

/** @brief One resolved frame of a call-stack. */
struct FStackTraceEntry
{
    /** @brief The room every name gets, fixed because the crash and allocation-tracking paths cannot allocate. */
    inline static constexpr uint32 MaxNameLength = 512;

    /** @brief The resolved symbol, empty when the address belongs to none the platform could name. */
    CHAR   FunctionName[MaxNameLength];

    /** @brief The source file, empty without the debug information that carries line numbers. */
    CHAR   Filename[MaxNameLength];

    /** @brief The binary the address lies in, which resolves even where no debug information is present. */
    CHAR   ModuleName[MaxNameLength];

    /** @brief The line in Filename, zero when there is no line information for the address. */
    uint32 Line;
};

struct FThreadStackContext
{
    /** @brief The thread to walk, a HANDLE on Windows and a thread_t port on macOS. */
    uint64 ThreadHandle = 0;

    /** @brief Register state to start the walk from, a CONTEXT* on Windows and null on macOS, which reads it off the thread. */
    void* RegisterState = nullptr;
};

struct CORE_API IPlatformStackTrace
{
    /** @return Returns true if the debug symbols needed to resolve addresses were loaded */
    static FORCEINLINE bool InitializeSymbols()
    {
        return true;
    }

    /** @brief Release the debug symbols loaded by InitializeSymbols */
    static FORCEINLINE void ReleaseSymbols()
    {
    }

    /** @return Returns the number of addresses written, dropping the IgnoreCount innermost frames */
    static int32 CaptureStackTrace(uint64* StackTrace, int32 MaxDepth, int32 IgnoreCount);

    /** @return Returns the number of return addresses captured for the calling thread, at most MaxDepth */
    static FORCEINLINE int32 CaptureStackTrace(uint64* StackTrace, int32 MaxDepth)
    {
        return 0;
    }

    /**
     * @brief Captures the return addresses of a thread other than the caller, which has to be suspended for
     * the walk to see a coherent frame chain.
     *
     * @param ThreadContext The thread to walk and the register state to start from.
     * @param StackTrace    Receives the return addresses, innermost frame first.
     * @param MaxDepth      How many addresses StackTrace has room for.
     * @return The number of addresses written, which is zero on a platform with no implementation.
     */
    static FORCEINLINE int32 CaptureThreadStackTrace(const FThreadStackContext& ThreadContext, uint64* StackTrace, int32 MaxDepth)
    {
        return 0;
    }

    /** @brief Resolve a return address into the function, file, line and module it belongs to */
    static FORCEINLINE void GetStackTraceEntryFromAddress(uint64 Address, FStackTraceEntry& OutStackTraceEntry)
    {
    }

    /** @return Returns the resolved call-stack of the calling thread, loading and releasing symbols around the capture */
    static TArray<FStackTraceEntry> GetStack(int32 MaxDepth = 128, int32 IgnoreCount = 0);

    /**
     * @brief Resolves the call-stack of a thread other than the caller, which has to be suspended.
     *
     * @param ThreadContext The thread to walk and the register state to start from.
     * @param MaxDepth      How many frames to resolve at most.
     * @return The resolved call-stack, empty when the thread could not be walked.
     */
    static TArray<FStackTraceEntry> GetThreadStack(const FThreadStackContext& ThreadContext, int32 MaxDepth = MAX_STACK_DEPTH);

    /** @return Returns the full path of the running executable */
    static FORCEINLINE String GetExecutableFilename()
    {
        return String();
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
