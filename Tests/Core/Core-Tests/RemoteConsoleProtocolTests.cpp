#include "RemoteConsoleProtocolTests.h"

#include <Core/Containers/Array.h>
#include <Core/Containers/String.h>
#include <Core/Json/JsonReader.h>
#include <Core/Json/JsonValue.h>
#include <Core/Misc/RemoteConsoleProtocol.h>

#include "TestCommon/TestMacros.h"

bool RemoteConsoleProtocol_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Complete lines are extracted and the partial remainder stays buffered");
    {
        String Buffer = "exec one\nexec two\npart";
        TArray<String> Lines;
        TEST_EXPECT(RemoteConsoleProtocol::ExtractLines(Buffer, Lines));
        TEST_EXPECT_EQ(Lines.Size(), 2);
        TEST_EXPECT(Lines[0] == "exec one");
        TEST_EXPECT(Lines[1] == "exec two");
        TEST_EXPECT(Buffer == "part");
    }

    TEST_SECTION("A line split across reads is joined once its end arrives");
    {
        String Buffer;
        TArray<String> Lines;

        Buffer.Append("{\"type\":");
        TEST_EXPECT(RemoteConsoleProtocol::ExtractLines(Buffer, Lines));
        TEST_EXPECT(Lines.IsEmpty());

        Buffer.Append("\"ping\"}\n");
        TEST_EXPECT(RemoteConsoleProtocol::ExtractLines(Buffer, Lines));
        TEST_EXPECT_EQ(Lines.Size(), 1);
        TEST_EXPECT(Lines[0] == "{\"type\":\"ping\"}");
        TEST_EXPECT(Buffer.IsEmpty());
    }

    TEST_SECTION("CRLF line endings lose their carriage return and empty lines survive");
    {
        String Buffer = "first\r\n\r\nsecond\r\n";
        TArray<String> Lines;
        TEST_EXPECT(RemoteConsoleProtocol::ExtractLines(Buffer, Lines));
        TEST_EXPECT_EQ(Lines.Size(), 3);
        TEST_EXPECT(Lines[0] == "first");
        TEST_EXPECT(Lines[1].IsEmpty());
        TEST_EXPECT(Lines[2] == "second");
    }

    TEST_SECTION("An unterminated line longer than MaxLineLength is reported");
    {
        String Buffer;
        Buffer.Resize(RemoteConsoleProtocol::MaxLineLength);
        for (int32 Index = 0; Index < Buffer.Length(); ++Index)
        {
            Buffer[Index] = 'a';
        }

        TArray<String> Lines;
        TEST_EXPECT(RemoteConsoleProtocol::ExtractLines(Buffer, Lines));

        Buffer.Append('a');
        TEST_EXPECT(!RemoteConsoleProtocol::ExtractLines(Buffer, Lines));
        TEST_EXPECT(Lines.IsEmpty());
    }

    TEST_SECTION("A message becomes one line of JSON that reads back the same");
    {
        FJsonValue Message = FJsonValue::MakeObject();
        Message.AddMember("type", FJsonValue(RemoteConsoleProtocol::TypeLog));
        Message.AddMember("message", FJsonValue("two\nlines"));

        const String Line = RemoteConsoleProtocol::ToLine(Message);
        TEST_EXPECT(Line.EndsWith("\n"));
        TEST_EXPECT_EQ(Line.FindChar('\n'), Line.Length() - 1);

        FJsonReader Reader;
        FJsonValue  ReadBack;
        TEST_CHECK(Reader.Parse(StringView(Line.Data(), Line.Length()), ReadBack));
        TEST_EXPECT(ReadBack == Message);
    }

    TEST_SECTION("Severities round-trip through their wire names");
    {
        const ELogSeverity Severities[] = { ELogSeverity::Info, ELogSeverity::Warning, ELogSeverity::Error };
        for (ELogSeverity Severity : Severities)
        {
            ELogSeverity Parsed = ELogSeverity::Info;
            TEST_EXPECT(RemoteConsoleProtocol::SeverityFromString(RemoteConsoleProtocol::SeverityToString(Severity), Parsed));
            TEST_EXPECT(Parsed == Severity);
        }

        ELogSeverity Unchanged = ELogSeverity::Warning;
        TEST_EXPECT(!RemoteConsoleProtocol::SeverityFromString("verbose", Unchanged));
        TEST_EXPECT(Unchanged == ELogSeverity::Warning);
    }

    TEST_END();
}
