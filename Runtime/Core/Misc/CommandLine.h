#pragma once 
#include "Core/Core.h"
#include "Core/Containers/StringView.h"

class CORE_API FCommandLine
{
public:
    enum 
    { 
        MaxCommandLineLength = 1024 
    };

    static bool Initialize(const CHAR** Args, int32 NumArgs);

    static bool Parse(const CHAR* Value);
    static bool Parse(const CHAR* Value, FStringView& OutValue);

    static FORCEINLINE const CHAR* Get()         { return CommandLine; } 
    static FORCEINLINE const CHAR* GetOriginal() { return OriginalCommandLine; }

private:
    static CHAR CommandLine[MaxCommandLineLength];
    static CHAR OriginalCommandLine[MaxCommandLineLength];
};