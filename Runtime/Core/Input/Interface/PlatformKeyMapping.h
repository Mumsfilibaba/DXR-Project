#pragma once
#include "Core/Input/InputCodes.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

class CPlatformKeyMapping
{
public:

    /* Initialize keytables */
    static FORCEINLINE void Initialize() {}

    /* Retrieve the key-code from the scan-code */
    static FORCEINLINE EKey GetKeyCodeFromScanCode( uint32 ScanCode ) { return EKey::Key_Unknown; }

    /* Retrieve the scan-code from the key-code */
    static FORCEINLINE uint32 GetScanCodeFromKeyCode( EKey KeyCode ) { return 0; }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif