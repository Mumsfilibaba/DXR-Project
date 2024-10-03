#pragma once
#include "Core/Containers/StaticArray.h"
#include "CoreApplication/Generic/InputCodes.h"
#include "CoreApplication/Generic/GenericInputMapper.h"

class COREAPPLICATION_API FMacInputMapper final : public EGenericInputMapper
{
    inline static constexpr uint32 NumKeys = 256;

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

    static FORCEINLINE EMouseButtonName::Type GetButtonFromIndex(uint32 ButtonIndex)
    {
        return ButtonFromButtonIndex[ButtonIndex];
    }

    static FORCEINLINE uint32 GetButtonFromIndex(EMouseButtonName::Type Button)
    {
        return static_cast<uint32>(ButtonIndexFromButton[Button]);
    }
    
    static NSUInteger TranslateKeyToModifierFlag(EKeyboardKeyName::Type KeyName)
    {
        switch (KeyName)
        {
            case EKeyboardKeyName::LeftShift:
            case EKeyboardKeyName::RightShift:
                return NSEventModifierFlagShift;
            case EKeyboardKeyName::LeftControl:
            case EKeyboardKeyName::RightControl:
                return NSEventModifierFlagControl;
            case EKeyboardKeyName::LeftAlt:
            case EKeyboardKeyName::RightAlt:
                return NSEventModifierFlagOption;
            case EKeyboardKeyName::LeftSuper:
            case EKeyboardKeyName::RightSuper:
                return NSEventModifierFlagCommand;
            case EKeyboardKeyName::CapsLock:
                return NSEventModifierFlagCapsLock;
        }

        return 0;
    }

private:
    static TStaticArray<EKeyboardKeyName::Type, NumKeys>                 KeyCodeFromScanCodeTable;
    static TStaticArray<uint16, NumKeys>                                 ScanCodeFromKeyCodeTable;
    static TStaticArray<EMouseButtonName::Type, EMouseButtonName::Count> ButtonFromButtonIndex;
    static TStaticArray<uint8, EMouseButtonName::Count>                  ButtonIndexFromButton;
};
