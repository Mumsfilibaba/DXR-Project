#include "CommandLine.h"

#include "Core/Containers/String.h"
#include "Core/Containers/StringView.h"

char CCommandLine::CommandLine[MAX_COMMANDLINE_LENGTH];

void CCommandLine::Initialize( int32 NumCommandLineArgs, const char** CommandLineArgs )
{
    // Zero commandline
    CMemory::Memzero( CommandLine, sizeof(CommandLine) );

    // Skip the executeable path
	CString Result;
    for ( int32 Index = 1; Index < NumCommandLineArgs; Index++ )
    {
        CStringView CurrentArg = CommandLineArgs[Index];
		CurrentArg.TrimInline();
		
		for ( int32 Character = 0; Character < CurrentArg.Size(); )
		{
			if ( CurrentArg[Character] == '-' )
			{
				CurrentArg.ShrinkLeftInline();
			}
			else
			{
				Character++;
			}
		}
		
		Result += '-';
		Result += CurrentArg;

		// Add space between arguments
        if ( Index + 1 < NumCommandLineArgs )
        {
            Result += ' ';
        }
    }

    CStringUtils::Copy( CommandLine, Result.CStr(), Result.Length() );
}

void CCommandLine::GetValue( const CString& Name, CString& Value )
{
}
