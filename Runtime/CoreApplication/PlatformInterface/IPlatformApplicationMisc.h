#pragma once 
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct IPlatformApplicationMisc
{
    /**
     * @brief Show a modal error dialog with a single dismiss button, blocking until the user closes it
     * @param Title Caption shown in the title of the dialog
     * @param Message Body text shown in the dialog
     */
    static FORCEINLINE void MessageBox(const String& Title, const String& Message)
    {
    }

    /**
     * @brief Dispatch queued OS messages to their handlers, called on the thread owning the event loop
     * @param bUntilEmpty Drain the queue rather than dispatching a single message. Neither case waits for one to arrive
     */
    static FORCEINLINE void PumpMessages(bool bUntilEmpty)
    {
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
