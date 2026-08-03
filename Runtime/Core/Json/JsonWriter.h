#pragma once
#include "Core/Json/JsonValue.h"

// Turns a FJsonValue into text. Output is always strict RFC 8259 with LF line endings and no BOM, 
// whatever the flags say. The formatting choices exist so that scene files committed to git produce 
// small, reviewable diffs.
class CORE_API FJsonWriter
{
public:

    // Appends a JSON string literal, quotes and escaping included. '/' is left alone and UTF-8 above
    // ASCII passes straight through, so text stays readable in a diff.
    static void WriteEscapedString(const String& Value, String& Output);
    
    // Appends a number in the shortest form that reads back as the same value. Values that survive a
    // round trip through float print at float precision, so engine data reads as 0.1 rather than
    // 0.10000000149011612. NaN and both infinities have no JSON spelling and are written as null.
    static void WriteDouble(double Value, String& Output);
    
    // Appends an integer in full, never in exponent form
    static void WriteInt64(int64 Value, String& Output);

public:
    explicit FJsonWriter(EJsonWriteFlags InFlags = EJsonWriteFlags::Pretty);
    ~FJsonWriter() = default;

    // Appends to Output, leaving anything already there untouched.
    void Write(const FJsonValue& Value, String& Output) const;

    // Returns Value as text, with a trailing newline when indenting.
    NODISCARD String ToString(const FJsonValue& Value) const;

private:
    void WriteValue(const FJsonValue& Value, String& Output, int32 Depth) const;
    void WriteArray(const FJsonValue& Value, String& Output, int32 Depth) const;
    void WriteObject(const FJsonValue& Value, String& Output, int32 Depth) const;
    void WriteNewLine(String& Output) const;
    void WriteIndent(String& Output, int32 Depth) const;

    bool ShouldInlineArray(const FJsonValue& Value) const;
    
    NODISCARD bool ShouldIndent() const;

    EJsonWriteFlags Flags;
};
