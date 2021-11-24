#include "CommandLine.h"

#include "Core/Containers/String.h"
#include "Core/Containers/StringView.h"

char CCommandLine::CommandLine[MAX_COMMANDLINE_LENGTH];

///////////////////////////////////////////////////////////////////////////////////////////////////

void CCommandLine::Initialize( int32 NumCommandLineArgs, const char** CommandLineArgs )
{
    // Zero commandline
    CMemory::Memzero( CommandLine, sizeof(CommandLine) );

    CString Result;
    // Skip the executeable path
    for ( int32 Index = 1; Index < NumCommandLineArgs; Index++ )
    {
        CStringView CurrentArg = CommandLineArgs[Index];
        for ( int32 Character = 0; Character < CurrentArg.Length(); Character++ )
        {
        }

        if ( Index + 1 < NumCommandLineArgs )
        {
            //Result;
        }
    }

    CStringTraits::Copy( CommandLine, Result.CStr(), Result.Length() );
}
