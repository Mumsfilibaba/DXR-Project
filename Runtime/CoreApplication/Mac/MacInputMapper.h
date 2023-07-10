#pragma once
#include "Core/Input/InputCodes.h"
#include "Core/Containers/StaticArray.h"
#include "CoreApplication/Generic/GenericInputMapper.h"

class COREAPPLICATION_API FMacInputMapper final : public EGenericInputMapper
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
    static TStaticArray<EKeyName::Type, NumKeys>                         KeyCodeFromScanCodeTable;
    static TStaticArray<uint16, NumKeys>                                 ScanCodeFromKeyCodeTable;
    static TStaticArray<EMouseButtonName::Type, EMouseButtonName::Count> ButtonFromButtonIndex;
    static TStaticArray<uint8, EMouseButtonName::Count>                  ButtonIndexFromButton;
};
