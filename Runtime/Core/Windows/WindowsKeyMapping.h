#pragma once
#include "Core/Core.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Input/InputCodes.h"
#include "Core/Generic/GenericKeyMapping.h"

class CORE_API FWindowsKeyMapping final : public FGenericKeyMapping
{
    enum { kNumKeys = 512 };

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
    static TStaticArray<EKey  , kNumKeys> KeyCodeFromScanCodeTable;
    static TStaticArray<uint16, kNumKeys> ScanCodeFromKeyCodeTable;
};
