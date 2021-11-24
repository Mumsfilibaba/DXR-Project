#pragma once
#include "Core/CoreModule.h"
#include "Core/Containers/String.h"

#define MAX_COMMANDLINE_LENGTH (512)

class CORE_API CCommandLine
{
public:
	
	/* Initialize the commandline args and parse all the arguments */
    static void Initialize( int32 NumCommandLineArgs, const char** CommandLineArgs );

	/* Retrives an value of the commandline arg if the argument has a value */
	static void GetValue( const CString& Name, CString& Value );
	
	/* Retrieve the commandline */
    static FORCEINLINE const char* Get()
	{
		return CommandLine;
	}

private:
	
	// The full commandline
    static char CommandLine[MAX_COMMANDLINE_LENGTH];
};
