#include "Core/Mac/MacPlatformStackTrace.h"
#include "Core/Platform/PlatformLibrary.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include <dlfcn.h>
#include <execinfo.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <CoreFoundation/CoreFoundation.h>

#define LOAD_FUNCTION(Function, LibraryHandle) \
    do \
    { \
        Function = FPlatformLibrary::LoadSymbol<decltype(Function)>(#Function, LibraryHandle); \
        if (!Function) \
        { \
            LOG_ERROR("Failed to load '%s'", #Function); \
            return false;  \
        } \
    } while(false)

// Based on https://github.com/mountainstorm/CoreSymbolication
extern "C"
{
    struct CSTypeRef 
    {
        void* CppData;
        void* CppObj;
    };

    typedef CSTypeRef CSSymbolicatorRef;
    typedef CSTypeRef CSSourceInfoRef;
    typedef CSTypeRef CSSymbolRef;
    typedef CSTypeRef CSSymbolOwnerRef;

    struct CSRange
    {
        uint64 Location;
        uint64 Length;
    };
    
    typedef int (^CSSymbolIterator)(CSSymbolRef Symbol);
    typedef int (^CSSourceInfoIterator)(CSSourceInfoRef SourceInfo);

    #define kCSNow (0x80000000u)
    
    /* Utility functions */
    typedef Boolean(*PFN_CSIsNull)(CSTypeRef CS);
    typedef void(*PFN_CSRelease)(CSTypeRef CS);

    /* Symbolicator functions */
    typedef CSSymbolicatorRef(*PFN_CSSymbolicatorCreateWithPid)(pid_t pid);
    typedef CSSourceInfoRef(*PFN_CSSymbolicatorGetSourceInfoWithAddressAtTime)(CSSymbolicatorRef Symbolicator, vm_address_t Address, uint64_t Time);
    
    /* Symbol functions */
    typedef const char* (*PFN_CSSymbolGetName)(CSSymbolRef Symbol);
    typedef const char* (*PFN_CSSymbolOwnerGetName)(CSSymbolOwnerRef Owner);

    /* Source functions */
    typedef const char*(*PFN_CSSourceInfoGetPath)(CSSourceInfoRef Info);
    typedef int(*PFN_CSSourceInfoGetLineNumber)(CSSourceInfoRef Info);
    typedef CSSymbolRef(*PFN_CSSourceInfoGetSymbol)(CSSourceInfoRef Info);
    typedef CSSymbolOwnerRef(*PFN_CSSourceInfoGetSymbolOwner)(CSSourceInfoRef Info);
}

static FCriticalSection GSymbolsCS;
static int32            GSymbolsRefCount = 0;

// Handle to the dynamic library
static void* GCoreSymbolicationLibrary = nullptr;

// Symbolicator for the process
static CSSymbolicatorRef GSymbolicator = {};

static PFN_CSIsNull                                     CSIsNull                                     = nullptr;
static PFN_CSRelease                                    CSRelease                                    = nullptr;
static PFN_CSSymbolicatorCreateWithPid                  CSSymbolicatorCreateWithPid                  = nullptr;
static PFN_CSSymbolicatorGetSourceInfoWithAddressAtTime CSSymbolicatorGetSourceInfoWithAddressAtTime = nullptr;
static PFN_CSSymbolGetName                              CSSymbolGetName                              = nullptr;
static PFN_CSSymbolOwnerGetName                         CSSymbolOwnerGetName                         = nullptr;
static PFN_CSSourceInfoGetPath                          CSSourceInfoGetPath                          = nullptr;
static PFN_CSSourceInfoGetLineNumber                    CSSourceInfoGetLineNumber                    = nullptr;
static PFN_CSSourceInfoGetSymbol                        CSSourceInfoGetSymbol                        = nullptr;
static PFN_CSSourceInfoGetSymbolOwner                   CSSourceInfoGetSymbolOwner                   = nullptr;

/** CoreSymbolication returns null for anything it has no information about */
static void CopySymbolString(CHAR (&OutBuffer)[FStackTraceEntry::MaxNameLength], const CHAR* Value)
{
    if (Value)
    {
        CString::Strncpy(OutBuffer, Value, FStackTraceEntry::MaxNameLength);
    }
}

static bool LoadSymbolFunctions()
{
    LOAD_FUNCTION(CSIsNull, GCoreSymbolicationLibrary);
    LOAD_FUNCTION(CSRelease, GCoreSymbolicationLibrary);

    LOAD_FUNCTION(CSSymbolicatorCreateWithPid, GCoreSymbolicationLibrary);
    LOAD_FUNCTION(CSSymbolicatorGetSourceInfoWithAddressAtTime, GCoreSymbolicationLibrary);

    LOAD_FUNCTION(CSSymbolGetName, GCoreSymbolicationLibrary);
    LOAD_FUNCTION(CSSymbolOwnerGetName, GCoreSymbolicationLibrary);

    LOAD_FUNCTION(CSSourceInfoGetPath, GCoreSymbolicationLibrary);
    LOAD_FUNCTION(CSSourceInfoGetLineNumber, GCoreSymbolicationLibrary);
    LOAD_FUNCTION(CSSourceInfoGetSymbol, GCoreSymbolicationLibrary);
    LOAD_FUNCTION(CSSourceInfoGetSymbolOwner, GCoreSymbolicationLibrary);

    return true;
}

bool FMacPlatformStackTrace::InitializeSymbols()
{
    TScopedLock Lock(GSymbolsCS);

    if (GSymbolsRefCount == 0)
    {
        // dlopen directly rather than through FPlatformLibrary, which decorates 
        // the name into lib<Name>.dylib and so cannot express a framework path.
        GCoreSymbolicationLibrary = ::dlopen("/System/Library/PrivateFrameworks/CoreSymbolication.framework/Versions/Current/CoreSymbolication", RTLD_LAZY);
        if (!GCoreSymbolicationLibrary)
        {
            LOG_ERROR("Failed to load CoreSymbolication");
            return false;
        }

        if (!LoadSymbolFunctions())
        {
            ::dlclose(GCoreSymbolicationLibrary);
            GCoreSymbolicationLibrary = nullptr;
            return false;
        }

        GSymbolicator = CSSymbolicatorCreateWithPid(getpid());
    }

    ++GSymbolsRefCount;
    return true;
}

void FMacPlatformStackTrace::ReleaseSymbols()
{
    TScopedLock Lock(GSymbolsCS);

    if ((GSymbolsRefCount > 0) && (--GSymbolsRefCount == 0))
    {
        if (!CSIsNull(GSymbolicator))
        {
            CSRelease(GSymbolicator);
        }

        GSymbolicator = {};

        CSIsNull  = nullptr;
        CSRelease = nullptr;

        CSSymbolicatorCreateWithPid                  = nullptr;
        CSSymbolicatorGetSourceInfoWithAddressAtTime = nullptr;

        CSSymbolGetName      = nullptr;
        CSSymbolOwnerGetName = nullptr;

        CSSourceInfoGetPath        = nullptr;
        CSSourceInfoGetLineNumber  = nullptr;
        CSSourceInfoGetSymbol      = nullptr;
        CSSourceInfoGetSymbolOwner = nullptr;

        ::dlclose(GCoreSymbolicationLibrary);
        GCoreSymbolicationLibrary = nullptr;
    }
}

int32 FMacPlatformStackTrace::CaptureStackTrace(uint64* StackTrace, int32 MaxDepth)
{
    if (!StackTrace || !MaxDepth)
    {
        return 0;
    }

    int32 ActualDepth = backtrace(reinterpret_cast<void**>(StackTrace), MaxDepth);
    return ActualDepth;
}

static bool ReadProcessMemory(uint64 Address, void* OutBuffer, uint64 Size)
{
    mach_vm_size_t BytesRead = 0;

    const kern_return_t Result = ::mach_vm_read_overwrite(
        ::mach_task_self(),
        static_cast<mach_vm_address_t>(Address),
        static_cast<mach_vm_size_t>(Size),
        reinterpret_cast<mach_vm_address_t>(OutBuffer),
        &BytesRead);

    return (Result == KERN_SUCCESS) && (BytesRead == Size);
}

int32 FMacPlatformStackTrace::CaptureThreadStackTrace(const FThreadStackContext& ThreadContext, uint64* StackTrace, int32 MaxDepth)
{
    if (!StackTrace || (MaxDepth <= 0) || !ThreadContext.ThreadHandle)
    {
        return 0;
    }

    const thread_t Thread = static_cast<thread_t>(ThreadContext.ThreadHandle);

    uint64 ProgramCounter = 0;
    uint64 LinkRegister   = 0;
    uint64 FramePointer   = 0;

#if PLATFORM_ARCHITECTURE_ARM64
    arm_thread_state64_t   ThreadState = {};
    mach_msg_type_number_t StateCount  = ARM_THREAD_STATE64_COUNT;
    if (::thread_get_state(Thread, ARM_THREAD_STATE64, reinterpret_cast<thread_state_t>(&ThreadState), &StateCount) != KERN_SUCCESS)
    {
        return 0;
    }

    ProgramCounter = arm_thread_state64_get_pc(ThreadState);
    LinkRegister   = arm_thread_state64_get_lr(ThreadState);
    FramePointer   = arm_thread_state64_get_fp(ThreadState);
#else
    x86_thread_state64_t   ThreadState = {};
    mach_msg_type_number_t StateCount  = x86_THREAD_STATE64_COUNT;
    if (::thread_get_state(Thread, x86_THREAD_STATE64, reinterpret_cast<thread_state_t>(&ThreadState), &StateCount) != KERN_SUCCESS)
    {
        return 0;
    }

    ProgramCounter = ThreadState.__rip;
    FramePointer   = ThreadState.__rbp;
#endif

    int32 CurrentDepth = 0;
    StackTrace[CurrentDepth++] = ProgramCounter;

    if (LinkRegister && (LinkRegister != ProgramCounter) && (CurrentDepth < MaxDepth))
    {
        StackTrace[CurrentDepth++] = LinkRegister;
    }

    while ((CurrentDepth < MaxDepth) && FramePointer)
    {
        struct FStackFrame
        {
            uint64 CallerFramePointer;
            uint64 ReturnAddress;
        } Frame = {};

        if ((FramePointer & (sizeof(uint64) - 1)) != 0)
        {
            break;
        }

        if (!ReadProcessMemory(FramePointer, &Frame, sizeof(Frame)))
        {
            break;
        }

        if (!Frame.ReturnAddress)
        {
            break;
        }

        if (Frame.ReturnAddress != StackTrace[CurrentDepth - 1])
        {
            StackTrace[CurrentDepth++] = Frame.ReturnAddress;
        }

        if (Frame.CallerFramePointer <= FramePointer)
        {
            break;
        }

        FramePointer = Frame.CallerFramePointer;
    }

    return CurrentDepth;
}

static void SymbolicateWithCoreSymbolication(uint64 Address, FStackTraceEntry& OutStackTraceEntry)
{
    if (!CSIsNull(GSymbolicator))
    {
        CSSourceInfoRef Symbol = CSSymbolicatorGetSourceInfoWithAddressAtTime(GSymbolicator, (vm_address_t)Address, kCSNow);
        if(!CSIsNull(Symbol))
        {
            CopySymbolString(OutStackTraceEntry.Filename, CSSourceInfoGetPath(Symbol));

            CSSymbolRef FunctionSymbol = CSSourceInfoGetSymbol(Symbol);
            if (!CSIsNull(FunctionSymbol))
            {
                CopySymbolString(OutStackTraceEntry.FunctionName, CSSymbolGetName(FunctionSymbol));
            }

            OutStackTraceEntry.Line = CSSourceInfoGetLineNumber(Symbol);

            CSSymbolOwnerRef Owner = CSSourceInfoGetSymbolOwner(Symbol);
            if(!CSIsNull(Owner))
            {
                CopySymbolString(OutStackTraceEntry.ModuleName, CSSymbolOwnerGetName(Owner));
            }
        }
    }
}

void FMacPlatformStackTrace::GetStackTraceEntryFromAddress(uint64 Address, FStackTraceEntry& OutStackTraceEntry)
{
    if (InitializeSymbols())
    {
        SymbolicateWithCoreSymbolication(Address, OutStackTraceEntry);
        ReleaseSymbols();
    }

    // CoreSymbolication only resolves an address when the binary has debug information 
    // beside it, so fall back to the dynamic linker for at least a function and module name.
    if (OutStackTraceEntry.FunctionName[0] == 0)
    {
        Dl_info Info;
        if (::dladdr(reinterpret_cast<const void*>(Address), &Info))
        {
            CopySymbolString(OutStackTraceEntry.FunctionName, Info.dli_sname);

            if (OutStackTraceEntry.ModuleName[0] == 0)
            {
                CopySymbolString(OutStackTraceEntry.ModuleName, Info.dli_fname);
            }
        }
    }
}
