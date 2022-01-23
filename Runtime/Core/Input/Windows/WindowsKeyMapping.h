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

    static void Initialize();

    static FORCEINLINE EKey GetKeyCodeFromScanCode(uint32 ScanCode)
    {
        return KeyCodeFromScanCodeTable[ScanCode];
    }

    static FORCEINLINE uint32 GetScanCodeFromKeyCode(EKey KeyCode)
    {
        return static_cast<uint32>(ScanCodeFromKeyCodeTable[KeyCode]);
    }

private:
    static EKey KeyCodeFromScanCodeTable[NumKeys];
    static uint16 ScanCodeFromKeyCodeTable[NumKeys];
};

#endif