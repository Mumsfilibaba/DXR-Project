#include "CommandLine.h"
#include "Parse.h"

#include "Core/Templates/CString.h"

CHAR FCommandLine::CommandLine[FCommandLine::MaxCommandLineLength]         = { 0 };
CHAR FCommandLine::OriginalCommandLine[FCommandLine::MaxCommandLineLength] = { 0 };

bool FCommandLine::Initialize(const CHAR** Args, int32 NumArgs)
{
    if (!Args)
    {
        return false;
    }

    CHAR* CommandLineIt         = CommandLine;
    CHAR* CommandLineEnd        = CommandLine + MaxCommandLineLength;
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
                FParse::ParseAlnum(&Iterator);

                {
                    const uintptr Length = static_cast<uintptr>(Iterator - Option);
                    FCString::Strncpy(CommandLineIt, Option, Length);
                    CommandLineIt += Length;
                }

                FParse::ParseWhiteSpace(&Iterator);

                if (*Iterator == '=')
                {
                    *(CommandLineIt++) = '=';

                    ++Iterator;
                    FParse::ParseWhiteSpace(&Iterator);

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
                        FParse::ParseAlnum(&ValueEnd);
                    }

                    {
                        const uintptr Length = static_cast<uintptr>(ValueEnd - Iterator);
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

bool FCommandLine::Parse(const CHAR* Value)
{
    // TODO: Have a way to do this non-case sensitive
    const CHAR* Result = FCString::Strstr(CommandLine, Value);
    return (Result != nullptr);
}

bool FCommandLine::Parse(const CHAR* Value, FStringView& OutValue)
{
    // TODO: Have a way to do this non-case sensitive
    if (const CHAR* Result = FCString::Strstr(CommandLine, Value))
    {
        FParse::ParseAlnum(&Result);
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
                FParse::ParseAlnum(&StringEnd);
            }
            
            const int32 Length = static_cast<int32>(StringEnd - Result);
            OutValue = FStringView(Result, Length);
        }

        return true;
    }

    return false;
}
