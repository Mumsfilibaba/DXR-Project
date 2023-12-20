#include "MacLibrary.h"
#include "MacPlatformMisc.h"

// Lazy mode resolves symbols when they are called for the first time, disable to load everything at loadtime
#define ENABLE_LIBRARY_LAZY_MODE (1)

// Enables logging to the NSLog
#define ENABLE_DYLIB_ERROR_LOGGING (0)

static void* SafeLoadDynamicLib(const CHAR* LibraryName)
{
    // Try and avoid dynamic memory allocation inside of this function
    const CHAR* Prefix    = FMacLibrary::GetDynamicLibPrefix();
    const CHAR* Extension = FMacLibrary::GetDynamicLibExtension();
    
    // Concat the realname
    constexpr uint32 MaxNameLength = 256;
    const uint32 FullLength = FCString::Strlen(LibraryName) + FCString::Strlen(Prefix) + FCString::Strlen(Extension);
    if (FullLength >= MaxNameLength)
    {
        return nullptr;
    }
    
    CHAR RealName[MaxNameLength];
    FMemory::Memzero(RealName, sizeof(RealName));
    
    FCString::Strcat(RealName, Prefix);
    FCString::Strcat(RealName, LibraryName);
    FCString::Strcat(RealName, Extension);
    
#if ENABLE_LIBRARY_LAZY_MODE
    const int32 Mode = RTLD_LAZY;
#else
    const int32 Mode = RTLD_NOW;
#endif
    
    // Now try and load
    void* Handle = ::dlopen(RealName, Mode);
    
    // We found the library so lets return the handle
    if (Handle)
    {
        return Handle;
    }
    
#if ENABLE_DYLIB_ERROR_LOGGING
    // If we did not find the library, lets check why
    const CHAR* Error = ::dlerror();
    if (Error)
    {
        FMacPlatformMisc::OutputDebugString(Error);
    }
#endif
    
    // Since the local lib folder is not check by default, check it as well
    const CHAR* Paths[] =
    {
        "usr/local/lib/",
    };
    
    constexpr uint32 MaxFullPathLength = MaxNameLength + 128;
    CHAR FullPath[MaxFullPathLength];
    for (const CHAR* Path : Paths)
    {
        const uint32 FullPathLength = FullLength + FCString::Strlen(Path);
        if (FullPathLength >= MaxFullPathLength)
        {
            continue;
        }
        
        FMemory::Memzero(FullPath, sizeof(FullPath));
        FCString::Strcat(FullPath, Path);
        FCString::Strcat(FullPath, RealName);
        
        // Try and load the handle again
        Handle = ::dlopen(FullPath, Mode);
        
        // We found the library so lets return the handle
        if (Handle)
        {
            return Handle;
        }
        
    #if ENABLE_DYLIB_ERROR_LOGGING
        // If we did not find the library, lets check why
        const CHAR* Error = ::dlerror();
        if (Error)
        {
            FMacPlatformMisc::OutputDebugString(Error);
        }
    #endif
    }
    
    // Finally, if we still did not find the handle, let's return nullptr
    return nullptr;
}

void* FMacLibrary::LoadDynamicLib(const CHAR* LibraryName)
{
    void* Handle = SafeLoadDynamicLib(LibraryName);
    if (Handle)
    {
        return Handle;
    }
    
    return nullptr;
}

void* FMacLibrary::GetLoadedHandle(const CHAR* LibraryName)
{ 
    void* Handle = SafeLoadDynamicLib(LibraryName);
    
    // Handle is ref-counted so release the new ref-count in order to have parity with windows
    if (Handle)
    {
        ::dlclose(Handle);
        return Handle;
    }
    
    return nullptr;
}
