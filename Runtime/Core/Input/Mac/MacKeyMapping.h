#pragma once

#if PLATFORM_MACOS
#include "Core/Input/InputCodes.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Input/Interface/PlatformKeyMappingInterface.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Class that maps key-code from scan-codes and the reverse

class CMacKeyMapping : public CPlatformKeyMappingInterface
{
    enum
    {
        NumKeys = 256
    };

public:

    /** Initialize key-tables */
    static void Initialize();
    
    /**
     * Retrieve the key-code from the scan-code 
     * 
     * @param Scan-Code: Scan-Code to convert into a engine key-code
     * @return: Returns a engine key-code
     */
    static FORCEINLINE EKey GetKeyCodeFromScanCode(uint32 ScanCode)
    {
        return KeyCodeFromScanCodeTable[ScanCode];
    }

    /**
     * Retrieve the scan-code from the key-code
     *
     * @param Key-Code: Engine key-code to convert into a scan-code
     * @return: Returns a scan-code representing the engine key-code
     */
    static FORCEINLINE uint32 GetScanCodeFromKeyCode(EKey KeyCode)
    {
        return static_cast<uint32>(ScanCodeFromKeyCodeTable[KeyCode]);
    }

    /**
     * Retrieve the mousebutton-code from mousebutton-index
     *
     * @param ButtonIndex: Mousebutton-index for mousebutton-code
     * @return: Returns a engine mousebutton-code representing the buttonindex
     */
    static FORCEINLINE EMouseButton GetButtonFromIndex(uint32 ButtonIndex)
    {
        return ButtonFromButtonIndex[ButtonIndex];
    }

    /**
     * Retrieve the mousebutton-code from mousebutton-index
     *
     * @param Button: Mousebutton-code for mousebutton index
     * @return: Returns a mousebutton-index representing the mousebutton-code
     */
    static FORCEINLINE uint32 GetButtonFromIndex(EMouseButton Button)
    {
        return static_cast<uint32>(ButtonIndexFromButton[Button]);
    }

private:

    /** Lookup table for converting from scan-code to enum */
    static EKey KeyCodeFromScanCodeTable[NumKeys];
    /** Lookup table for converting from scan-code to enum */
    static uint16 ScanCodeFromKeyCodeTable[NumKeys];
    /** Converts the button-index to mouse button */
    static EMouseButton ButtonFromButtonIndex[EMouseButton::MouseButton_Count];
    /** Converts the button-index to mouse button */
    static uint8 ButtonIndexFromButton[EMouseButton::MouseButton_Count];
};

#endif
