#include "Core/Json/JsonWriter.h"
#include "Core/Math/Math.h"
#include "Core/Templates/CString.h"

// Holds "%.17f" of any value that takes fixed notation here, sign and all, with room to spare
static constexpr int32 GNumberBufferSize = 64;

static constexpr int32 GFloatMaxPrecision  = 9;
static constexpr int32 GDoubleMaxPrecision = 17;

// Outside this magnitude range fixed notation is longer than it is worth reading
static constexpr int32 GMinFixedExponent = -5;
static constexpr int32 GMaxFixedExponent = 16;

static FORCEINLINE bool RoundTrips(const CHAR_T* Text, double Value, bool bAsFloat)
{
    CHAR_T* ParseEnd = nullptr;
    if (bAsFloat)
    {
        return CString::Strtof(Text, &ParseEnd) == static_cast<float>(Value);
    }

    return CString::Strtod(Text, &ParseEnd) == Value;
}

// Rewrites a %g exponent as a lowercase 'e' with no '+' and no padding, and returns it. Runtimes have
// historically disagreed on how wide to pad an exponent, and output that shifts with the toolchain
// would show up as noise in every diff.
static int32 NormalizeExponent(CHAR_T* Buffer)
{
    CHAR_T* Exponent = nullptr;
    for (CHAR_T* Current = Buffer; *Current != '\0'; ++Current)
    {
        if ((*Current == 'e') || (*Current == 'E'))
        {
            Exponent = Current;
            break;
        }
    }

    if (Exponent == nullptr)
    {
        return 0;
    }

    const CHAR_T* Digits    = Exponent + 1;
    bool          bNegative = false;

    if ((*Digits == '+') || (*Digits == '-'))
    {
        bNegative = (*Digits == '-');
        ++Digits;
    }

    while ((*Digits == '0') && (*(Digits + 1) != '\0'))
    {
        ++Digits;
    }

    int32 Magnitude = 0;
    for (const CHAR_T* Current = Digits; *Current != '\0'; ++Current)
    {
        Magnitude = (Magnitude * 10) + (*Current - '0');
    }

    CHAR_T* Write = Exponent;
    *Write++      = 'e';
    
    if (bNegative)
    {
        *Write++ = '-';
    }

    while (*Digits != '\0')
    {
        *Write++ = *Digits++;
    }

    *Write = '\0';
    return bNegative ? -Magnitude : Magnitude;
}

static void FormatDouble(double Value, CHAR_T* Buffer, int32 BufferSize)
{
    // A value that came from a float, or happens to land exactly on one, prints at float
    // precision so engine data does not turn 0.1f into 0.10000000149011612 in the file.
    const bool  bFloatExact  = (static_cast<double>(static_cast<float>(Value)) == Value);
    const int32 MaxPrecision = bFloatExact ? GFloatMaxPrecision : GDoubleMaxPrecision;

    int32 Precision = MaxPrecision;
    for (int32 Attempt = 1; Attempt < MaxPrecision; ++Attempt)
    {
        CString::Snprintf(Buffer, BufferSize, TEXT("%.*g"), Attempt, Value);
        if (RoundTrips(Buffer, Value, bFloatExact))
        {
            Precision = Attempt;
            break;
        }
    }

    CString::Snprintf(Buffer, BufferSize, TEXT("%.*g"), Precision, Value);

    const int32 Exponent = NormalizeExponent(Buffer);
    if (Exponent == 0)
    {
        return;
    }

    // %g reaches for scientific notation as soon as the exponent passes the digit count, which
    // turns a plain 5000 into 5e3. Spell out anything within a readable range instead.
    if ((Exponent < GMinFixedExponent) || (Exponent > GMaxFixedExponent))
    {
        return;
    }

    CHAR_T FixedBuffer[GNumberBufferSize];
    const int32 Decimals = Math::Max(0, Precision - 1 - Exponent);
    CString::Snprintf(FixedBuffer, GNumberBufferSize, TEXT("%.*f"), Decimals, Value);

    if (RoundTrips(FixedBuffer, Value, bFloatExact))
    {
        CString::Strncpy(Buffer, FixedBuffer, BufferSize);
    }
}


FJsonWriter::FJsonWriter(EJsonWriteFlags InFlags)
    : Flags(InFlags)
{
}

bool FJsonWriter::ShouldIndent() const
{
    return IsEnumFlagSet(Flags, EJsonWriteFlags::Indent);
}

void FJsonWriter::WriteNewLine(FString& Output) const
{
    if (ShouldIndent())
    {
        Output.Append('\n');
    }
}

void FJsonWriter::WriteIndent(FString& Output, int32 Depth) const
{
    if (!ShouldIndent())
    {
        return;
    }

    const int32 NumSpaces = Depth * JSON_INDENT_WIDTH;
    for (int32 Index = 0; Index < NumSpaces; ++Index)
    {
        Output.Append(' ');
    }
}

void FJsonWriter::Write(const FJsonValue& Value, FString& Output) const
{
    WriteValue(Value, Output, 0);
}

FString FJsonWriter::ToString(const FJsonValue& Value) const
{
    FString Output;
    WriteValue(Value, Output, 0);
    WriteNewLine(Output);
    return Output;
}

void FJsonWriter::WriteValue(const FJsonValue& Value, FString& Output, int32 Depth) const
{
    switch (Value.GetType())
    {
        case EJsonType::Null:
        {
            Output.Append(TEXT("null"));
            break;
        }

        case EJsonType::Bool:
        {
            Output.Append(Value.GetBoolOr(false) ? TEXT("true") : TEXT("false"));
            break;
        }

        case EJsonType::Number:
        {
            if (Value.IsIntegral())
            {
                WriteInt64(Value.GetInt64Or(0), Output);
            }
            else
            {
                WriteDouble(Value.GetDoubleOr(0.0), Output);
            }

            break;
        }

        case EJsonType::String:
        {
            WriteEscapedString(Value.GetStringOr(TEXT("")), Output);
            break;
        }

        case EJsonType::Array:
        {
            WriteArray(Value, Output, Depth);
            break;
        }

        case EJsonType::Object:
        {
            WriteObject(Value, Output, Depth);
            break;
        }
    }
}

bool FJsonWriter::ShouldInlineArray(const FJsonValue& Value) const
{
    if (!IsEnumFlagSet(Flags, EJsonWriteFlags::InlineScalarArrays))
    {
        return false;
    }

    if (Value.Num() > JSON_MAX_INLINE_ARRAY_ELEMENTS)
    {
        return false;
    }

    for (int32 Index = 0; Index < Value.Num(); ++Index)
    {
        const EJsonType ElementType = Value[Index].GetType();
        if ((ElementType == EJsonType::Array) || (ElementType == EJsonType::Object) || (ElementType == EJsonType::String))
        {
            return false;
        }
    }

    return true;
}

void FJsonWriter::WriteArray(const FJsonValue& Value, FString& Output, int32 Depth) const
{
    const int32 NumElements = Value.Num();
    if (NumElements == 0)
    {
        Output.Append(TEXT("[]"));
        return;
    }

    const bool bInline = ShouldInlineArray(Value);

    Output.Append('[');
    for (int32 Index = 0; Index < NumElements; ++Index)
    {
        if (Index > 0)
        {
            Output.Append(',');
            if (bInline && ShouldIndent())
            {
                Output.Append(' ');
            }
        }

        if (!bInline)
        {
            WriteNewLine(Output);
            WriteIndent(Output, Depth + 1);
        }

        WriteValue(Value[Index], Output, Depth + 1);
    }

    if (!bInline)
    {
        WriteNewLine(Output);
        WriteIndent(Output, Depth);
    }

    Output.Append(']');
}

void FJsonWriter::WriteObject(const FJsonValue& Value, FString& Output, int32 Depth) const
{
    const int32 NumMembers = Value.NumMembers();
    if (NumMembers == 0)
    {
        Output.Append(TEXT("{}"));
        return;
    }

    Output.Append('{');
    for (int32 Index = 0; Index < NumMembers; ++Index)
    {
        if (Index > 0)
        {
            Output.Append(',');
        }

        WriteNewLine(Output);
        WriteIndent(Output, Depth + 1);

        WriteEscapedString(Value.GetMemberName(Index), Output);
        Output.Append(':');

        if (ShouldIndent())
        {
            Output.Append(' ');
        }

        WriteValue(Value.GetMemberValue(Index), Output, Depth + 1);
    }

    WriteNewLine(Output);
    WriteIndent(Output, Depth);
    Output.Append('}');
}

void FJsonWriter::WriteEscapedString(const FString& Value, FString& Output)
{
    Output.Append('"');

    const CHAR_T* Characters = Value.Data();
    for (int32 Index = 0; Index < Value.Length(); ++Index)
    {
        const CHAR_T Character = Characters[Index];
        switch (Character)
        {
            case '\"': Output.Append(TEXT("\\\"")); continue;
            case '\\': Output.Append(TEXT("\\\\")); continue;
            case '\b': Output.Append(TEXT("\\b"));  continue;
            case '\f': Output.Append(TEXT("\\f"));  continue;
            case '\n': Output.Append(TEXT("\\n"));  continue;
            case '\r': Output.Append(TEXT("\\r"));  continue;
            case '\t': Output.Append(TEXT("\\t"));  continue;
            default: break;
        }

        const uint8 Unsigned = static_cast<uint8>(Character);
        if (Unsigned < 0x20)
        {
            CHAR_T Buffer[8];
            CString::Snprintf(Buffer, ARRAY_COUNT(Buffer), TEXT("\\u%04x"), static_cast<uint32>(Unsigned));
            Output.Append(Buffer);
        }
        else
        {
            Output.Append(Character);
        }
    }

    Output.Append('"');
}

void FJsonWriter::WriteInt64(int64 Value, FString& Output)
{
    CHAR_T Buffer[GNumberBufferSize];
    CString::Snprintf(Buffer, GNumberBufferSize, TEXT("%lld"), Value);
    Output.Append(Buffer);
}

void FJsonWriter::WriteDouble(double Value, FString& Output)
{
    if (Math::IsNaN(Value) || Math::IsInfinity(Value))
    {
        Output.Append(TEXT("null"));
        return;
    }

    CHAR_T Buffer[GNumberBufferSize];
    FormatDouble(Value, Buffer, GNumberBufferSize);
    Output.Append(Buffer);
}
