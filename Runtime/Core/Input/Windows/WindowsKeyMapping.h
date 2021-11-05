#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/CoreModule.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Input/InputCodes.h"
#include "Core/Input/Interface/PlatformKeyMapping.h"

/* Class that maps key-code from scan-codes and the reverse */
class CORE_API CWindowsKeyMapping : public CPlatformKeyMapping
{
    friend class CWindowsApplication;

public:

    /* Retrieve the key-code from the scan-code */
    static FORCEINLINE EKey GetKeyCodeFromScanCode( uint32 ScanCode )
    {
        return KeyCodeFromScanCodeTable[ScanCode];
    }

    /* Retrieve the scan-code from the key-code */
    static FORCEINLINE uint32 GetScanCodeFromKeyCode( EKey KeyCode )
    {
        return static_cast<uint32>(ScanCodeFromKeyCodeTable[KeyCode]);
    }

private:

    /* Init the key-tables */
    static void Init();

    /* Table to convert from ScanCode to KeyCode */
    static TStaticArray<EKey, 512>   KeyCodeFromScanCodeTable;

    /* Table to convert from KeyCode to ScanCode */
    static TStaticArray<uint16, 512> ScanCodeFromKeyCodeTable;

};

#endif