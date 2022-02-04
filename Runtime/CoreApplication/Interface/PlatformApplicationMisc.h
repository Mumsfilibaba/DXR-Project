#pragma once 
#include "Core/Input/ModifierKeyState.h"
#include "Core/Containers/String.h"

#include "CoreApplication/CoreApplication.h"

#ifdef MessageBox
#undef MessageBox
#endif

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

// TODO: Enable other types of Modal windows for supported platforms

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Platform interface for miscellaneous application utility functions

class CPlatformApplicationMisc
{
public:

    /**
     * Takes the title of the MessageBox and the message to be displayed
     * 
     * @param Title: Title of the message-box
     * @param Message: Message of the message-box
     */
    static FORCEINLINE void MessageBox(const String& Title, const String& Message) { }

    /**
     * Sends a Exit Message to the application with a certain ExitCode, this way the application instance is not required to pump the messages 
     * 
     * @param ExitCode: Exit code for the exit-event
     */
    static FORCEINLINE void RequestExit(int32 ExitCode) { }

    /**
     * Pumps the application's message queue, this way the application instance is not required to pump the messages 
     * 
     * @param bUntilEmpty: Pump messages until there are no longer any messages in the queue
     */
    static FORCEINLINE void PumpMessages(bool bUntilEmpty) { }

    /**
     * Retrieves the state of modifier keys
     * 
     * @return: Returns the current modifier-key state 
     */
    static FORCEINLINE SModifierKeyState GetModifierKeyState() { return SModifierKeyState(); }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop

#endif
