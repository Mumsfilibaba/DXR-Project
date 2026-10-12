#include "IniFileTests.h"

#include <Core/Containers/Array.h>
#include <Core/Containers/String.h>
#include <Core/Misc/IniFile.h>
#include <Core/Platform/PlatformFile.h>
#include <Core/Filesystem/File.h>
#include <Core/Templates/CString.h>

#include "TestCommon/TestMacros.h"

static FIniFile ParseIni(const CHAR_T* InText)
{
    // The parser walks the buffer in place, so the null-terminator has to be copied along
    TArray<CHAR_T> Text(InText, CString::Strlen(InText) + 1);

    FIniFile IniFile;
    IniFile.ParseFromText(Text);
    return IniFile;
}

static bool WriteTextFile(const FString& Path, const CHAR_T* Text)
{
    TFileRef<IPlatformFile> File = FPlatformFile::OpenForWrite(Path);
    if (!File)
    {
        return false;
    }

    return File::WriteTextFile(File.Get(), FString(Text));
}

bool IniFile_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("Sections and lookup"));
    {
        FIniFile Ini = ParseIni(
            TEXT("[Renderer]\n"
            "Width = 1280\n"
            "Height = 720\n"
            "[Audio]\n"
            "Volume = 0.5\n"));

        TEST_EXPECT_EQ(Ini.Sections.Size(), 2);

        FString Value;
        TEST_EXPECT(Ini.GetString(TEXT("Renderer"), TEXT("Width"), Value));
        TEST_EXPECT(Value == TEXT("1280"));
        TEST_EXPECT(Ini.GetString(TEXT("Audio"), TEXT("Volume"), Value));
        TEST_EXPECT(Value == TEXT("0.5"));

        // A key that lives in another section is not visible through this one
        TEST_EXPECT(!Ini.GetString(TEXT("Renderer"), TEXT("Volume"), Value));

        // Neither is anything in a section that does not exist
        TEST_EXPECT(!Ini.GetString(TEXT("Missing"), TEXT("Width"), Value));
    }

    TEST_SECTION(TEXT("A section header that appears twice reopens the section rather than replacing it"));
    {
        FIniFile Ini = ParseIni(
            TEXT("[Renderer]\n"
            "Width = 1280\n"
            "[Audio]\n"
            "Volume = 0.5\n"
            "[Renderer]\n"
            "Height = 720\n"));

        TEST_EXPECT_EQ(Ini.Sections.Size(), 2);

        FString Value;
        TEST_EXPECT(Ini.GetString(TEXT("Renderer"), TEXT("Width"), Value));
        TEST_EXPECT(Value == TEXT("1280"));
        TEST_EXPECT(Ini.GetString(TEXT("Renderer"), TEXT("Height"), Value));
        TEST_EXPECT(Value == TEXT("720"));
    }

    TEST_SECTION(TEXT("Keys before any header land in the global section"));
    {
        FIniFile Ini = ParseIni(
            TEXT("GlobalKey = 7\n"
            "[Renderer]\n"
            "Width = 1280\n"));

        TEST_EXPECT(Ini.Sections.Contains(TEXT("")));

        FString Value;
        TEST_EXPECT(Ini.GetString(TEXT(""), TEXT("GlobalKey"), Value));
        TEST_EXPECT(Value == TEXT("7"));
    }

    TEST_SECTION(TEXT("An empty section name searches every section"));
    {
        FIniFile Ini = ParseIni(
            TEXT("[Renderer]\n"
            "Width = 1280\n"));

        // This is the path FConsoleManager::RegisterObject relies on
        FString Value;
        TEST_EXPECT(Ini.GetString(TEXT(""), TEXT("Width"), Value));
        TEST_EXPECT(Value == TEXT("1280"));

        TEST_EXPECT(Ini.FindValue(TEXT("Width")) != nullptr);
        TEST_EXPECT(Ini.FindValue(nullptr, TEXT("Width")) != nullptr);
        TEST_EXPECT(Ini.FindValue(TEXT("NotPresent")) == nullptr);
    }

    TEST_SECTION(TEXT("Comments and surrounding whitespace"));
    {
        FIniFile Ini = ParseIni(
            TEXT("; A leading comment\n"
            "[Renderer]\n"
            "   Indented = 1\n"
            "Spaced   =   2\n"
            "; Commented = 3\n"));

        FString Value;
        TEST_EXPECT(Ini.GetString(TEXT("Renderer"), TEXT("Indented"), Value));
        TEST_EXPECT(Value == TEXT("1"));
        TEST_EXPECT(Ini.GetString(TEXT("Renderer"), TEXT("Spaced"), Value));
        TEST_EXPECT(Value == TEXT("2"));

        // Commented-out entries are not values
        TEST_EXPECT(!Ini.GetString(TEXT("Renderer"), TEXT("Commented"), Value));
    }

    TEST_SECTION(TEXT("Quoted values keep their inner spaces"));
    {
        FIniFile Ini = ParseIni(
            TEXT("[Window]\n"
            "Title = \"Main Window\"\n"
            "Trailing = \"Value \"\n"));

        FString Value;
        TEST_EXPECT(Ini.GetString(TEXT("Window"), TEXT("Title"), Value));
        TEST_EXPECT(Value == TEXT("Main Window"));
        TEST_EXPECT(Ini.GetString(TEXT("Window"), TEXT("Trailing"), Value));
        TEST_EXPECT(Value == TEXT("Value "));
    }

    TEST_SECTION(TEXT("Unquoted values stop at the first space"));
    {
        FIniFile Ini = ParseIni(
            TEXT("[Renderer]\n"
            "Key = 5 ignored\n"));

        FString Value;
        TEST_EXPECT(Ini.GetString(TEXT("Renderer"), TEXT("Key"), Value));
        TEST_EXPECT(Value == TEXT("5"));
    }

    TEST_SECTION(TEXT("CRLF parses the same as LF"));
    {
        FIniFile Unix = ParseIni(
            TEXT("[Renderer]\n"
            "Width = 1280\n"
            "Name = \"Main Window\"\n"));

        FIniFile Windows = ParseIni(
            TEXT("[Renderer]\r\n"
            "Width = 1280\r\n"
            "Name = \"Main Window\"\r\n"));

        TEST_EXPECT(Windows.Sections == Unix.Sections);
    }

    TEST_SECTION(TEXT("Typed getters"));
    {
        FIniFile Ini = ParseIni(
            TEXT("[Values]\n"
            "IntValue = 42\n"
            "NegativeInt = -7\n"
            "FloatValue = 0.5\n"
            "BoolTrue = true\n"
            "BoolFalse = false\n"
            "NotANumber = abc\n"));

        int32 IntValue = 0;
        TEST_EXPECT(Ini.GetInt(TEXT("Values"), TEXT("IntValue"), IntValue));
        TEST_EXPECT_EQ(IntValue, 42);
        TEST_EXPECT(Ini.GetInt(TEXT("Values"), TEXT("NegativeInt"), IntValue));
        TEST_EXPECT_EQ(IntValue, -7);

        float FloatValue = 0.0f;
        TEST_EXPECT(Ini.GetFloat(TEXT("Values"), TEXT("FloatValue"), FloatValue));
        TEST_EXPECT_EQ(FloatValue, 0.5f);

        bool bValue = false;
        TEST_EXPECT(Ini.GetBool(TEXT("Values"), TEXT("BoolTrue"), bValue));
        TEST_EXPECT_EQ(bValue, true);
        TEST_EXPECT(Ini.GetBool(TEXT("Values"), TEXT("BoolFalse"), bValue));
        TEST_EXPECT_EQ(bValue, false);

        // A value that does not convert reports failure rather than a garbage result
        TEST_EXPECT(!Ini.GetInt(TEXT("Values"), TEXT("NotANumber"), IntValue));
        TEST_EXPECT(!Ini.GetFloat(TEXT("Values"), TEXT("NotANumber"), FloatValue));
        TEST_EXPECT(!Ini.GetBool(TEXT("Values"), TEXT("NotANumber"), bValue));
    }

    TEST_SECTION(TEXT("Typed setters round-trip through the getters"));
    {
        FIniFile Ini = ParseIni(
            TEXT("[Values]\n"
            "IntValue = 0\n"
            "FloatValue = 0.0\n"
            "BoolValue = false\n"
            "StringValue = Original\n"));

        TEST_EXPECT(Ini.SetInt(TEXT("Values"), TEXT("IntValue"), 7));
        int32 IntValue = 0;
        TEST_EXPECT(Ini.GetInt(TEXT("Values"), TEXT("IntValue"), IntValue));
        TEST_EXPECT_EQ(IntValue, 7);

        TEST_EXPECT(Ini.SetFloat(TEXT("Values"), TEXT("FloatValue"), 0.25f));
        float FloatValue = 0.0f;
        TEST_EXPECT(Ini.GetFloat(TEXT("Values"), TEXT("FloatValue"), FloatValue));
        TEST_EXPECT_EQ(FloatValue, 0.25f);

        TEST_EXPECT(Ini.SetBool(TEXT("Values"), TEXT("BoolValue"), true));
        bool bValue = false;
        TEST_EXPECT(Ini.GetBool(TEXT("Values"), TEXT("BoolValue"), bValue));
        TEST_EXPECT_EQ(bValue, true);

        TEST_EXPECT(Ini.SetString(TEXT("Values"), TEXT("StringValue"), FString(TEXT("Changed"))));
        FString Value;
        TEST_EXPECT(Ini.GetString(TEXT("Values"), TEXT("StringValue"), Value));
        TEST_EXPECT(Value == TEXT("Changed"));

        // Setting a key that was never in the file fails rather than creating it
        TEST_EXPECT(!Ini.SetString(TEXT("Values"), TEXT("NotPresent"), FString(TEXT("Value"))));
    }

    TEST_SECTION(TEXT("The set-or-add setters author a file that starts out empty"));
    {
        FIniFile Ini;

        Ini.SetOrAddString(TEXT("Layout"), TEXT("Kind"), FString(TEXT("Split")));
        Ini.SetOrAddInt(TEXT("Layout"), TEXT("NumNodes"), 3);
        Ini.SetOrAddFloat(TEXT("Layout"), TEXT("Fraction"), 0.25f);
        Ini.SetOrAddBool(TEXT("Layout"), TEXT("Collapsed"), true);

        TEST_EXPECT_EQ(Ini.Sections.Size(), 1);

        // The section carries its own name, which is what a dump writes the header from
        FIniSection* Section = Ini.Sections.Find(TEXT("Layout"));
        TEST_EXPECT(Section != nullptr);
        TEST_EXPECT(Section->Name == TEXT("Layout"));

        FString Value;
        TEST_EXPECT(Ini.GetString(TEXT("Layout"), TEXT("Kind"), Value));
        TEST_EXPECT(Value == TEXT("Split"));

        int32 NumNodes = 0;
        TEST_EXPECT(Ini.GetInt(TEXT("Layout"), TEXT("NumNodes"), NumNodes));
        TEST_EXPECT_EQ(NumNodes, 3);

        float Fraction = 0.0f;
        TEST_EXPECT(Ini.GetFloat(TEXT("Layout"), TEXT("Fraction"), Fraction));
        TEST_EXPECT_EQ(Fraction, 0.25f);

        bool bCollapsed = false;
        TEST_EXPECT(Ini.GetBool(TEXT("Layout"), TEXT("Collapsed"), bCollapsed));
        TEST_EXPECT_EQ(bCollapsed, true);
    }

    TEST_SECTION(TEXT("A set-or-add over a parsed key changes it without adding a second one"));
    {
        FIniFile Ini = ParseIni(
            TEXT("[Values]\n"
            "StringValue = Original\n"));

        Ini.SetOrAddString(TEXT("Values"), TEXT("StringValue"), FString(TEXT("Changed")));

        TEST_EXPECT_EQ(Ini.Sections.Size(), 1);
        TEST_EXPECT_EQ(Ini.Sections.Find(TEXT("Values"))->Values.Size(), 1);

        FString Value;
        TEST_EXPECT(Ini.GetString(TEXT("Values"), TEXT("StringValue"), Value));
        TEST_EXPECT(Value == TEXT("Changed"));

        // The value read from disk is left alone, so a restore still reaches it
        Ini.Sections.Find(TEXT("Values"))->Restore();
        TEST_EXPECT(Ini.GetString(TEXT("Values"), TEXT("StringValue"), Value));
        TEST_EXPECT(Value == TEXT("Original"));
    }

    TEST_SECTION(TEXT("An authored file parses back from its own dump"));
    {
        FIniFile Ini;
        Ini.SetOrAddInt(TEXT("Layout"), TEXT("Version"), 1);
        Ini.SetOrAddString(TEXT("Node0"), TEXT("Kind"), FString(TEXT("Tabs")));

        FString Dumped;
        Ini.DumpToString(Dumped);

        FIniFile Reparsed = ParseIni(*Dumped);
        TEST_EXPECT(Reparsed.Sections == Ini.Sections);
    }

    TEST_SECTION(TEXT("Restore and SaveCurrent"));
    {
        FIniFile Ini = ParseIni(
            TEXT("[Values]\n"
            "Key = Original\n"));

        FIniSection* Section = Ini.Sections.Find(TEXT("Values"));
        TEST_EXPECT(Section != nullptr);

        if (Section)
        {
            FString Value;

            // An edit is visible immediately but can be rolled back to what the file held
            TEST_EXPECT(Ini.SetString(TEXT("Values"), TEXT("Key"), FString(TEXT("Changed"))));
            TEST_EXPECT(Ini.GetString(TEXT("Values"), TEXT("Key"), Value));
            TEST_EXPECT(Value == TEXT("Changed"));

            Section->Restore();
            TEST_EXPECT(Ini.GetString(TEXT("Values"), TEXT("Key"), Value));
            TEST_EXPECT(Value == TEXT("Original"));

            // Once saved, the edit becomes the value a restore falls back to
            TEST_EXPECT(Ini.SetString(TEXT("Values"), TEXT("Key"), FString(TEXT("Kept"))));
            if (FIniValue* IniValue = Ini.FindValue(TEXT("Values"), TEXT("Key")))
            {
                IniValue->SaveCurrent();
            }

            Section->Restore();
            TEST_EXPECT(Ini.GetString(TEXT("Values"), TEXT("Key"), Value));
            TEST_EXPECT(Value == TEXT("Kept"));
        }
    }

    TEST_SECTION(TEXT("Dump round-trip"));
    {
        FIniFile Original = ParseIni(
            TEXT("GlobalKey = 1\n"
            "[Renderer]\n"
            "Width = 1280\n"
            "Name = \"Main Window\"\n"));

        FString Dumped;
        Original.DumpToString(Dumped);

        FIniFile Reparsed = ParseIni(*Dumped);
        TEST_EXPECT(Reparsed.Sections == Original.Sections);
    }

    TEST_SECTION(TEXT("Malformed input"));
    {
        FIniFile Ini = ParseIni(
            TEXT("[Unclosed\n"
            "NoEqualsSign\n"
            "Unterminated = \"oops\n"
            "[Valid]\n"
            "Key = 1\n"));

        // The malformed lines are ignored or clamped to the line, parsing continues
        FString Value;
        TEST_EXPECT(Ini.GetString(TEXT("Valid"), TEXT("Key"), Value));
        TEST_EXPECT(Value == TEXT("1"));
        TEST_EXPECT(Ini.GetString(TEXT(""), TEXT("Unterminated"), Value));
        TEST_EXPECT(Value == TEXT("oops"));

        // The unclosed header never opened a section
        TEST_EXPECT(!Ini.Sections.Contains(TEXT("Unclosed")));
        TEST_EXPECT(Ini.FindValue(TEXT("NoEqualsSign")) == nullptr);
    }

    TEST_SECTION(TEXT("File round-trip"));
    {
        // Relative, so it lands in the working directory next to TestResults_Core.log
        const FString TempPath(TEXT("IniFileTests.tmp.ini"));

        FIniFile Written = ParseIni(
            TEXT("[Renderer]\n"
            "Width = 1280\n"
            "Name = \"Main Window\"\n"));
        Written.Filename = TempPath;
        TEST_EXPECT(Written.WriteToFile());

        FIniFile Loaded;
        TEST_EXPECT(Loaded.LoadFromFile(TempPath));
        TEST_EXPECT(Loaded.Sections == Written.Sections);
        TEST_EXPECT(Loaded.Filename == TempPath);

        // Loading a path that does not exist fails without leaving state behind
        FIniFile Missing;
        TEST_EXPECT(!Missing.LoadFromFile(TempPath + TEXT(".does-not-exist")));
        TEST_EXPECT(Missing.Sections.IsEmpty());

        FPlatformFile::DeleteFile(TempPath.Data());
    }

    TEST_SECTION(TEXT("Include directive"));
    {
        const FString ChildPath(TEXT("IniFileTests.child.ini"));
        const FString ParentPath(TEXT("IniFileTests.parent.ini"));

        TEST_EXPECT(WriteTextFile(ChildPath,
            TEXT("[Audio]\n"
            "Volume = 0.5\n"
            "[Renderer]\n"
            "Height = 720\n")));

        TEST_EXPECT(WriteTextFile(ParentPath,
            TEXT("[Renderer]\n"
            "include \"IniFileTests.child.ini\"\n"
            "Width = 1280\n"
            "Height = 900\n")));

        FIniFile Parent;
        TEST_EXPECT(Parent.LoadFromFile(ParentPath));

        FString Value;
        TEST_EXPECT(Parent.GetString(TEXT("Audio"), TEXT("Volume"), Value));
        TEST_EXPECT(Value == TEXT("0.5"));
        TEST_EXPECT(Parent.GetString(TEXT("Renderer"), TEXT("Width"), Value));
        TEST_EXPECT(Value == TEXT("1280"));

        TEST_EXPECT(Parent.GetString(TEXT("Renderer"), TEXT("Height"), Value));
        TEST_EXPECT(Value == TEXT("900"));

        FPlatformFile::DeleteFile(ChildPath.Data());
        FPlatformFile::DeleteFile(ParentPath.Data());
    }

    TEST_SECTION(TEXT("Include override order"));
    {
        const FString ChildPath(TEXT("IniFileTests.override.child.ini"));
        const FString ParentPath(TEXT("IniFileTests.override.parent.ini"));

        TEST_EXPECT(WriteTextFile(ChildPath,
            TEXT("[Values]\n"
            "Key = FromChild\n")));

        TEST_EXPECT(WriteTextFile(ParentPath,
            TEXT("[Values]\n"
            "Key = FromParentBefore\n"
            "include \"IniFileTests.override.child.ini\"\n"
            "Key = FromParentAfter\n")));

        FIniFile Parent;
        TEST_EXPECT(Parent.LoadFromFile(ParentPath));

        FString Value;
        TEST_EXPECT(Parent.GetString(TEXT("Values"), TEXT("Key"), Value));
        TEST_EXPECT(Value == TEXT("FromParentAfter"));

        FPlatformFile::DeleteFile(ChildPath.Data());
        FPlatformFile::DeleteFile(ParentPath.Data());
    }

    TEST_SECTION(TEXT("Included file starts in the global section"));
    {
        const FString ChildPath(TEXT("IniFileTests.global.child.ini"));
        const FString ParentPath(TEXT("IniFileTests.global.parent.ini"));

        TEST_EXPECT(WriteTextFile(ChildPath,
            TEXT("GlobalKey = FromChild\n")));

        TEST_EXPECT(WriteTextFile(ParentPath,
            TEXT("[Renderer]\n"
            "include \"IniFileTests.global.child.ini\"\n"
            "Width = 1280\n")));

        FIniFile Parent;
        TEST_EXPECT(Parent.LoadFromFile(ParentPath));

        FString Value;
        TEST_EXPECT(Parent.GetString(TEXT(""), TEXT("GlobalKey"), Value));
        TEST_EXPECT(Value == TEXT("FromChild"));
        TEST_EXPECT(Parent.GetString(TEXT("Renderer"), TEXT("Width"), Value));
        TEST_EXPECT(Value == TEXT("1280"));

        FPlatformFile::DeleteFile(ChildPath.Data());
        FPlatformFile::DeleteFile(ParentPath.Data());
    }

    TEST_SECTION(TEXT("A file including itself is refused"));
    {
        const FString SelfPath(TEXT("IniFileTests.self.ini"));

        TEST_EXPECT(WriteTextFile(SelfPath,
            TEXT("include \"IniFileTests.self.ini\"\n"
            "[Valid]\n"
            "Key = 1\n")));

        FIniFile Self;
        TEST_EXPECT(Self.LoadFromFile(SelfPath));

        FString Value;
        TEST_EXPECT(Self.GetString(TEXT("Valid"), TEXT("Key"), Value));
        TEST_EXPECT(Value == TEXT("1"));

        FPlatformFile::DeleteFile(SelfPath.Data());
    }

    TEST_SECTION(TEXT("A missing include warns and parsing continues"));
    {
        const FString ParentPath(TEXT("IniFileTests.missing.parent.ini"));

        TEST_EXPECT(WriteTextFile(ParentPath,
            TEXT("include \"IniFileTests.missing.does-not-exist.ini\"\n"
            "[Valid]\n"
            "Key = 1\n")));

        FIniFile Parent;
        TEST_EXPECT(Parent.LoadFromFile(ParentPath));

        FString Value;
        TEST_EXPECT(Parent.GetString(TEXT("Valid"), TEXT("Key"), Value));
        TEST_EXPECT(Value == TEXT("1"));

        FPlatformFile::DeleteFile(ParentPath.Data());
    }

    TEST_SECTION(TEXT("Nested includes reach the deepest file"));
    {
        const FString NestedCPath(TEXT("IniFileTests.c.ini"));
        const FString NestedBPath(TEXT("IniFileTests.b.ini"));
        const FString NestedAPath(TEXT("IniFileTests.a.ini"));

        TEST_EXPECT(WriteTextFile(NestedCPath,
            TEXT("[Values]\n"
            "Key = FromC\n")));

        TEST_EXPECT(WriteTextFile(NestedBPath,
            TEXT("include \"IniFileTests.c.ini\"\n")));

        TEST_EXPECT(WriteTextFile(NestedAPath,
            TEXT("include \"IniFileTests.b.ini\"\n")));

        FIniFile Root;
        TEST_EXPECT(Root.LoadFromFile(NestedAPath));

        FString Value;
        TEST_EXPECT(Root.GetString(TEXT("Values"), TEXT("Key"), Value));
        TEST_EXPECT(Value == TEXT("FromC"));

        FPlatformFile::DeleteFile(NestedCPath.Data());
        FPlatformFile::DeleteFile(NestedBPath.Data());
        FPlatformFile::DeleteFile(NestedAPath.Data());
    }

    TEST_SECTION(TEXT("A line with include and an equals sign is still a key"));
    {
        FIniFile Ini = ParseIni(
            TEXT("[Values]\n"
            "include = 1\n"));

        FString Value;
        TEST_EXPECT(Ini.GetString(TEXT("Values"), TEXT("include"), Value));
        TEST_EXPECT(Value == TEXT("1"));
    }

    TEST_END();
}
