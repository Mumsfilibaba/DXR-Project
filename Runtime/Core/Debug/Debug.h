#pragma once
#include "Core/Platform/PlatformMisc.h"

#ifdef OutputDebugString
     #undef OutputDebugString
#endif

#ifdef OutputDebugFormat
     #undef OutputDebugFormat
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FDebug - Class for easy access to debugging functions 

struct FDebug
{
    static FORCEINLINE void OutputDebugString(const FString& Message)
    {
        FPlatformMisc::OutputDebugString(Message);
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
        FString FormattedMessage = FString::CreateFormattedArgs(Format, ArgumentList);
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
        return FPlatformMisc::IsDebuggerPresent();
    }
};
