#pragma once
#include "Core/Containers/StaticArray.h"
#include "Core/Input/InputCodes.h"
#include "Core/Input/Generic/GenericKeyMapping.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacKeyMapping

class CMacKeyMapping : public CGenericKeyMapping
{
    enum
    {
        kNumKeys = 256
    };

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericKeyMapping Interface

    static void Initialize();
    
    static FORCEINLINE EKey GetKeyCodeFromScanCode(uint32 ScanCode)
    {
        return KeyCodeFromScanCodeTable[ScanCode];
    }

    static FORCEINLINE uint32 GetScanCodeFromKeyCode(EKey KeyCode)
    {
        return static_cast<uint32>(ScanCodeFromKeyCodeTable[KeyCode]);
    }

public:

    /**
     * @brief: Retrieve the mousebutton-code from mousebutton-index
     *
     * @param ButtonIndex: Mousebutton-index for mousebutton-code
     * @return: Returns a engine mousebutton-code representing the buttonindex
     */
    static FORCEINLINE EMouseButton GetButtonFromIndex(uint32 ButtonIndex)
    {
        return ButtonFromButtonIndex[ButtonIndex];
    }

    /**
     * @brief: Retrieve the mousebutton-code from mousebutton-index
     *
     * @param Button: Mousebutton-code for mousebutton index
     * @return: Returns a mousebutton-index representing the mousebutton-code
     */
    static FORCEINLINE uint32 GetButtonFromIndex(EMouseButton Button)
    {
        return static_cast<uint32>(ButtonIndexFromButton[Button]);
    }

private:

    static TStaticArray<EKey        , kNumKeys>                        KeyCodeFromScanCodeTable;
    static TStaticArray<uint16      , kNumKeys>                        ScanCodeFromKeyCodeTable;
    static TStaticArray<EMouseButton, EMouseButton::MouseButton_Count> ButtonFromButtonIndex;
    static TStaticArray<uint8       , EMouseButton::MouseButton_Count> ButtonIndexFromButton;
};
