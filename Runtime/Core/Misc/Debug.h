#pragma once
#include "Core/Platform/PlatformMisc.h"

struct FDebug
{
    static FORCEINLINE void OutputDebugString(const FString& Message)
    {
        FPlatformMisc::OutputDebugString(Message.GetCString());
    }

    /**
     * @brief Outputs a formatted debug string to the attached debugger
     * @param InFormat Format to print to the attached debugger, followed by arguments for the string
     */
    template<typename... ArgTypes>
    static FORCEINLINE void OutputDebugFormat(const CHAR* InFormat, ArgTypes&&... Args)
    {
        const FString FormattedMessage = FString::CreateFormatted(InFormat, Forward<ArgTypes>(Args)...);
        OutputDebugString(FormattedMessage);
    }

    /**
     * @brief Checks weather or not the application is running inside a debugger
     * @return Returns true if the debugger is present, otherwise false
     */
    static FORCEINLINE bool IsDebuggerPresent()
    {
        return FPlatformMisc::IsDebuggerPresent();
    }
};
