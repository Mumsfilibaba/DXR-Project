#pragma once

#if defined(PLATFORM_MACOS)
#include "Core/Input/InputCodes.h"
#include "Core/Containers/StaticArray.h"

class CMacKeyMappings
{
    friend class CMacApplication;

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

    /* Retrieve the mouse-button from the mouse-button index */
    static FORCEINLINE EMouseButton GetButtonFromIndex( uint32 ButtonIndex )
    {
        return ButtonFromButtonIndex[ButtonIndex];
    }

    /* Retrieve the mouse-button-index from the mouse-button */
    static FORCEINLINE uint32 GetButtonFromIndex( EMouseButton Button )
    {
        return static_cast<uint32>(ScanCodeFromKeyCodeTable[KeyCode]);
    }

private:

    static void Init();

    /* Lookup table for converting from scan-code to enum */
    static TStaticArray<EKey, 256> KeyCodeFromScanCode[256];

    /* Lookup table for converting from scan-code to enum */
    static TStaticArray<uint16, 256> ScanCodeFromKeyCode[256];

    /* Converts the button-index to mouse button */
    static TStaticArray<EMouseButton, EMouseButton::MouseButton_Count> ButtonFromButtonIndex;

    /* Converts the button-index to mouse button */
    static TStaticArray<uint8, EMouseButton::MouseButton_Count> ButtonIndexFromButton;
};

#endif