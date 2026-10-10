#include "Core/Misc/RemoteConsoleProtocol.h"
#include "Core/Json/JsonWriter.h"

const CHAR* RemoteConsoleProtocol::SeverityToString(ELogSeverity Severity)
{
    switch (Severity)
    {
        case ELogSeverity::Warning: return "warning";
        case ELogSeverity::Error:   return "error";
        default:                    return "info";
    }
}

bool RemoteConsoleProtocol::SeverityFromString(const String& Text, ELogSeverity& OutSeverity)
{
    if (Text.Equals("info", EStringCaseType::NoCase))
    {
        OutSeverity = ELogSeverity::Info;
        return true;
    }
    else if (Text.Equals("warning", EStringCaseType::NoCase))
    {
        OutSeverity = ELogSeverity::Warning;
        return true;
    }
    else if (Text.Equals("error", EStringCaseType::NoCase))
    {
        OutSeverity = ELogSeverity::Error;
        return true;
    }

    return false;
}

bool RemoteConsoleProtocol::ExtractLines(String& Buffer, TArray<String>& OutLines)
{
    const CHAR* Data   = Buffer.Data();
    const int32 Length = Buffer.Length();

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

        OutLines.Emplace(Data + LineStart, LineEnd - LineStart);
        LineStart = Index + 1;
    }

    if (LineStart > 0)
    {
        Buffer.Remove(0, LineStart);
    }

    return Buffer.Length() <= MaxLineLength;
}

String RemoteConsoleProtocol::ToLine(const FJsonValue& Message)
{
    const FJsonWriter Writer(EJsonWriteFlags::Compact);

    String Line;
    Writer.Write(Message, Line);
    Line.Append('\n');
    return Line;
}
