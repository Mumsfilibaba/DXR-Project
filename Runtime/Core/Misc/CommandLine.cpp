#include "Core/Misc/CommandLine.h"
#include "Core/Misc/Parse.h"
#include "Core/Templates/CString.h"

CHAR CommandLine::CommandLineBuffer[CommandLine::MaxCommandLineLength]   = { 0 };
CHAR CommandLine::OriginalCommandLine[CommandLine::MaxCommandLineLength] = { 0 };

bool CommandLine::Initialize(const CHAR** Args, int32 NumArgs)
{
    if (!Args)
    {
        return false;
    }

    CHAR* CommandLineIt         = CommandLineBuffer;
    CHAR* CommandLineEnd        = CommandLineBuffer + MaxCommandLineLength;
    CHAR* OriginalCommandLineIt = OriginalCommandLine;

    for (int32 Index = 0; Index < NumArgs; ++Index)
    {
        const CHAR* CurrentArg = Args[Index];
        if (!CurrentArg)
        {
            return false;
        }

        {
            const int32 Length = FCString::Strlen(CurrentArg);
            FCString::Strncpy(OriginalCommandLineIt, CurrentArg, Length);
            OriginalCommandLineIt += Length;
        }

        while (CurrentArg && *CurrentArg && (CommandLineIt < CommandLineEnd))
        {
            if (const CHAR* Option = FCString::Strchr(CurrentArg, '-'))
            {
                // Find the end of the value
                const CHAR* Iterator = Option + 1;
                Parse::ParseAlnum(&Iterator);

                {
                    const UPTR_INT Length = static_cast<UPTR_INT>(Iterator - Option);
                    FCString::Strncpy(CommandLineIt, Option, Length);
                    CommandLineIt += Length;
                }

                Parse::ParseWhiteSpace(&Iterator);

                if (*Iterator == '=')
                {
                    *(CommandLineIt++) = '=';

                    ++Iterator;
                    Parse::ParseWhiteSpace(&Iterator);

                    // Special case for string-values
                    const CHAR* ValueEnd = nullptr;
                    if (*Iterator == '\"')
                    {
                        ValueEnd = FCString::Strchr(Iterator + 1, '\"');
                        if (ValueEnd)
                            ++ValueEnd;
                    }
                    else
                    {
                        ValueEnd = FCString::Strchr(Iterator, ' ');
                    }

                    if (!ValueEnd)
                    {
                        ValueEnd = Iterator;
                        Parse::ParseAlnum(&ValueEnd);
                    }

                    {
                        const UPTR_INT Length = static_cast<UPTR_INT>(ValueEnd - Iterator);
                        FCString::Strncpy(CommandLineIt, Iterator, Length);
                        CommandLineIt += Length;
                    }
                }

                *(CommandLineIt++) = ' ';
                CurrentArg = Iterator;
            }
            else
            {
                // Invalid arg
                break;
            }
        }
    }

    return true;
}

bool CommandLine::FindOption(const CHAR* Value)
{
    // TODO: Have a way to do this non-case sensitive
    const CHAR* Result = FCString::Strstr(CommandLineBuffer, Value);
    return (Result != nullptr);
}

bool CommandLine::FindOption(const CHAR* Value, FStringView& OutValue)
{
    // TODO: Have a way to do this non-case sensitive
    if (const CHAR* Result = FCString::Strstr(CommandLineBuffer, Value))
    {
        Parse::ParseAlnum(&Result);
        if (*Result == '=')
        {
            ++Result;

            const CHAR* StringEnd = Result++;
            if (*StringEnd == '\"')
            {
                StringEnd = FCString::Strchr(Result, '\"');
                CHECK(StringEnd != nullptr);
            }
            else
            {
                Parse::ParseAlnum(&StringEnd);
            }
            
            const int32 Length = static_cast<int32>(StringEnd - Result);
            OutValue = FStringView(Result, Length);
        }

        return true;
    }

    return false;
}
