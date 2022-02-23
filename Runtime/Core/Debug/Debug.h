#pragma once
#include "CoreApplication/Platform/PlatformDebugMisc.h"

#ifdef OutputDebugString
#undef OutputDebugString
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Class for easy access to debugging functions 

class Debug
{
public:

    /**
     * If the debugger is attached, a breakpoint will be set at this point of the code
     */
    static FORCEINLINE void DebugBreak()
    {
        PlatformDebugMisc::DebugBreak();
    }

    /**
     * Outputs a debug string to the attached debugger
     *
     * @param Message: Message to print to the attached debugger
     */
    static FORCEINLINE void OutputDebugString(const String& Message)
    {
        PlatformDebugMisc::OutputDebugString(Message);
    }

    /**
     * Outputs a formatted debug string to the attached debugger
     *
     * @param Format: Format to print to the attached debugger, followed by arguments for the string
     */
    static FORCEINLINE void OutputDebugFormat(const char* Format, ...)
    {
        va_list ArgumentList;
        va_start(ArgumentList, Format);

        String FormattedMessage = String::CreateFormatedV(Format, ArgumentList);

        va_end(ArgumentList);

        OutputDebugString(FormattedMessage);
    }

    /**
     * Checks weather or not the application is running inside a debugger
     *
     * @return: Returns true if the debugger is present, otherwise false
     */
    static FORCEINLINE bool IsDebuggerPresent()
    {
        return PlatformDebugMisc::IsDebuggerPresent();
    }
};