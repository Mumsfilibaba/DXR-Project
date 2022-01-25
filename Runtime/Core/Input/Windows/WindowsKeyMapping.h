#pragma once

#if PLATFORM_WINDOWS
#include "Core/Core.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Input/InputCodes.h"
#include "Core/Input/Interface/PlatformInterfaceKeyMapping.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Class that maps key-code from scan-codes and the reverse

class CORE_API CWindowsKeyMapping : public CPlatformKeyMappingInterface
{
    enum
    {
        NumKeys = 512
    };

public:

    /** Initialize the key-tables */
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

private:
    static EKey KeyCodeFromScanCodeTable[NumKeys];
    static uint16 ScanCodeFromKeyCodeTable[NumKeys];
};

#endif