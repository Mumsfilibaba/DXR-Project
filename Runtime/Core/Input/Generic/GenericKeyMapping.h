#pragma once
#include "Core/Input/InputCodes.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif


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

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif