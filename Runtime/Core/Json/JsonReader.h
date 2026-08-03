#pragma once
#include "Core/Json/JsonValue.h"

// Recursive-descent parser producing a FJsonValue. Reads a StringView, so it never assumes a null 
// terminator and never copies the input. Failures report a line and column rather than just a 
// message, which is what makes a hand-edited scene file or a botched merge resolution recoverable.
class CORE_API FJsonReader
{
public:
    explicit FJsonReader(EJsonParseFlags InFlags = EJsonParseFlags::None, int32 InMaxDepth = JSON_DEFAULT_MAX_DEPTH);
    ~FJsonReader() = default;

    // Succeeds only when the whole input was one valid value. A leading UTF-8 BOM is 
    // skipped, both LF and CRLF are accepted, and OutValue is left untouched on failure.
    bool Parse(const StringView& Text, FJsonValue& OutValue, FJsonError* OutError = nullptr);

private:
    static void AppendUtf8(uint32 CodePoint, String& OutString);

    bool ParseValue(FJsonValue& OutValue, int32 Depth);
    bool ParseObject(FJsonValue& OutValue, int32 Depth);
    bool ParseArray(FJsonValue& OutValue, int32 Depth);
    bool ParseString(String& OutString);
    bool ParseNumber(FJsonValue& OutValue);
    bool ParseKeyword(const CHAR* Keyword, FJsonValue&& Result, FJsonValue& OutValue);

    // Reads the four hex digits of a \uXXXX escape
    bool ParseHexQuad(uint32& OutCodePoint);

    // Appends a code point to OutString as UTF-8, combining surrogate pairs
    bool ParseEscape(String& OutString);

    void SkipWhitespaceAndComments();
    bool SkipComment();

    NODISCARD bool IsAtEnd() const;
    NODISCARD CHAR Peek() const;
    NODISCARD CHAR PeekAt(int32 Lookahead) const;

    // Consumes one character, keeping the line and column counters in step
    CHAR Advance();

    // Consumes Expected and reports an error naming it when the next character differs
    bool Expect(CHAR Expected);
    bool Fail(const CHAR* Format, ...);

    const CHAR* Start;
    const CHAR* Current;
    const CHAR* End;

    int32 Line;
    int32 LineStartOffset;

    EJsonParseFlags Flags;
    int32           MaxDepth;

    FJsonError Error;
    bool       bHasError;
};
