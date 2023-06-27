#pragma once
#include "Core/Containers/StaticArray.h"
#include "Core/Input/InputCodes.h"
#include "Core/Generic/GenericKeyMapping.h"

class CORE_API FMacKeyMapping final : public FGenericKeyMapping
{
    inline static constexpr uint32 NumKeys = 256;

public:
    static void Initialize();
    
    static FORCEINLINE EKeyName::Type GetKeyCodeFromScanCode(uint32 ScanCode)
    {
        return KeyCodeFromScanCodeTable[ScanCode];
    }

    static FORCEINLINE uint32 GetScanCodeFromKeyCode(EKeyName::Type KeyCode)
    {
        return static_cast<uint32>(ScanCodeFromKeyCodeTable[KeyCode]);
    }

    static FORCEINLINE EMouseButtonName::Type GetButtonFromIndex(uint32 ButtonIndex)
    {
        return ButtonFromButtonIndex[ButtonIndex];
    }

    static FORCEINLINE uint32 GetButtonFromIndex(EMouseButtonName::Type Button)
    {
        return static_cast<uint32>(ButtonIndexFromButton[Button]);
    }

private:
    static TStaticArray<EKeyName, NumKeys>                                 KeyCodeFromScanCodeTable;
    static TStaticArray<uint16, NumKeys>                               ScanCodeFromKeyCodeTable;
    static TStaticArray<EMouseButtonName, EMouseButtonName::Count> ButtonFromButtonIndex;
    static TStaticArray<uint8, EMouseButtonName::Count>        ButtonIndexFromButton;
};
