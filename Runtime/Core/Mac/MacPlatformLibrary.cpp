#include "Core/Mac/MacPlatformLibrary.h"
#include "Core/Mac/MacPlatformMisc.h"
#include "Core/Mac/MacPlatformFile.h"

#include <sys/param.h>

// Lazy mode resolves symbols when they are called for the first time, disable to load everything at loadtime
#define ENABLE_LIBRARY_LAZY_MODE (1)

// Enables logging to the NSLog
#define ENABLE_DYLIB_ERROR_LOGGING (0)

static const CHAR_T* BuildExecutableRelativeDir(CHAR_T* OutBuffer, uint32 BufferSize, const CHAR_T* Suffix)
{
    const CHAR_T* ExecutablePath = FMacPlatformFile::GetExecutablePath();
    
    const CHAR_T* LastSeparator = CString::Strrchr(ExecutablePath, '/');
    if (!LastSeparator)
    {
        return TEXT("");
    }
    
    const uint32 DirLength = static_cast<uint32>(LastSeparator - ExecutablePath) + 1;
    if (DirLength + CString::Strlen(Suffix) >= BufferSize)
    {
        return TEXT("");
    }
    
    Memory::Memcpy(OutBuffer, ExecutablePath, DirLength * sizeof(CHAR_T));
    CString::Strcat(OutBuffer, Suffix);
    return OutBuffer;
}

static const CHAR_T* GetExecutableDir()
{
    static CHAR_T StaticExecutableDir[MAXPATHLEN] = { 0 };
    
    if (!StaticExecutableDir[0])
    {
        return BuildExecutableRelativeDir(StaticExecutableDir, sizeof(StaticExecutableDir), TEXT(""));
    }
    
    return StaticExecutableDir;
}

static const CHAR_T* GetBundleFrameworksDir()
{
    static CHAR_T StaticFrameworksDir[MAXPATHLEN] = { 0 };
    
    if (!StaticFrameworksDir[0])
    {
        return BuildExecutableRelativeDir(StaticFrameworksDir, sizeof(StaticFrameworksDir), TEXT("../Frameworks/"));
    }
    
    return StaticFrameworksDir;
}

#if ENABLE_LIBRARY_LAZY_MODE
    #define DEFAULT_DYLIB_MODE RTLD_LAZY
#else
    #define DEFAULT_DYLIB_MODE RTLD_NOW
#endif

static void* SafeLoadDynamicLib(const CHAR_T* LibraryName, int32 Mode = DEFAULT_DYLIB_MODE)
{
    // Try and avoid dynamic memory allocation inside of this function
    const CHAR_T* Prefix    = FMacPlatformLibrary::GetDynamicLibPrefix();
    const CHAR_T* Extension = FMacPlatformLibrary::GetDynamicLibExtension();
    
    // Concat the realname
    constexpr uint32 MaxNameLength = 256;
    const uint32 FullLength = CString::Strlen(LibraryName) + CString::Strlen(Prefix) + CString::Strlen(Extension);
    if (FullLength >= MaxNameLength)
    {
        return nullptr;
    }
    
    CHAR_T RealName[MaxNameLength];
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
    const CHAR_T* Error = ::dlerror();
    if (Error)
    {
        FMacPlatformMisc::OutputDebugString(Error);
    }
#endif
    
    const CHAR_T* Paths[] =
    {
        TEXT("@rpath/"),                // Resolved through the LC_RPATH of this image
        GetBundleFrameworksDir(),
        GetExecutableDir(),
        TEXT("/usr/local/lib/"),        // SDK-installed libraries such as the Vulkan loader
    };
    
    constexpr uint32 MaxFullPathLength = MaxNameLength + MAXPATHLEN;
    CHAR_T FullPath[MaxFullPathLength];
    for (const CHAR_T* Path : Paths)
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
        const CHAR_T* Error = ::dlerror();
        if (Error)
        {
            FMacPlatformMisc::OutputDebugString(Error);
        }
    #endif
    }
    
    // Finally, if we still did not find the handle, let's return nullptr
    return nullptr;
}

void* FMacPlatformLibrary::LoadDynamicLib(const CHAR_T* LibraryName)
{
    void* Handle = SafeLoadDynamicLib(LibraryName);
    if (Handle)
    {
        return Handle;
    }
    
    return nullptr;
}

void* FMacPlatformLibrary::LoadDynamicLibFromFile(const CHAR_T* Filepath)
{
    return ::dlopen(Filepath, DEFAULT_DYLIB_MODE);
}

bool FMacPlatformLibrary::ReadStringExport(const CHAR_T* Filepath, const CHAR_T* SymbolName, FString& OutValue)
{
    void* Handle = ::dlopen(Filepath, RTLD_LAZY | RTLD_LOCAL);
    if (!Handle)
    {
        return false;
    }

    const ANSICHAR* Value = static_cast<const ANSICHAR*>(::dlsym(Handle, SymbolName));
    if (Value)
    {
        OutValue = StringCast<CHAR_T>(Value).ToString();
    }

    ::dlclose(Handle);
    return Value != nullptr;
}

void* FMacPlatformLibrary::GetLoadedHandle(const CHAR_T* LibraryName)
{ 
    // RTLD_NOLOAD returns a handle only if the library is already loaded, for parity with windows
    return SafeLoadDynamicLib(LibraryName, RTLD_LAZY | RTLD_NOLOAD);
}
