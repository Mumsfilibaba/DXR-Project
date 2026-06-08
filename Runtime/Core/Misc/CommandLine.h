#pragma once 
#include "Core/Core.h"
#include "Core/Containers/StringView.h"

class CORE_API CommandLine
{
public:
    static constexpr uint64 MaxCommandLineLength = 1024;

    static bool Initialize(const CHAR** Args, int32 NumArgs);

    static bool FindOption(const CHAR* Value);
    static bool FindOption(const CHAR* Value, FStringView& OutValue);

    static FORCEINLINE const CHAR* Get()
    {
        return CommandLineBuffer;
    }

    static FORCEINLINE const CHAR* GetOriginal()
    {
        return OriginalCommandLine;
    }

private:
    static CHAR CommandLineBuffer[MaxCommandLineLength];
    static CHAR OriginalCommandLine[MaxCommandLineLength];
};