#if defined(PLATFORM_WINDOWS)
#include "WindowsApplicationMisc.h"

#include "Core/Application/ModifierKeyState.h"

void CWindowsApplicationMisc::PumpMessages( bool UntilEmpty )
{
}

SModifierKeyState CWindowsApplicationMisc::GetModifierKeyState()
{
    uint32 ModifierMask = 0;
    if ( GetKeyState( VK_CONTROL ) & 0x8000 )
    {
        ModifierMask |= EModifierFlag::ModifierFlag_Ctrl;
    }
    if ( GetKeyState( VK_MENU ) & 0x8000 )
    {
        ModifierMask |= EModifierFlag::ModifierFlag_Alt;
    }
    if ( GetKeyState( VK_SHIFT ) & 0x8000 )
    {
        ModifierMask |= EModifierFlag::ModifierFlag_Shift;
    }
    if ( GetKeyState( VK_CAPITAL ) & 1 )
    {
        ModifierMask |= EModifierFlag::ModifierFlag_CapsLock;
    }
    if ( (GetKeyState( VK_LWIN ) | GetKeyState( VK_RWIN )) & 0x8000 )
    {
        ModifierMask |= EModifierFlag::ModifierFlag_Super;
    }
    if ( GetKeyState( VK_NUMLOCK ) & 1 )
    {
        ModifierMask |= EModifierFlag::ModifierFlag_NumLock;
    }

    return SModifierKeyState( ModifierMask );
}

#endif

