#pragma once
#include "Core/CoreModule.h"
#include "Core/Containers/String.h"

#define MAX_COMMANDLINE_LENGTH (512)

class CORE_API CCommandLine
{
public:
	
	/* Initialize the commandline args and parse all the arguments */
    static void Initialize( int32 NumCommandLineArgs, const char** CommandLineArgs );

	/* Retrieve the commandline */
    static FORCEINLINE const char* Get()
	{
		return CommandLine;
	}

private:
    static char CommandLine[MAX_COMMANDLINE_LENGTH];
};
