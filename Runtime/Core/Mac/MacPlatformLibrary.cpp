#include "Core/Mac/MacPlatformLibrary.h"
#include "Core/Mac/MacPlatformMisc.h"
#include "Core/Mac/MacPlatformFile.h"

#include <sys/param.h>

// Lazy mode resolves symbols when they are called for the first time, disable to load everything at loadtime
#define ENABLE_LIBRARY_LAZY_MODE (1)

// Enables logging to the NSLog
#define ENABLE_DYLIB_ERROR_LOGGING (0)

static const CHAR* BuildExecutableRelativeDir(CHAR* OutBuffer, uint32 BufferSize, const CHAR* Suffix)
{
    const CHAR* ExecutablePath = FMacPlatformFile::GetExecutablePath();
    
    const CHAR* LastSeparator = CString::Strrchr(ExecutablePath, '/');
    if (!LastSeparator)
    {
        return "";
    }
    
    const uint32 DirLength = static_cast<uint32>(LastSeparator - ExecutablePath) + 1;
    if (DirLength + CString::Strlen(Suffix) >= BufferSize)
    {
        return "";
    }
    
    Memory::Memcpy(OutBuffer, ExecutablePath, DirLength);
    CString::Strcat(OutBuffer, Suffix);
    return OutBuffer;
}

static const CHAR* GetExecutableDir()
{
    static CHAR StaticExecutableDir[MAXPATHLEN] = { 0 };
    
    if (!StaticExecutableDir[0])
    {
        return BuildExecutableRelativeDir(StaticExecutableDir, sizeof(StaticExecutableDir), "");
    }
    
    return StaticExecutableDir;
}

static const CHAR* GetBundleFrameworksDir()
{
    static CHAR StaticFrameworksDir[MAXPATHLEN] = { 0 };
    
    if (!StaticFrameworksDir[0])
    {
        return BuildExecutableRelativeDir(StaticFrameworksDir, sizeof(StaticFrameworksDir), "../Frameworks/");
    }
    
    return StaticFrameworksDir;
}

#if ENABLE_LIBRARY_LAZY_MODE
    #define DEFAULT_DYLIB_MODE RTLD_LAZY
#else
    #define DEFAULT_DYLIB_MODE RTLD_NOW
#endif

static void* SafeLoadDynamicLib(const CHAR* LibraryName, int32 Mode = DEFAULT_DYLIB_MODE)
{
    // Try and avoid dynamic memory allocation inside of this function
    const CHAR* Prefix    = FMacPlatformLibrary::GetDynamicLibPrefix();
    const CHAR* Extension = FMacPlatformLibrary::GetDynamicLibExtension();
    
    // Concat the realname
    constexpr uint32 MaxNameLength = 256;
    const uint32 FullLength = CString::Strlen(LibraryName) + CString::Strlen(Prefix) + CString::Strlen(Extension);
    if (FullLength >= MaxNameLength)
    {
        return nullptr;
    }
    
    CHAR RealName[MaxNameLength];
    Memory::Memzero(RealName, sizeof(RealName));
    
    CString::Strcat(RealName, Prefix);
    CString::Strcat(RealName, LibraryName);
    CString::Strcat(RealName, Extension);
    
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
    
    const CHAR* Paths[] =
    {
        "@rpath/",                // Resolved through the LC_RPATH of this image
        GetBundleFrameworksDir(),
        GetExecutableDir(),
        "/usr/local/lib/",        // SDK-installed libraries such as the Vulkan loader
    };
    
    constexpr uint32 MaxFullPathLength = MaxNameLength + MAXPATHLEN;
    CHAR FullPath[MaxFullPathLength];
    for (const CHAR* Path : Paths)
    {
        const uint32 FullPathLength = FullLength + CString::Strlen(Path);
        if (FullPathLength >= MaxFullPathLength)
        {
            continue;
        }
        
        Memory::Memzero(FullPath, sizeof(FullPath));
        CString::Strcat(FullPath, Path);
        CString::Strcat(FullPath, RealName);
        
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

void* FMacPlatformLibrary::LoadDynamicLib(const CHAR* LibraryName)
{
    void* Handle = SafeLoadDynamicLib(LibraryName);
    if (Handle)
    {
        return Handle;
    }
    
    return nullptr;
}

void* FMacPlatformLibrary::GetLoadedHandle(const CHAR* LibraryName)
{ 
    // RTLD_NOLOAD returns a handle only if the library is already loaded, for parity with windows
    return SafeLoadDynamicLib(LibraryName, RTLD_LAZY | RTLD_NOLOAD);
}
