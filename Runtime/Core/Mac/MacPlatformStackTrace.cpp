#include "Core/Mac/MacPlatformStackTrace.h"
#include "Core/Platform/PlatformLibrary.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include <dlfcn.h>
#include <execinfo.h>
#include <mach/mach.h>
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

static PFN_CSIsNull  CSIsNull  = nullptr;
static PFN_CSRelease CSRelease = nullptr;

static PFN_CSSymbolicatorCreateWithPid                  CSSymbolicatorCreateWithPid                  = nullptr;
static PFN_CSSymbolicatorGetSourceInfoWithAddressAtTime CSSymbolicatorGetSourceInfoWithAddressAtTime = nullptr;

static PFN_CSSymbolGetName      CSSymbolGetName      = nullptr;
static PFN_CSSymbolOwnerGetName CSSymbolOwnerGetName = nullptr;

static PFN_CSSourceInfoGetPath        CSSourceInfoGetPath        = nullptr;
static PFN_CSSourceInfoGetLineNumber  CSSourceInfoGetLineNumber  = nullptr;
static PFN_CSSourceInfoGetSymbol      CSSourceInfoGetSymbol      = nullptr;
static PFN_CSSourceInfoGetSymbolOwner CSSourceInfoGetSymbolOwner = nullptr;

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
    }

    ++GSymbolsRefCount;
    return true;
}

void FMacPlatformStackTrace::ReleaseSymbols()
{
    TScopedLock Lock(GSymbolsCS);

    if ((GSymbolsRefCount > 0) && (--GSymbolsRefCount == 0))
    {
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

static void SymbolicateWithCoreSymbolication(uint64 Address, FStackTraceEntry& OutStackTraceEntry)
{
    pid_t ProcessID = getpid();

    CSSymbolicatorRef Symbolicator = CSSymbolicatorCreateWithPid(ProcessID);
    if(!CSIsNull(Symbolicator))
    {
        CSSourceInfoRef Symbol = CSSymbolicatorGetSourceInfoWithAddressAtTime(Symbolicator, (vm_address_t)Address, kCSNow);
        if(!CSIsNull(Symbol))
        {
            // Any of these can come back null for an address without full debug information
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
        
        CSRelease(Symbolicator);
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
