#include "MacPlatformStackTrace.h"
#include "Core/Platform/PlatformLibrary.h"
#include "Core/Misc/OutputDeviceLogger.h"

#include <execinfo.h>
#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach.h>

#define LOAD_FUNCTION(Function, LibraryHandle)                                                        \
    do                                                                                                \
    {                                                                                                 \
        Function = FPlatformLibrary::LoadSymbol<decltype(Function)>(#Function, LibraryHandle); \
        if (!Function)                                                                                \
        {                                                                                             \
            LOG_ERROR("Failed to load '%s'", #Function);                                              \
            return false;                                                                             \
        }                                                                                             \
    } while(false)

// Based on https://github.com/mountainstorm/CoreSymbolication
extern "C"
{
    /* Types */

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

    /* Defines */

    #define kCSNow (0x80000000u)
    
    /* Utility functions */

    typedef Boolean (*PFN_CSIsNull)(CSTypeRef CS);
    typedef void    (*PFN_CSRelease)(CSTypeRef CS);

    /* Symbolicator functions */

    typedef CSSymbolicatorRef (*PFN_CSSymbolicatorCreateWithPid)(pid_t pid);
    typedef CSSourceInfoRef   (*PFN_CSSymbolicatorGetSourceInfoWithAddressAtTime)(CSSymbolicatorRef Symbolicator, vm_address_t Address, uint64_t Time);
    
    /* Symbol functions */

    typedef const char* (*PFN_CSSymbolGetName)(CSSymbolRef Symbol);
    typedef const char* (*PFN_CSSymbolOwnerGetName)(CSSymbolOwnerRef Owner);

    /* Source functions */

    typedef const char*      (*PFN_CSSourceInfoGetPath)(CSSourceInfoRef Info);
    typedef int              (*PFN_CSSourceInfoGetLineNumber)(CSSourceInfoRef Info);
    typedef CSSymbolRef      (*PFN_CSSourceInfoGetSymbol)(CSSourceInfoRef Info);
    typedef CSSymbolOwnerRef (*PFN_CSSourceInfoGetSymbolOwner)(CSSourceInfoRef Info);
}

// Keeps track if the symbolification helpers has been initialized
static bool GIsInitialized = false;

// Handle to the dynamic library
static void* GCoreSymbolicationLibrary = nullptr;

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

bool FMacPlatformStackTrace::InitializeSymbols()
{
    if (!GIsInitialized)
    {
        GCoreSymbolicationLibrary = FPlatformLibrary::LoadDynamicLib("/System/Library/PrivateFrameworks/CoreSymbolication.framework/Versions/Current/CoreSymbolication");
        if(GCoreSymbolicationLibrary)
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
        }
    }

    return true;
}

void FMacPlatformStackTrace::ReleaseSymbols()
{
    if (GIsInitialized)
    {
        CSIsNull                                     = nullptr;
        CSRelease                                    = nullptr;

        CSSymbolicatorCreateWithPid                  = nullptr;
        CSSymbolicatorGetSourceInfoWithAddressAtTime = nullptr;

        CSSymbolGetName                              = nullptr;
        CSSymbolOwnerGetName                         = nullptr;

        CSSourceInfoGetPath                          = nullptr;
        CSSourceInfoGetLineNumber                    = nullptr;
        CSSourceInfoGetSymbol                        = nullptr;
        CSSourceInfoGetSymbolOwner                   = nullptr;

        FPlatformLibrary::FreeDynamicLib(GCoreSymbolicationLibrary);
        GCoreSymbolicationLibrary = nullptr;
        
        GIsInitialized = false;
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

void FMacPlatformStackTrace::GetStackTraceEntryFromAddress(uint64 Address, FStackTraceEntry& OutStackTraceEntry)
{
    if (!InitializeSymbols())
    {
        return;
    }

    pid_t ProcessID = getpid();

    CSSymbolicatorRef Symbolicator = CSSymbolicatorCreateWithPid(ProcessID);
    if(!CSIsNull(Symbolicator))
    {
        CSSourceInfoRef Symbol = CSSymbolicatorGetSourceInfoWithAddressAtTime(Symbolicator, (vm_address_t)Address, kCSNow);
        if(!CSIsNull(Symbol))
        {
            FCString::Strncpy(OutStackTraceEntry.Filename, CSSourceInfoGetPath(Symbol), ARRAY_COUNT(OutStackTraceEntry.Filename));
            FCString::Strncpy(OutStackTraceEntry.FunctionName, CSSymbolGetName(CSSourceInfoGetSymbol(Symbol)), ARRAY_COUNT(OutStackTraceEntry.FunctionName));
            
            OutStackTraceEntry.Line = CSSourceInfoGetLineNumber(Symbol);

            CSSymbolOwnerRef Owner = CSSourceInfoGetSymbolOwner(Symbol);
            if(!CSIsNull(Owner))
            {
                const CHAR* DylibName = CSSymbolOwnerGetName(Owner);
                FCString::Strncpy(OutStackTraceEntry.ModuleName, DylibName, ARRAY_COUNT(OutStackTraceEntry.ModuleName));
            }
        }
        
        CSRelease(Symbolicator);
    }
}
