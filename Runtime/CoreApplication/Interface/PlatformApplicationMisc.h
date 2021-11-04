#pragma once 
#include "Core/Application/ModifierKeyState.h"
#include "Core/Containers/String.h"

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

class CPlatformApplicationMisc
{
public:

    /* Takes the title of the MessageBox and the message to be displayed */
    static FORCEINLINE void MessageBox( const CString& Title, const CString& Message ) {}

    /* Sends a Exit Message to the application with a certain ExitCode, this way the application instance is not required to pump the messages */
    static FORCEINLINE void RequestExit( int32 ExitCode ) {}

    /* Pumps the application's message queue, this way the application instance is not required to pump the messages */
    static FORCEINLINE void PumpMessages( bool UntilEmpty ) {}

    /* Retrieves the state of modifier keys */
    static FORCEINLINE SModifierKeyState GetModifierKeyState() { return SModifierKeyState(); }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop

#endif
