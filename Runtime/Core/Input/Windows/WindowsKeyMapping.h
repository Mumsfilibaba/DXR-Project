#pragma once

#if PLATFORM_WINDOWS
#include "Core/Core.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Input/InputCodes.h"
#include "Core/Input/Interface/PlatformKeyMapping.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Class that maps key-code from scan-codes and the reverse

class CORE_API CWindowsKeyMapping : public CPlatformKeyMapping
{
    enum
    {
        NumKeys = 512
    };

public:

    /* Init the key-tables */
    static void Initialize();

    /* Retrieve the key-code from the scan-code */
    static FORCEINLINE EKey GetKeyCodeFromScanCode(uint32 ScanCode)
    {
        return KeyCodeFromScanCodeTable[ScanCode];
    }

    /* Retrieve the scan-code from the key-code */
    static FORCEINLINE uint32 GetScanCodeFromKeyCode(EKey KeyCode)
    {
        return static_cast<uint32>(ScanCodeFromKeyCodeTable[KeyCode]);
    }

private:

    /* Table to convert from ScanCode to KeyCode */
    static EKey KeyCodeFromScanCodeTable[NumKeys];
    /* Table to convert from KeyCode to ScanCode */
    static uint16 ScanCodeFromKeyCodeTable[NumKeys];
};

#endif