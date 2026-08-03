#include "Core/Json/JsonReader.h"
#include "Core/Templates/CString.h"

#include <cstdarg>
#include <cstdio>

static constexpr int32 GMaxErrorMessageLength = 256;

// No JSON number needs more room than this, and anything longer is malformed anyway
static constexpr int32  GMaxNumberLength    = 64;
static constexpr uint32 GHighSurrogateFirst = 0xD800;
static constexpr uint32 GHighSurrogateLast  = 0xDBFF;
static constexpr uint32 GLowSurrogateFirst  = 0xDC00;
static constexpr uint32 GLowSurrogateLast   = 0xDFFF;

static FORCEINLINE bool IsDigit(CHAR Character)
{
    return (Character >= '0') && (Character <= '9');
}

static FORCEINLINE bool IsHexDigit(CHAR Character)
{
    return IsDigit(Character) || ((Character >= 'a') && (Character <= 'f')) || ((Character >= 'A') && (Character <= 'F'));
}

static FORCEINLINE uint32 HexDigitValue(CHAR Character)
{
    if (IsDigit(Character))
    {
        return static_cast<uint32>(Character - '0');
    }

    if ((Character >= 'a') && (Character <= 'f'))
    {
        return static_cast<uint32>(Character - 'a') + 10;
    }

    return static_cast<uint32>(Character - 'A') + 10;
}

FJsonReader::FJsonReader(EJsonParseFlags InFlags, int32 InMaxDepth)
    : Start(nullptr)
    , Current(nullptr)
    , End(nullptr)
    , Line(1)
    , LineStartOffset(0)
    , Flags(InFlags)
    , MaxDepth(InMaxDepth)
    , Error()
    , bHasError(false)
{
}

bool FJsonReader::IsAtEnd() const
{
    return Current >= End;
}

CHAR FJsonReader::Peek() const
{
    return IsAtEnd() ? '\0' : *Current;
}

CHAR FJsonReader::PeekAt(int32 Lookahead) const
{
    const CHAR* Position = Current + Lookahead;
    return (Position >= End) ? '\0' : *Position;
}

CHAR FJsonReader::Advance()
{
    if (IsAtEnd())
    {
        return '\0';
    }

    const CHAR Character = *Current++;
    if (Character == '\n')
    {
        ++Line;
        LineStartOffset = static_cast<int32>(Current - Start);
    }

    return Character;
}

bool FJsonReader::Fail(const CHAR* Format, ...)
{
    // Only the first failure is worth reporting. Everything after it is a consequence of 
    // the parser being lost, and would just bury the position that actually matters.
    if (bHasError)
    {
        return false;
    }

    CHAR Message[GMaxErrorMessageLength];
    Message[0] = 0;

    va_list ArgList;
    va_start(ArgList, Format);
    vsnprintf(Message, sizeof(Message), Format, ArgList);
    va_end(ArgList);

    const int32 Offset = static_cast<int32>(Current - Start);

    Error.Line    = Line;
    Error.Column  = (Offset - LineStartOffset) + 1;
    Error.Offset  = Offset;
    Error.Message = Message;
    
    bHasError     = true;
    return false;
}

bool FJsonReader::Expect(CHAR Expected)
{
    if (Peek() != Expected)
    {
        if (IsAtEnd())
        {
            return Fail("expected '%c' but reached the end of the document", Expected);
        }

        return Fail("expected '%c' but found '%c'", Expected, Peek());
    }

    Advance();
    return true;
}

bool FJsonReader::SkipComment()
{
    if ((Peek() != '/') || !IsEnumFlagSet(Flags, EJsonParseFlags::AllowComments))
    {
        return false;
    }

    const CHAR Kind = PeekAt(1);
    if (Kind == '/')
    {
        while (!IsAtEnd() && (Peek() != '\n'))
        {
            Advance();
        }

        return true;
    }

    if (Kind == '*')
    {
        Advance();
        Advance();

        while (!IsAtEnd())
        {
            if ((Peek() == '*') && (PeekAt(1) == '/'))
            {
                Advance();
                Advance();
                return true;
            }

            Advance();
        }

        Fail("unterminated block comment");
        return true;
    }

    return false;
}

void FJsonReader::SkipWhitespaceAndComments()
{
    while (!IsAtEnd())
    {
        const CHAR Character = Peek();
        if ((Character == ' ') || (Character == '\t') || (Character == '\n') || (Character == '\r'))
        {
            Advance();
            continue;
        }

        if (SkipComment())
        {
            if (bHasError)
            {
                return;
            }

            continue;
        }

        return;
    }
}

bool FJsonReader::Parse(const StringView& Text, FJsonValue& OutValue, FJsonError* OutError)
{
    Start   = Text.Data();
    Current = Start;
    End     = Start + Text.Length();

    Line            = 1;
    LineStartOffset = 0;
    Error           = FJsonError();
    bHasError       = false;

    // A BOM is not part of the document, but editors add one and it must not reach the parser
    if ((End - Current) >= 3)
    {
        const uint8 First  = static_cast<uint8>(Current[0]);
        const uint8 Second = static_cast<uint8>(Current[1]);
        const uint8 Third  = static_cast<uint8>(Current[2]);
        if ((First == 0xEF) && (Second == 0xBB) && (Third == 0xBF))
        {
            Current += 3;
            LineStartOffset = 3;
        }
    }

    FJsonValue Parsed;

    SkipWhitespaceAndComments();
    if (!bHasError)
    {
        if (IsAtEnd())
        {
            Fail("the document is empty");
        }
        else if (ParseValue(Parsed, 0))
        {
            SkipWhitespaceAndComments();
            if (!bHasError && !IsAtEnd())
            {
                Fail("unexpected '%c' after the document value", Peek());
            }
        }
    }

    if (bHasError)
    {
        if (OutError)
        {
            *OutError = Error;
        }

        return false;
    }

    OutValue = Move(Parsed);
    return true;
}

bool FJsonReader::ParseKeyword(const CHAR* Keyword, FJsonValue&& Result, FJsonValue& OutValue)
{
    const int32 KeywordLength = CString::Strlen(Keyword);
    if (((End - Current) < KeywordLength) || (CString::Strncmp(Current, Keyword, KeywordLength) != 0))
    {
        return Fail("expected '%s'", Keyword);
    }

    for (int32 Index = 0; Index < KeywordLength; ++Index)
    {
        Advance();
    }

    OutValue = Move(Result);
    return true;
}

bool FJsonReader::ParseValue(FJsonValue& OutValue, int32 Depth)
{
    if (Depth > MaxDepth)
    {
        return Fail("nesting is deeper than the limit of %d", MaxDepth);
    }

    SkipWhitespaceAndComments();
    if (bHasError)
    {
        return false;
    }

    if (IsAtEnd())
    {
        return Fail("expected a value but reached the end of the document");
    }

    const CHAR Character = Peek();
    switch (Character)
    {
        case '{':
        {
            return ParseObject(OutValue, Depth);
        }

        case '[':
        {
            return ParseArray(OutValue, Depth);
        }

        case '"':
        {
            String Parsed;
            if (!ParseString(Parsed))
            {
                return false;
            }

            OutValue = FJsonValue(Parsed);
            return true;
        }

        case 't':
        {
            return ParseKeyword("true", FJsonValue(true), OutValue);
        }

        case 'f':
        {
            return ParseKeyword("false", FJsonValue(false), OutValue);
        }

        case 'n':
        {
            return ParseKeyword("null", FJsonValue(), OutValue);
        }

        default:
        {
            if ((Character == '-') || IsDigit(Character))
            {
                return ParseNumber(OutValue);
            }

            return Fail("unexpected '%c' where a value was expected", Character);
        }
    }
}

bool FJsonReader::ParseObject(FJsonValue& OutValue, int32 Depth)
{
    if (!Expect('{'))
    {
        return false;
    }

    FJsonValue Object = FJsonValue::MakeObject();

    SkipWhitespaceAndComments();
    if (bHasError)
    {
        return false;
    }

    if (Peek() == '}')
    {
        Advance();
        OutValue = Move(Object);
        return true;
    }

    while (true)
    {
        SkipWhitespaceAndComments();
        if (bHasError)
        {
            return false;
        }

        if (Peek() != '"')
        {
            if (IsAtEnd())
            {
                return Fail("expected a member name but reached the end of the document");
            }

            return Fail("expected a member name in quotes but found '%c'", Peek());
        }

        String Name;
        if (!ParseString(Name))
        {
            return false;
        }

        SkipWhitespaceAndComments();
        if (bHasError || !Expect(':'))
        {
            return false;
        }

        FJsonValue MemberValue;
        if (!ParseValue(MemberValue, Depth + 1))
        {
            return false;
        }

        // A duplicate name is not an error in RFC 8259 and the last one is what every reader uses
        Object.AddMember(Name.Data(), Move(MemberValue));

        SkipWhitespaceAndComments();
        if (bHasError)
        {
            return false;
        }

        if (Peek() == ',')
        {
            Advance();
            SkipWhitespaceAndComments();
            if (bHasError)
            {
                return false;
            }

            if (Peek() == '}')
            {
                if (!IsEnumFlagSet(Flags, EJsonParseFlags::AllowTrailingCommas))
                {
                    return Fail("trailing comma before '}'");
                }

                Advance();
                break;
            }

            continue;
        }

        if (Peek() == '}')
        {
            Advance();
            break;
        }

        if (IsAtEnd())
        {
            return Fail("expected ',' or '}' but reached the end of the document");
        }

        return Fail("expected ',' or '}' but found '%c'", Peek());
    }

    OutValue = Move(Object);
    return true;
}

bool FJsonReader::ParseArray(FJsonValue& OutValue, int32 Depth)
{
    if (!Expect('['))
    {
        return false;
    }

    FJsonValue Array = FJsonValue::MakeArray();

    SkipWhitespaceAndComments();
    if (bHasError)
    {
        return false;
    }

    if (Peek() == ']')
    {
        Advance();
        OutValue = Move(Array);
        return true;
    }

    while (true)
    {
        FJsonValue Element;
        if (!ParseValue(Element, Depth + 1))
        {
            return false;
        }

        Array.Add(Move(Element));

        SkipWhitespaceAndComments();
        if (bHasError)
        {
            return false;
        }

        if (Peek() == ',')
        {
            Advance();
            SkipWhitespaceAndComments();
            if (bHasError)
            {
                return false;
            }

            if (Peek() == ']')
            {
                if (!IsEnumFlagSet(Flags, EJsonParseFlags::AllowTrailingCommas))
                {
                    return Fail("trailing comma before ']'");
                }

                Advance();
                break;
            }

            continue;
        }

        if (Peek() == ']')
        {
            Advance();
            break;
        }

        if (IsAtEnd())
        {
            return Fail("expected ',' or ']' but reached the end of the document");
        }

        return Fail("expected ',' or ']' but found '%c'", Peek());
    }

    OutValue = Move(Array);
    return true;
}

bool FJsonReader::ParseHexQuad(uint32& OutCodePoint)
{
    uint32 CodePoint = 0;
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const CHAR Character = Peek();
        if (!IsHexDigit(Character))
        {
            if (IsAtEnd())
            {
                return Fail("incomplete '\\u' escape at the end of the document");
            }

            return Fail("'\\u' needs four hex digits but found '%c'", Character);
        }

        CodePoint = (CodePoint << 4) | HexDigitValue(Character);
        Advance();
    }

    OutCodePoint = CodePoint;
    return true;
}

void FJsonReader::AppendUtf8(uint32 CodePoint, String& OutString)
{
    if (CodePoint <= 0x7F)
    {
        OutString.Append(static_cast<CHAR>(CodePoint));
    }
    else if (CodePoint <= 0x7FF)
    {
        OutString.Append(static_cast<CHAR>(0xC0 | (CodePoint >> 6)));
        OutString.Append(static_cast<CHAR>(0x80 | (CodePoint & 0x3F)));
    }
    else if (CodePoint <= 0xFFFF)
    {
        OutString.Append(static_cast<CHAR>(0xE0 | (CodePoint >> 12)));
        OutString.Append(static_cast<CHAR>(0x80 | ((CodePoint >> 6) & 0x3F)));
        OutString.Append(static_cast<CHAR>(0x80 | (CodePoint & 0x3F)));
    }
    else
    {
        OutString.Append(static_cast<CHAR>(0xF0 | (CodePoint >> 18)));
        OutString.Append(static_cast<CHAR>(0x80 | ((CodePoint >> 12) & 0x3F)));
        OutString.Append(static_cast<CHAR>(0x80 | ((CodePoint >> 6) & 0x3F)));
        OutString.Append(static_cast<CHAR>(0x80 | (CodePoint & 0x3F)));
    }
}

bool FJsonReader::ParseEscape(String& OutString)
{
    const CHAR Escaped = Advance();
    switch (Escaped)
    {
        case '"':  OutString.Append('\"'); return true;
        case '\\': OutString.Append('\\'); return true;
        case '/':  OutString.Append('/');  return true;
        case 'b':  OutString.Append('\b'); return true;
        case 'f':  OutString.Append('\f'); return true;
        case 'n':  OutString.Append('\n'); return true;
        case 'r':  OutString.Append('\r'); return true;
        case 't':  OutString.Append('\t'); return true;
        case 'u':  break;
        default:
        {
            if (Escaped == '\0')
            {
                return Fail("incomplete escape at the end of the document");
            }

            return Fail("unknown escape '\\%c'", Escaped);
        }
    }

    uint32 CodePoint = 0;
    if (!ParseHexQuad(CodePoint))
    {
        return false;
    }

    if ((CodePoint >= GLowSurrogateFirst) && (CodePoint <= GLowSurrogateLast))
    {
        return Fail("'\\u%04X' is a trailing surrogate with no leading surrogate before it", CodePoint);
    }

    if ((CodePoint >= GHighSurrogateFirst) && (CodePoint <= GHighSurrogateLast))
    {
        if ((Peek() != '\\') || (PeekAt(1) != 'u'))
        {
            return Fail("'\\u%04X' is a leading surrogate and must be followed by a '\\u' escape", CodePoint);
        }

        Advance();
        Advance();

        uint32 LowSurrogate = 0;
        if (!ParseHexQuad(LowSurrogate))
        {
            return false;
        }

        if ((LowSurrogate < GLowSurrogateFirst) || (LowSurrogate > GLowSurrogateLast))
        {
            return Fail("'\\u%04X' does not pair with the leading surrogate before it", LowSurrogate);
        }

        CodePoint = 0x10000 + ((CodePoint - GHighSurrogateFirst) << 10) + (LowSurrogate - GLowSurrogateFirst);
    }

    AppendUtf8(CodePoint, OutString);
    return true;
}

bool FJsonReader::ParseString(String& OutString)
{
    if (!Expect('"'))
    {
        return false;
    }

    OutString.Clear();

    while (true)
    {
        if (IsAtEnd())
        {
            return Fail("unterminated string");
        }

        const CHAR Character = Peek();
        if (Character == '"')
        {
            Advance();
            return true;
        }

        if (Character == '\\')
        {
            Advance();
            if (!ParseEscape(OutString))
            {
                return false;
            }

            continue;
        }

        if (static_cast<uint8>(Character) < 0x20)
        {
            return Fail("control character 0x%02X must be escaped inside a string", static_cast<uint32>(static_cast<uint8>(Character)));
        }

        OutString.Append(Character);
        Advance();
    }
}

bool FJsonReader::ParseNumber(FJsonValue& OutValue)
{
    const CHAR* NumberStart = Current;

    if (Peek() == '-')
    {
        Advance();
    }

    if (!IsDigit(Peek()))
    {
        if (IsAtEnd())
        {
            return Fail("expected a digit but reached the end of the document");
        }

        return Fail("expected a digit after '-' but found '%c'", Peek());
    }

    if (Peek() == '0')
    {
        Advance();
        if (IsDigit(Peek()))
        {
            return Fail("numbers may not have a leading zero");
        }
    }
    else
    {
        while (IsDigit(Peek()))
        {
            Advance();
        }
    }

    bool bIsReal = false;

    if (Peek() == '.')
    {
        Advance();
        if (!IsDigit(Peek()))
        {
            return Fail("expected a digit after the decimal point");
        }

        while (IsDigit(Peek()))
        {
            Advance();
        }

        bIsReal = true;
    }

    if ((Peek() == 'e') || (Peek() == 'E'))
    {
        Advance();
        if ((Peek() == '+') || (Peek() == '-'))
        {
            Advance();
        }

        if (!IsDigit(Peek()))
        {
            return Fail("expected a digit in the exponent");
        }

        while (IsDigit(Peek()))
        {
            Advance();
        }

        bIsReal = true;
    }

    const int32 NumberLength = static_cast<int32>(Current - NumberStart);
    if (NumberLength >= GMaxNumberLength)
    {
        return Fail("the number has more digits than can be represented");
    }

    CHAR Buffer[GMaxNumberLength];
    for (int32 Index = 0; Index < NumberLength; ++Index)
    {
        Buffer[Index] = NumberStart[Index];
    }

    Buffer[NumberLength] = '\0';

    CHAR* ParseEnd = nullptr;
    if (!bIsReal)
    {
        const int64 Integer = CString::Strtoi64(Buffer, &ParseEnd, 10);
        CHAR Printed[GMaxNumberLength];
        CString::Snprintf(Printed, GMaxNumberLength, "%lld", Integer);

        // "-0" is the one integer whose canonical form differs from how it may be written
        const bool bNegativeZero = (Integer == 0) && (Buffer[0] == '-');
        if (bNegativeZero || (CString::Strcmp(Printed, Buffer) == 0))
        {
            OutValue = FJsonValue(Integer);
            return true;
        }
    }

    OutValue = FJsonValue(CString::Strtod(Buffer, &ParseEnd));
    return true;
}
