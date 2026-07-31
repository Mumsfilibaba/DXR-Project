#include "Core/Misc/CommandLine.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/Parse.h"
#include "Core/Templates/CString.h"

CHAR CommandLine::CommandLineBuffer[CommandLine::MaxCommandLineLength]   = { 0 };
CHAR CommandLine::OriginalCommandLine[CommandLine::MaxCommandLineLength] = { 0 };

static void AppendChecked(CHAR*& It, CHAR* End, const CHAR* Source, UPTR_INT Length)
{
    const UPTR_INT Available = static_cast<UPTR_INT>(End - It);
    const UPTR_INT ToCopy    = (Length < Available) ? Length : Available;
    CString::Strncpy(It, Source, ToCopy);
    It += ToCopy;
}

static const CHAR* FindOptionToken(const CHAR* Buffer, const CHAR* Name)
{
    const int32 NameLength = CString::Strlen(Name);
    if (NameLength <= 0)
    {
        return nullptr;
    }

    for (const CHAR* Match = CString::Stristr(Buffer, Name); Match; Match = CString::Stristr(Match + 1, Name))
    {
        const CHAR* Dash = Match - 1;
        if ((Match == Buffer) || (*Dash != '-'))
        {
            continue;
        }

        if ((Dash != Buffer) && (*(Dash - 1) != ' '))
        {
            continue;
        }

        const CHAR Terminator = *(Match + NameLength);
        if ((Terminator == '=') || (Terminator == ' ') || (Terminator == '\0'))
        {
            return Match;
        }
    }

    return nullptr;
}

bool CommandLine::Initialize(const CHAR** Args, int32 NumArgs)
{
    if (!Args)
    {
        return false;
    }

    CHAR* CommandLineIt          = CommandLineBuffer;
    CHAR* CommandLineEnd         = CommandLineBuffer + MaxCommandLineLength - 1;
    CHAR* OriginalCommandLineIt  = OriginalCommandLine;
    CHAR* OriginalCommandLineEnd = OriginalCommandLine + MaxCommandLineLength - 1;

    // Terminate up front so a repeated Initialize cannot leave a stale tail behind
    *CommandLineIt         = '\0';
    *OriginalCommandLineIt = '\0';

    for (int32 Index = 0; Index < NumArgs; ++Index)
    {
        const CHAR* CurrentArg = Args[Index];
        if (!CurrentArg)
        {
            return false;
        }

        if (Index > 0)
        {
            AppendChecked(OriginalCommandLineIt, OriginalCommandLineEnd, " ", 1);
        }

        AppendChecked(OriginalCommandLineIt, OriginalCommandLineEnd, CurrentArg, CString::Strlen(CurrentArg));

        while (CurrentArg && *CurrentArg && (CommandLineIt < CommandLineEnd))
        {
            const CHAR* Option = CString::Strchr(CurrentArg, '-');
            if (!Option)
            {
                // No further options in this argument
                break;
            }

            const CHAR* Iterator = Option + 1;
            Parse::ParseOptionName(&Iterator);

            AppendChecked(CommandLineIt, CommandLineEnd, Option, static_cast<UPTR_INT>(Iterator - Option));

            // Where the scan resumes; the '=' branch moves this past the value
            const CHAR* NextArg = Iterator;

            Parse::ParseWhiteSpace(&Iterator);

            if (*Iterator == '=')
            {
                AppendChecked(CommandLineIt, CommandLineEnd, "=", 1);

                ++Iterator;
                Parse::ParseWhiteSpace(&Iterator);

                // Special case for string-values
                const CHAR* ValueEnd = nullptr;
                if (*Iterator == '\"')
                {
                    ValueEnd = CString::Strchr(Iterator + 1, '\"');
                    if (ValueEnd)
                    {
                        ++ValueEnd;
                    }
                }
                else
                {
                    ValueEnd = CString::Strchr(Iterator, ' ');
                }

                // Unquoted and last on the line: the value runs to the end of the argument
                if (!ValueEnd)
                {
                    ValueEnd = Iterator;
                    Parse::ParseValue(&ValueEnd);
                }

                AppendChecked(CommandLineIt, CommandLineEnd, Iterator, static_cast<UPTR_INT>(ValueEnd - Iterator));
                NextArg = ValueEnd;
            }

            AppendChecked(CommandLineIt, CommandLineEnd, " ", 1);

            // Resume after the value, so a '-' inside it cannot start a phantom option
            CurrentArg = NextArg;
        }
    }

    *CommandLineIt         = '\0';
    *OriginalCommandLineIt = '\0';

    if ((CommandLineIt == CommandLineEnd) || (OriginalCommandLineIt == OriginalCommandLineEnd))
    {
        LOG_WARNING("CommandLine was truncated at %d characters", static_cast<int32>(MaxCommandLineLength));
    }

    return true;
}

bool CommandLine::FindOption(const CHAR* Value)
{
    return FindOptionToken(CommandLineBuffer, Value) != nullptr;
}

bool CommandLine::FindOption(const CHAR* Value, StringView& OutValue)
{
    const CHAR* Result = FindOptionToken(CommandLineBuffer, Value);
    if (!Result)
    {
        return false;
    }

    Result += CString::Strlen(Value);

    // A bare switch: report it as found, with an empty but valid view
    if (*Result != '=')
    {
        OutValue = StringView(Result, 0);
        return true;
    }

    ++Result;

    const CHAR* StringEnd = Result;
    if (*Result == '\"')
    {
        ++Result;

        StringEnd = CString::Strchr(Result, '\"');
        if (!StringEnd)
        {
            // Unterminated quote: take the rest of the token rather than asserting
            StringEnd = Result;
            Parse::ParseValue(&StringEnd);
        }
    }
    else
    {
        Parse::ParseValue(&StringEnd);
    }

    OutValue = StringView(Result, static_cast<int32>(StringEnd - Result));
    return true;
}
