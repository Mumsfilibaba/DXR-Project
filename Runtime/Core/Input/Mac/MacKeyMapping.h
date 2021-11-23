#pragma once

#if PLATFORM_MACOS
#include "Core/Input/InputCodes.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Input/Interface/PlatformKeyMapping.h"

class CMacKeyMapping : public CPlatformKeyMapping
{
    enum
    {
        NumKeys = 256
    };

public:

    /* Init the key-tables */
    static void Initialize();
    
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

    /* Retrieve the mouse-button from the mouse-button index */
    static FORCEINLINE EMouseButton GetButtonFromIndex( uint32 ButtonIndex )
    {
        return ButtonFromButtonIndex[ButtonIndex];
    }

    /* Retrieve the mouse-button-index from the mouse-button */
    static FORCEINLINE uint32 GetButtonFromIndex( EMouseButton Button )
    {
        return static_cast<uint32>(ButtonIndexFromButton[Button]);
    }

private:

    /* Lookup table for converting from scan-code to enum */
    static EKey KeyCodeFromScanCodeTable[NumKeys];

    /* Lookup table for converting from scan-code to enum */
    static uint16 ScanCodeFromKeyCodeTable[NumKeys];

    /* Converts the button-index to mouse button */
    static EMouseButton ButtonFromButtonIndex[EMouseButton::MouseButton_Count];

    /* Converts the button-index to mouse button */
    static uint8 ButtonIndexFromButton[EMouseButton::MouseButton_Count];
};

#endif
