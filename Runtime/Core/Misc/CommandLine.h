#pragma once
#include "Core/CoreModule.h"
#include "Core/Containers/String.h"

class CORE_API CCommandLine
{
public:
    static bool Parse( const char* RawCommandLine );

    static bool GetInt( const char* Parameter, int& Value );
    static bool GetBool( const char* parameter );

    static const CString& Get();

private:
    static CString CommandLine;
};
