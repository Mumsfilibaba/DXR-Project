#pragma once 
#include "Core/Core.h"
#include "Core/Containers/StringView.h"

class CORE_API CommandLine
{
public:
    static constexpr uint64 MaxCommandLineLength = 1024;

    static bool Initialize(const CHAR_T** Args, int32 NumArgs);

    static bool FindOption(const CHAR_T* Value);
    static bool FindOption(const CHAR_T* Value, StringView& OutValue);

    static FORCEINLINE const CHAR_T* Get()
    {
        return CommandLineBuffer;
    }

    static FORCEINLINE const CHAR_T* GetOriginal()
    {
        return OriginalCommandLine;
    }

private:
    static CHAR_T CommandLineBuffer[MaxCommandLineLength];
    static CHAR_T OriginalCommandLine[MaxCommandLineLength];
};