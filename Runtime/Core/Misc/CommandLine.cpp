#include "Core/Misc/CommandLine.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Misc/Parse.h"
#include "Core/Templates/CString.h"

CHAR_T CommandLine::CommandLineBuffer[CommandLine::MaxCommandLineLength]   = { 0 };
CHAR_T CommandLine::OriginalCommandLine[CommandLine::MaxCommandLineLength] = { 0 };

static void AppendChecked(CHAR_T*& It, CHAR_T* End, const CHAR_T* Source, UPTR_INT Length)
{
    const UPTR_INT Available = static_cast<UPTR_INT>(End - It);
    const UPTR_INT ToCopy    = (Length < Available) ? Length : Available;
    CString::Strncpy(It, Source, ToCopy);
    It += ToCopy;
}

static const CHAR_T* FindOptionToken(const CHAR_T* Buffer, const CHAR_T* Name)
{
    const int32 NameLength = CString::Strlen(Name);
    if (NameLength <= 0)
    {
        return nullptr;
    }

    for (const CHAR_T* Match = CString::Stristr(Buffer, Name); Match; Match = CString::Stristr(Match + 1, Name))
    {
        const CHAR_T* Dash = Match - 1;
        if ((Match == Buffer) || (*Dash != '-'))
        {
            continue;
        }

        if ((Dash != Buffer) && (*(Dash - 1) != ' '))
        {
            continue;
        }

        const CHAR_T Terminator = *(Match + NameLength);
        if ((Terminator == '=') || (Terminator == ' ') || (Terminator == '\0'))
        {
            return Match;
        }
    }

    return nullptr;
}

bool CommandLine::Initialize(const CHAR_T** Args, int32 NumArgs)
{
    if (!Args)
    {
        return false;
    }

    CHAR_T* CommandLineIt          = CommandLineBuffer;
    CHAR_T* CommandLineEnd         = CommandLineBuffer + MaxCommandLineLength - 1;
    CHAR_T* OriginalCommandLineIt  = OriginalCommandLine;
    CHAR_T* OriginalCommandLineEnd = OriginalCommandLine + MaxCommandLineLength - 1;

    // Terminate up front so a repeated Initialize cannot leave a stale tail behind
    *CommandLineIt         = '\0';
    *OriginalCommandLineIt = '\0';

    for (int32 Index = 0; Index < NumArgs; ++Index)
    {
        const CHAR_T* CurrentArg = Args[Index];
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
            const CHAR_T* Option = CString::Strchr(CurrentArg, '-');
            if (!Option)
            {
                // No further options in this argument
                break;
            }

            const CHAR_T* Iterator = Option + 1;
            Parse::ParseOptionName(&Iterator);

            AppendChecked(CommandLineIt, CommandLineEnd, Option, static_cast<UPTR_INT>(Iterator - Option));

            // Where the scan resumes; the '=' branch moves this past the value
            const CHAR_T* NextArg = Iterator;

            Parse::ParseWhiteSpace(&Iterator);

            if (*Iterator == '=')
            {
                AppendChecked(CommandLineIt, CommandLineEnd, "=", 1);

                ++Iterator;
                Parse::ParseWhiteSpace(&Iterator);

                // Special case for string-values
                const CHAR_T* ValueEnd = nullptr;
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

bool CommandLine::FindOption(const CHAR_T* Value)
{
    return FindOptionToken(CommandLineBuffer, Value) != nullptr;
}

bool CommandLine::FindOption(const CHAR_T* Value, FStringView& OutValue)
{
    const CHAR_T* Result = FindOptionToken(CommandLineBuffer, Value);
    if (!Result)
    {
        return false;
    }

    Result += CString::Strlen(Value);

    // A bare switch: report it as found, with an empty but valid view
    if (*Result != '=')
    {
        OutValue = FStringView(Result, 0);
        return true;
    }

    ++Result;

    const CHAR_T* StringEnd = Result;
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

    OutValue = FStringView(Result, static_cast<int32>(StringEnd - Result));
    return true;
}
