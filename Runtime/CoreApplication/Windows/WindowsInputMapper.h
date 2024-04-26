#pragma once
#include "Core/Core.h"
#include "Core/Containers/StaticArray.h"
#include "CoreApplication/Generic/InputCodes.h"
#include "CoreApplication/Generic/GenericInputMapper.h"

class COREAPPLICATION_API FWindowsInputMapper final : public EGenericInputMapper
{
    inline static constexpr uint32 NumKeys = 512;

public:
    static void Initialize();

    static FORCEINLINE EKeyboardKeyName::Type GetKeyCodeFromScanCode(uint32 ScanCode)
    {
        return KeyCodeFromScanCodeTable[ScanCode];
    }

    static FORCEINLINE uint32 GetScanCodeFromKeyCode(EKeyboardKeyName::Type KeyCode)
    {
        return static_cast<uint32>(ScanCodeFromKeyCodeTable[KeyCode]);
    }

private:
    static TStaticArray<EKeyboardKeyName::Type, NumKeys> KeyCodeFromScanCodeTable;
    static TStaticArray<uint16, NumKeys>                 ScanCodeFromKeyCodeTable;
};
