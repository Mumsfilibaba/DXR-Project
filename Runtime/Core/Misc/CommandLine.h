#pragma once
#include "Core/Core.h"
#include "Core/Containers/StringView.h"

class CORE_API CommandLine
{
public:
    static constexpr uint64 MaxCommandLineLength = 1024;

    /**
     * @brief Takes the arguments the program was started with, as the operating system split them. Each argument that
     * starts with '-' is one option: its name runs to an '=', and its value from there to the end of the argument, spaces
     * included, so a value needs no quotes of its own. Any other argument is kept in the original command line only.
     *
     * @param Args    The arguments, without the program's path.
     * @param NumArgs The number of arguments.
     * @return False when Args or one of the arguments is null.
     */
    static bool Initialize(const CHAR_T** Args, int32 NumArgs);

    /** @return True when the option was given, with or without a value. Names are matched whole and without regard to case. */
    static bool FindOption(const CHAR_T* Name);

    /**
     * @param Name     The option's name, without the leading '-'.
     * @param OutValue Set to the option's value, which is empty for an option given without one, and left untouched when the
     *                 option was not given. It stays valid until the next Initialize.
     * @return True when the option was given.
     */
    static bool FindOption(const CHAR_T* Name, FStringView& OutValue);

    /** @return The arguments joined by spaces, as they were given, for showing in a log or a report. */
    static FORCEINLINE const CHAR_T* GetOriginal()
    {
        return OriginalCommandLine;
    }

private:
    static CHAR_T Options[MaxCommandLineLength];
    static CHAR_T OriginalCommandLine[MaxCommandLineLength];
};
