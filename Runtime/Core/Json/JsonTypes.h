#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Templates/Utility/EnumOperators.h"
#include "Core/Templates/Utility/UnderlyingTypeValue.h"

enum class EJsonType : uint8
{
    Null   = 0,
    Bool   = 1,
    Number = 2,
    String = 3,
    Array  = 4,
    Object = 5,
};

enum class EJsonParseFlags : uint8
{
    None                = 0,
    AllowComments       = FLAG(1),                             // Accept '//' line comments and '/* */' block comments. Comments are skipped, never retained.
    AllowTrailingCommas = FLAG(2),                             // Accept a single trailing comma before a closing ']' or '}'
    Relaxed             = AllowComments | AllowTrailingCommas, // Everything a hand-edited file is likely to contain
};

ENUM_CLASS_OPERATORS(EJsonParseFlags);

enum class EJsonWriteFlags : uint8
{
    None               = 0,
    Indent             = FLAG(1),                     // Indent nested values and put each member on its own line
    InlineScalarArrays = FLAG(2),                     // Keep arrays holding only numbers, bools and nulls on a single line
    Compact            = None,                        // Everything on one line, no spaces
    Pretty             = Indent | InlineScalarArrays, // Readable, diff-friendly output
};

ENUM_CLASS_OPERATORS(EJsonWriteFlags);

struct FJsonError
{
    FJsonError()
        : Line(0)
        , Column(0)
        , Offset(0)
        , Message()
    {
    }

    // Formats as "line 3, column 12: expected ':'"
    NODISCARD CORE_API String ToString() const;

    int32  Line;
    int32  Column;
    int32  Offset;
    String Message;
};

// Nesting past this many arrays/objects is rejected rather than risking the stack
#ifndef JSON_DEFAULT_MAX_DEPTH
    #define JSON_DEFAULT_MAX_DEPTH (128)
#endif

// Number of spaces per indent level when EJsonWriteFlags::Indent is set
#ifndef JSON_INDENT_WIDTH
    #define JSON_INDENT_WIDTH (4)
#endif

// Longest scalar-only array that EJsonWriteFlags::InlineScalarArrays keeps on one line
#ifndef JSON_MAX_INLINE_ARRAY_ELEMENTS
    #define JSON_MAX_INLINE_ARRAY_ELEMENTS (16)
#endif
