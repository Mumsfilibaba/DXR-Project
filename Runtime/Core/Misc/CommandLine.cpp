#include "Core/Misc/CommandLine.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Misc/Parse.h"
#include "Core/Templates/CString.h"

CHAR_T CommandLine::Options[CommandLine::MaxCommandLineLength]             = { 0 };
CHAR_T CommandLine::OriginalCommandLine[CommandLine::MaxCommandLineLength] = { 0 };

static bool AppendChecked(CHAR_T*& It, CHAR_T* End, const CHAR_T* Source, UPTR_INT Length)
{
    const UPTR_INT Available = static_cast<UPTR_INT>(End - It);
    const UPTR_INT ToCopy    = (Length < Available) ? Length : Available;

    CString::Strncpy(It, Source, ToCopy);
    It += ToCopy;
    return ToCopy == Length;
}

static const CHAR_T* FindOptionNameEnd(const CHAR_T* Options, const CHAR_T* Name)
{
    const int32 NameLength = CString::Strlen(Name);
    if (NameLength <= 0)
    {
        return nullptr;
    }

    for (const CHAR_T* Option = Options; *Option != '\0'; Option += CString::Strlen(Option) + 1)
    {
        const CHAR_T* NameEnd = Option + NameLength;
        if (CString::Strnicmp(Option, Name, static_cast<SIZE_T>(NameLength)) == 0 && (*NameEnd == '=' || *NameEnd == '\0'))
        {
            return NameEnd;
        }
    }

    return nullptr;
}

bool CommandLine::Initialize(const CHAR_T** Args, int32 NumArgs)
{
    // Emptied up front so that a repeated or failed Initialize cannot leave stale options behind
    Options[0]             = '\0';
    OriginalCommandLine[0] = '\0';

    if (!Args)
    {
        return false;
    }

    // The last character of the options is kept for the empty option that ends them
    CHAR_T* OptionsIt   = Options;
    CHAR_T* OptionsEnd  = Options + MaxCommandLineLength - 1;
    CHAR_T* OriginalIt  = OriginalCommandLine;
    CHAR_T* OriginalEnd = OriginalCommandLine + MaxCommandLineLength - 1;

    bool bIsTruncated = false;
    bool bIsValid     = true;

    for (int32 Index = 0; Index < NumArgs; ++Index)
    {
        const CHAR_T* Argument = Args[Index];
        if (!Argument)
        {
            bIsValid = false;
            break;
        }

        const UPTR_INT Length = static_cast<UPTR_INT>(CString::Strlen(Argument));
        if (Index > 0)
        {
            bIsTruncated |= !AppendChecked(OriginalIt, OriginalEnd, TEXT(" "), 1);
        }

        bIsTruncated |= !AppendChecked(OriginalIt, OriginalEnd, Argument, Length);

        if (Argument[0] != '-')
        {
            continue;
        }

        const CHAR_T* NameEnd = Argument + 1;
        Parse::ParseOptionName(&NameEnd);

        if (NameEnd == Argument + 1 || (*NameEnd != '=' && *NameEnd != '\0'))
        {
            continue;
        }

        // An option that does not fit is dropped whole, since a value cut short would be worse than none
        const UPTR_INT OptionLength = Length - 1;
        if (OptionLength + 1 > static_cast<UPTR_INT>(OptionsEnd - OptionsIt))
        {
            bIsTruncated = true;
            continue;
        }

        CString::Strncpy(OptionsIt, Argument + 1, OptionLength);
        OptionsIt[OptionLength] = '\0';
        OptionsIt += OptionLength + 1;
        *OptionsIt = '\0';
    }

    *OriginalIt = '\0';

    if (bIsTruncated)
    {
        LOG_WARNING("CommandLine is longer than %d characters, and what did not fit was dropped", static_cast<int32>(MaxCommandLineLength));
    }

    return bIsValid;
}

bool CommandLine::FindOption(const CHAR_T* Name)
{
    return FindOptionNameEnd(Options, Name) != nullptr;
}

bool CommandLine::FindOption(const CHAR_T* Name, FStringView& OutValue)
{
    const CHAR_T* NameEnd = FindOptionNameEnd(Options, Name);
    if (!NameEnd)
    {
        return false;
    }

    // A bare switch is reported as found, with an empty but valid view
    OutValue = (*NameEnd == '=') ? FStringView(NameEnd + 1) : FStringView(NameEnd, 0);
    return true;
}
