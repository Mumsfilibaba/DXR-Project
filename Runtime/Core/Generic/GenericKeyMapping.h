#pragma once
#include "Core/Input/InputCodes.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING


struct FGenericKeyMapping
{
    /** @brief - Initialize key-tables */
    static FORCEINLINE void Initialize() { }

    /**
     * @brief      - Retrieve the key-code from the scan-code 
     * @param Scan - Code - Scan-Code to convert into a engine key-code
     * @return     - Returns a engine key-code
     */
    static FORCEINLINE EKey GetKeyCodeFromScanCode(uint32 ScanCode) { return EKey::Key_Unknown; }

    /**
     * @brief     - Retrieve the scan-code from the key-code
     * @param Key - Code - Engine key-code to convert into a scan-code
     * @return    - Returns a scan-code representing the engine key-code
     */
    static FORCEINLINE uint32 GetScanCodeFromKeyCode(EKey KeyCode) { return 0; }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING