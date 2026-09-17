#include "Core/Platform/PlatformLibrary.h"
#include "CoreApplication/Windows/WindowsApplication.h"
#include "CoreApplication/Windows/WindowsApplicationMisc.h"

#if PLATFORM_WINDOWS_11
static DWORD GetWindowsBuildNumber()
{
    typedef LONG(WINAPI* PFN_RtlGetVersion)(OSVERSIONINFOW*);

    void* NtDll = FPlatformLibrary::GetLoadedHandle("ntdll");
    if (!NtDll)
    {
        return 0;
    }

    PFN_RtlGetVersion RtlGetVersion = FPlatformLibrary::LoadSymbol<PFN_RtlGetVersion>("RtlGetVersion", NtDll);
    if (!RtlGetVersion)
    {
        return 0;
    }

    OSVERSIONINFOW VersionInfo = {};
    VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);

    return (RtlGetVersion(&VersionInfo) == 0) ? VersionInfo.dwBuildNumber : 0;
}
#endif

void FWindowsApplicationMisc::PumpMessages(bool bUntilEmpty)
{
    MSG Message;

    do
    {
        BOOL Result = PeekMessage(&Message, 0, 0, 0, PM_REMOVE);
        if (!Result)
        {
            break;
        }

        TranslateMessage(&Message);
        DispatchMessage(&Message);
    } while (bUntilEmpty);
}

bool FWindowsApplicationMisc::SupportsRoundedWindowCorners()
{
#if PLATFORM_WINDOWS_11
    static const bool bSupported = GetWindowsBuildNumber() >= 22000;
    return bSupported;
#else
    return false;
#endif
}
