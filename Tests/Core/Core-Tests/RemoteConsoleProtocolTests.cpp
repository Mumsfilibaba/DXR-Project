#include "RemoteConsoleProtocolTests.h"

#include <Core/Containers/Array.h>
#include <Core/Containers/String.h>
#include <Core/Containers/StringConversion.h>
#include <Core/Json/JsonReader.h>
#include <Core/Json/JsonValue.h>
#include <Core/Misc/RemoteConsoleProtocol.h>

#include "TestCommon/TestMacros.h"

bool RemoteConsoleProtocol_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("Complete lines are extracted and the partial remainder stays buffered"));
    {
        FAnsiString Buffer = "exec one\nexec two\npart";
        TArray<FString> Lines;
        TEST_EXPECT(RemoteConsoleProtocol::ExtractLines(Buffer, Lines));
        TEST_EXPECT_EQ(Lines.Size(), 2);
        TEST_EXPECT(Lines[0] == TEXT("exec one"));
        TEST_EXPECT(Lines[1] == TEXT("exec two"));
        TEST_EXPECT(Buffer == "part");
    }

    TEST_SECTION(TEXT("A line split across reads is joined once its end arrives"));
    {
        FAnsiString Buffer;
        TArray<FString> Lines;

        Buffer.Append("{\"type\":");
        TEST_EXPECT(RemoteConsoleProtocol::ExtractLines(Buffer, Lines));
        TEST_EXPECT(Lines.IsEmpty());

        Buffer.Append("\"ping\"}\n");
        TEST_EXPECT(RemoteConsoleProtocol::ExtractLines(Buffer, Lines));
        TEST_EXPECT_EQ(Lines.Size(), 1);
        TEST_EXPECT(Lines[0] == TEXT("{\"type\":\"ping\"}"));
        TEST_EXPECT(Buffer.IsEmpty());
    }

    TEST_SECTION(TEXT("CRLF line endings lose their carriage return and empty lines survive"));
    {
        FAnsiString Buffer = "first\r\n\r\nsecond\r\n";
        TArray<FString> Lines;
        TEST_EXPECT(RemoteConsoleProtocol::ExtractLines(Buffer, Lines));
        TEST_EXPECT_EQ(Lines.Size(), 3);
        TEST_EXPECT(Lines[0] == TEXT("first"));
        TEST_EXPECT(Lines[1].IsEmpty());
        TEST_EXPECT(Lines[2] == TEXT("second"));
    }

    TEST_SECTION(TEXT("Lines arrive as UTF-8 and are converted to CHAR_T"));
    {
        FAnsiString Buffer = "exec Åäö 日本\n";
        TArray<FString> Lines;
        TEST_EXPECT(RemoteConsoleProtocol::ExtractLines(Buffer, Lines));
        TEST_EXPECT_EQ(Lines.Size(), 1);
        TEST_EXPECT(Lines[0] == TEXT("exec Åäö 日本"));
    }

    TEST_SECTION(TEXT("An unterminated line longer than MaxLineLength is reported"));
    {
        FAnsiString Buffer;
        Buffer.Resize(RemoteConsoleProtocol::MaxLineLength);
        for (int32 Index = 0; Index < Buffer.Length(); ++Index)
        {
            Buffer[Index] = 'a';
        }

        TArray<FString> Lines;
        TEST_EXPECT(RemoteConsoleProtocol::ExtractLines(Buffer, Lines));

        Buffer.Append('a');
        TEST_EXPECT(!RemoteConsoleProtocol::ExtractLines(Buffer, Lines));
        TEST_EXPECT(Lines.IsEmpty());
    }

    TEST_SECTION(TEXT("A message becomes one line of JSON that reads back the same"));
    {
        FJsonValue Message = FJsonValue::MakeObject();
        Message.AddMember(TEXT("type"), FJsonValue(RemoteConsoleProtocol::TypeLog));
        Message.AddMember(TEXT("message"), FJsonValue(TEXT("two\nlines")));

        const FAnsiString Line = RemoteConsoleProtocol::ToLine(Message);
        TEST_EXPECT(Line.EndsWith("\n"));
        TEST_EXPECT_EQ(Line.FindChar('\n'), Line.Length() - 1);

        FJsonReader Reader;
        FJsonValue  ReadBack;
        const TStringConversion<CHAR_T, ANSICHAR> LineText = StringCast<CHAR_T>(Line);
        TEST_CHECK(Reader.Parse(FStringView(LineText.Get(), LineText.Length()), ReadBack));
        TEST_EXPECT(ReadBack == Message);
    }

    TEST_SECTION(TEXT("Severities round-trip through their wire names"));
    {
        const ELogSeverity Severities[] = { ELogSeverity::Info, ELogSeverity::Warning, ELogSeverity::Error };
        for (ELogSeverity Severity : Severities)
        {
            ELogSeverity Parsed = ELogSeverity::Info;
            TEST_EXPECT(RemoteConsoleProtocol::SeverityFromString(RemoteConsoleProtocol::SeverityToString(Severity), Parsed));
            TEST_EXPECT(Parsed == Severity);
        }

        ELogSeverity Unchanged = ELogSeverity::Warning;
        TEST_EXPECT(!RemoteConsoleProtocol::SeverityFromString(TEXT("verbose"), Unchanged));
        TEST_EXPECT(Unchanged == ELogSeverity::Warning);
    }

    TEST_END();
}
