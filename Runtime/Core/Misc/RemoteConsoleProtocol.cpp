#include "Core/Misc/RemoteConsoleProtocol.h"
#include "Core/Json/JsonWriter.h"
#include "Core/Containers/StringConversion.h"

const CHAR_T* RemoteConsoleProtocol::SeverityToString(ELogSeverity Severity)
{
    switch (Severity)
    {
        case ELogSeverity::Warning: return TEXT("warning");
        case ELogSeverity::Error:   return TEXT("error");
        default:                    return TEXT("info");
    }
}

bool RemoteConsoleProtocol::SeverityFromString(const FString& Text, ELogSeverity& OutSeverity)
{
    if (Text.Equals(TEXT("info"), EStringCaseType::NoCase))
    {
        OutSeverity = ELogSeverity::Info;
        return true;
    }
    else if (Text.Equals(TEXT("warning"), EStringCaseType::NoCase))
    {
        OutSeverity = ELogSeverity::Warning;
        return true;
    }
    else if (Text.Equals(TEXT("error"), EStringCaseType::NoCase))
    {
        OutSeverity = ELogSeverity::Error;
        return true;
    }

    return false;
}

bool RemoteConsoleProtocol::ExtractLines(FAnsiString& Buffer, TArray<FString>& OutLines)
{
    const ANSICHAR* Data   = Buffer.Data();
    const int32     Length = Buffer.Length();

    int32 LineStart = 0;
    for (int32 Index = 0; Index < Length; ++Index)
    {
        if (Data[Index] != '\n')
        {
            continue;
        }

        int32 LineEnd = Index;
        if ((LineEnd > LineStart) && (Data[LineEnd - 1] == '\r'))
        {
            --LineEnd;
        }

        OutLines.Emplace(StringCast<CHAR_T>(Data + LineStart, LineEnd - LineStart).ToString());
        LineStart = Index + 1;
    }

    if (LineStart > 0)
    {
        Buffer.Remove(0, LineStart);
    }

    return Buffer.Length() <= MaxLineLength;
}

FAnsiString RemoteConsoleProtocol::ToLine(const FJsonValue& Message)
{
    const FJsonWriter Writer(EJsonWriteFlags::Compact);

    FString Text;
    Writer.Write(Message, Text);

    FAnsiString Line = StringCast<ANSICHAR>(Text).ToString();
    Line.Append('\n');
    return Line;
}
