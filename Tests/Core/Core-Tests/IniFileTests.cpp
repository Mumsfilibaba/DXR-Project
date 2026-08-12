#include "IniFileTests.h"

#include <Core/Containers/Array.h>
#include <Core/Containers/String.h>
#include <Core/Misc/IniFile.h>
#include <Core/Platform/PlatformFile.h>
#include <Core/Filesystem/File.h>
#include <Core/Templates/CString.h>

#include "TestCommon/TestMacros.h"

// The platform file layer exposes no delete, so the temp-file section cleans up through the CRT
#include <cstdio>

static FIniFile ParseIni(const CHAR* InText)
{
    // The parser walks the buffer in place, so the null-terminator has to be copied along
    TArray<CHAR> Text(InText, CString::Strlen(InText) + 1);

    FIniFile IniFile;
    IniFile.ParseFromText(Text);
    return IniFile;
}

static bool WriteTextFile(const String& Path, const CHAR* Text)
{
    TFileRef<IPlatformFile> File = FPlatformFile::OpenForWrite(Path);
    if (!File)
    {
        return false;
    }

    return File::WriteTextFile(File.Get(), String(Text));
}

bool IniFile_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Sections and lookup");
    {
        FIniFile Ini = ParseIni(
            "[Renderer]\n"
            "Width = 1280\n"
            "Height = 720\n"
            "[Audio]\n"
            "Volume = 0.5\n");

        TEST_EXPECT_EQ(Ini.Sections.Size(), 2);

        String Value;
        TEST_EXPECT(Ini.GetString("Renderer", "Width", Value));
        TEST_EXPECT(Value == "1280");
        TEST_EXPECT(Ini.GetString("Audio", "Volume", Value));
        TEST_EXPECT(Value == "0.5");

        // A key that lives in another section is not visible through this one
        TEST_EXPECT(!Ini.GetString("Renderer", "Volume", Value));

        // Neither is anything in a section that does not exist
        TEST_EXPECT(!Ini.GetString("Missing", "Width", Value));
    }

    TEST_SECTION("Keys before any header land in the global section");
    {
        FIniFile Ini = ParseIni(
            "GlobalKey = 7\n"
            "[Renderer]\n"
            "Width = 1280\n");

        TEST_EXPECT(Ini.Sections.Contains(""));

        String Value;
        TEST_EXPECT(Ini.GetString("", "GlobalKey", Value));
        TEST_EXPECT(Value == "7");
    }

    TEST_SECTION("An empty section name searches every section");
    {
        FIniFile Ini = ParseIni(
            "[Renderer]\n"
            "Width = 1280\n");

        // This is the path FConsoleManager::RegisterObject relies on
        String Value;
        TEST_EXPECT(Ini.GetString("", "Width", Value));
        TEST_EXPECT(Value == "1280");

        TEST_EXPECT(Ini.FindValue("Width") != nullptr);
        TEST_EXPECT(Ini.FindValue(nullptr, "Width") != nullptr);
        TEST_EXPECT(Ini.FindValue("NotPresent") == nullptr);
    }

    TEST_SECTION("Comments and surrounding whitespace");
    {
        FIniFile Ini = ParseIni(
            "; A leading comment\n"
            "[Renderer]\n"
            "   Indented = 1\n"
            "Spaced   =   2\n"
            "; Commented = 3\n");

        String Value;
        TEST_EXPECT(Ini.GetString("Renderer", "Indented", Value));
        TEST_EXPECT(Value == "1");
        TEST_EXPECT(Ini.GetString("Renderer", "Spaced", Value));
        TEST_EXPECT(Value == "2");

        // Commented-out entries are not values
        TEST_EXPECT(!Ini.GetString("Renderer", "Commented", Value));
    }

    TEST_SECTION("Quoted values keep their inner spaces");
    {
        FIniFile Ini = ParseIni(
            "[Window]\n"
            "Title = \"Main Window\"\n"
            "Trailing = \"Value \"\n");

        String Value;
        TEST_EXPECT(Ini.GetString("Window", "Title", Value));
        TEST_EXPECT(Value == "Main Window");
        TEST_EXPECT(Ini.GetString("Window", "Trailing", Value));
        TEST_EXPECT(Value == "Value ");
    }

    TEST_SECTION("Unquoted values stop at the first space");
    {
        FIniFile Ini = ParseIni(
            "[Renderer]\n"
            "Key = 5 ignored\n");

        String Value;
        TEST_EXPECT(Ini.GetString("Renderer", "Key", Value));
        TEST_EXPECT(Value == "5");
    }

    TEST_SECTION("CRLF parses the same as LF");
    {
        FIniFile Unix = ParseIni(
            "[Renderer]\n"
            "Width = 1280\n"
            "Name = \"Main Window\"\n");

        FIniFile Windows = ParseIni(
            "[Renderer]\r\n"
            "Width = 1280\r\n"
            "Name = \"Main Window\"\r\n");

        TEST_EXPECT(Windows.Sections == Unix.Sections);
    }

    TEST_SECTION("Typed getters");
    {
        FIniFile Ini = ParseIni(
            "[Values]\n"
            "IntValue = 42\n"
            "NegativeInt = -7\n"
            "FloatValue = 0.5\n"
            "BoolTrue = true\n"
            "BoolFalse = false\n"
            "NotANumber = abc\n");

        int32 IntValue = 0;
        TEST_EXPECT(Ini.GetInt("Values", "IntValue", IntValue));
        TEST_EXPECT_EQ(IntValue, 42);
        TEST_EXPECT(Ini.GetInt("Values", "NegativeInt", IntValue));
        TEST_EXPECT_EQ(IntValue, -7);

        float FloatValue = 0.0f;
        TEST_EXPECT(Ini.GetFloat("Values", "FloatValue", FloatValue));
        TEST_EXPECT_EQ(FloatValue, 0.5f);

        bool bValue = false;
        TEST_EXPECT(Ini.GetBool("Values", "BoolTrue", bValue));
        TEST_EXPECT_EQ(bValue, true);
        TEST_EXPECT(Ini.GetBool("Values", "BoolFalse", bValue));
        TEST_EXPECT_EQ(bValue, false);

        // A value that does not convert reports failure rather than a garbage result
        TEST_EXPECT(!Ini.GetInt("Values", "NotANumber", IntValue));
        TEST_EXPECT(!Ini.GetFloat("Values", "NotANumber", FloatValue));
        TEST_EXPECT(!Ini.GetBool("Values", "NotANumber", bValue));
    }

    TEST_SECTION("Typed setters round-trip through the getters");
    {
        FIniFile Ini = ParseIni(
            "[Values]\n"
            "IntValue = 0\n"
            "FloatValue = 0.0\n"
            "BoolValue = false\n"
            "StringValue = Original\n");

        TEST_EXPECT(Ini.SetInt("Values", "IntValue", 7));
        int32 IntValue = 0;
        TEST_EXPECT(Ini.GetInt("Values", "IntValue", IntValue));
        TEST_EXPECT_EQ(IntValue, 7);

        TEST_EXPECT(Ini.SetFloat("Values", "FloatValue", 0.25f));
        float FloatValue = 0.0f;
        TEST_EXPECT(Ini.GetFloat("Values", "FloatValue", FloatValue));
        TEST_EXPECT_EQ(FloatValue, 0.25f);

        TEST_EXPECT(Ini.SetBool("Values", "BoolValue", true));
        bool bValue = false;
        TEST_EXPECT(Ini.GetBool("Values", "BoolValue", bValue));
        TEST_EXPECT_EQ(bValue, true);

        TEST_EXPECT(Ini.SetString("Values", "StringValue", String("Changed")));
        String Value;
        TEST_EXPECT(Ini.GetString("Values", "StringValue", Value));
        TEST_EXPECT(Value == "Changed");

        // Setting a key that was never in the file fails rather than creating it
        TEST_EXPECT(!Ini.SetString("Values", "NotPresent", String("Value")));
    }

    TEST_SECTION("Restore and SaveCurrent");
    {
        FIniFile Ini = ParseIni(
            "[Values]\n"
            "Key = Original\n");

        FIniSection* Section = Ini.Sections.Find("Values");
        TEST_EXPECT(Section != nullptr);

        if (Section)
        {
            String Value;

            // An edit is visible immediately but can be rolled back to what the file held
            TEST_EXPECT(Ini.SetString("Values", "Key", String("Changed")));
            TEST_EXPECT(Ini.GetString("Values", "Key", Value));
            TEST_EXPECT(Value == "Changed");

            Section->Restore();
            TEST_EXPECT(Ini.GetString("Values", "Key", Value));
            TEST_EXPECT(Value == "Original");

            // Once saved, the edit becomes the value a restore falls back to
            TEST_EXPECT(Ini.SetString("Values", "Key", String("Kept")));
            if (FIniValue* IniValue = Ini.FindValue("Values", "Key"))
            {
                IniValue->SaveCurrent();
            }

            Section->Restore();
            TEST_EXPECT(Ini.GetString("Values", "Key", Value));
            TEST_EXPECT(Value == "Kept");
        }
    }

    TEST_SECTION("Dump round-trip");
    {
        FIniFile Original = ParseIni(
            "GlobalKey = 1\n"
            "[Renderer]\n"
            "Width = 1280\n"
            "Name = \"Main Window\"\n");

        String Dumped;
        Original.DumpToString(Dumped);

        FIniFile Reparsed = ParseIni(*Dumped);
        TEST_EXPECT(Reparsed.Sections == Original.Sections);
    }

    TEST_SECTION("Malformed input");
    {
        FIniFile Ini = ParseIni(
            "[Unclosed\n"
            "NoEqualsSign\n"
            "Unterminated = \"oops\n"
            "[Valid]\n"
            "Key = 1\n");

        // The malformed lines are ignored or clamped to the line, parsing continues
        String Value;
        TEST_EXPECT(Ini.GetString("Valid", "Key", Value));
        TEST_EXPECT(Value == "1");
        TEST_EXPECT(Ini.GetString("", "Unterminated", Value));
        TEST_EXPECT(Value == "oops");

        // The unclosed header never opened a section
        TEST_EXPECT(!Ini.Sections.Contains("Unclosed"));
        TEST_EXPECT(Ini.FindValue("NoEqualsSign") == nullptr);
    }

    TEST_SECTION("File round-trip");
    {
        // Relative, so it lands in the working directory next to TestResults_Core.log
        const String TempPath("IniFileTests.tmp.ini");

        FIniFile Written = ParseIni(
            "[Renderer]\n"
            "Width = 1280\n"
            "Name = \"Main Window\"\n");
        Written.Filename = TempPath;
        TEST_EXPECT(Written.WriteToFile());

        FIniFile Loaded;
        TEST_EXPECT(Loaded.LoadFromFile(TempPath));
        TEST_EXPECT(Loaded.Sections == Written.Sections);
        TEST_EXPECT(Loaded.Filename == TempPath);

        // Loading a path that does not exist fails without leaving state behind
        FIniFile Missing;
        TEST_EXPECT(!Missing.LoadFromFile(TempPath + ".does-not-exist"));
        TEST_EXPECT(Missing.Sections.IsEmpty());

        ::remove(TempPath.Data());
    }

    TEST_SECTION("Include directive");
    {
        const String ChildPath("IniFileTests.child.ini");
        const String ParentPath("IniFileTests.parent.ini");

        TEST_EXPECT(WriteTextFile(ChildPath,
            "[Audio]\n"
            "Volume = 0.5\n"
            "[Renderer]\n"
            "Height = 720\n"));

        TEST_EXPECT(WriteTextFile(ParentPath,
            "[Renderer]\n"
            "include \"IniFileTests.child.ini\"\n"
            "Width = 1280\n"
            "Height = 900\n"));

        FIniFile Parent;
        TEST_EXPECT(Parent.LoadFromFile(ParentPath));

        String Value;
        TEST_EXPECT(Parent.GetString("Audio", "Volume", Value));
        TEST_EXPECT(Value == "0.5");
        TEST_EXPECT(Parent.GetString("Renderer", "Width", Value));
        TEST_EXPECT(Value == "1280");

        TEST_EXPECT(Parent.GetString("Renderer", "Height", Value));
        TEST_EXPECT(Value == "900");

        ::remove(ChildPath.Data());
        ::remove(ParentPath.Data());
    }

    TEST_SECTION("Include override order");
    {
        const String ChildPath("IniFileTests.override.child.ini");
        const String ParentPath("IniFileTests.override.parent.ini");

        TEST_EXPECT(WriteTextFile(ChildPath,
            "[Values]\n"
            "Key = FromChild\n"));

        TEST_EXPECT(WriteTextFile(ParentPath,
            "[Values]\n"
            "Key = FromParentBefore\n"
            "include \"IniFileTests.override.child.ini\"\n"
            "Key = FromParentAfter\n"));

        FIniFile Parent;
        TEST_EXPECT(Parent.LoadFromFile(ParentPath));

        String Value;
        TEST_EXPECT(Parent.GetString("Values", "Key", Value));
        TEST_EXPECT(Value == "FromParentAfter");

        ::remove(ChildPath.Data());
        ::remove(ParentPath.Data());
    }

    TEST_SECTION("Included file starts in the global section");
    {
        const String ChildPath("IniFileTests.global.child.ini");
        const String ParentPath("IniFileTests.global.parent.ini");

        TEST_EXPECT(WriteTextFile(ChildPath,
            "GlobalKey = FromChild\n"));

        TEST_EXPECT(WriteTextFile(ParentPath,
            "[Renderer]\n"
            "include \"IniFileTests.global.child.ini\"\n"
            "Width = 1280\n"));

        FIniFile Parent;
        TEST_EXPECT(Parent.LoadFromFile(ParentPath));

        String Value;
        TEST_EXPECT(Parent.GetString("", "GlobalKey", Value));
        TEST_EXPECT(Value == "FromChild");
        TEST_EXPECT(Parent.GetString("Renderer", "Width", Value));
        TEST_EXPECT(Value == "1280");

        ::remove(ChildPath.Data());
        ::remove(ParentPath.Data());
    }

    TEST_SECTION("A file including itself is refused");
    {
        const String SelfPath("IniFileTests.self.ini");

        TEST_EXPECT(WriteTextFile(SelfPath,
            "include \"IniFileTests.self.ini\"\n"
            "[Valid]\n"
            "Key = 1\n"));

        FIniFile Self;
        TEST_EXPECT(Self.LoadFromFile(SelfPath));

        String Value;
        TEST_EXPECT(Self.GetString("Valid", "Key", Value));
        TEST_EXPECT(Value == "1");

        ::remove(SelfPath.Data());
    }

    TEST_SECTION("A missing include warns and parsing continues");
    {
        const String ParentPath("IniFileTests.missing.parent.ini");

        TEST_EXPECT(WriteTextFile(ParentPath,
            "include \"IniFileTests.missing.does-not-exist.ini\"\n"
            "[Valid]\n"
            "Key = 1\n"));

        FIniFile Parent;
        TEST_EXPECT(Parent.LoadFromFile(ParentPath));

        String Value;
        TEST_EXPECT(Parent.GetString("Valid", "Key", Value));
        TEST_EXPECT(Value == "1");

        ::remove(ParentPath.Data());
    }

    TEST_SECTION("Nested includes reach the deepest file");
    {
        const String NestedCPath("IniFileTests.c.ini");
        const String NestedBPath("IniFileTests.b.ini");
        const String NestedAPath("IniFileTests.a.ini");

        TEST_EXPECT(WriteTextFile(NestedCPath,
            "[Values]\n"
            "Key = FromC\n"));

        TEST_EXPECT(WriteTextFile(NestedBPath,
            "include \"IniFileTests.c.ini\"\n"));

        TEST_EXPECT(WriteTextFile(NestedAPath,
            "include \"IniFileTests.b.ini\"\n"));

        FIniFile Root;
        TEST_EXPECT(Root.LoadFromFile(NestedAPath));

        String Value;
        TEST_EXPECT(Root.GetString("Values", "Key", Value));
        TEST_EXPECT(Value == "FromC");

        ::remove(NestedCPath.Data());
        ::remove(NestedBPath.Data());
        ::remove(NestedAPath.Data());
    }

    TEST_SECTION("A line with include and an equals sign is still a key");
    {
        FIniFile Ini = ParseIni(
            "[Values]\n"
            "include = 1\n");

        String Value;
        TEST_EXPECT(Ini.GetString("Values", "include", Value));
        TEST_EXPECT(Value == "1");
    }

    TEST_END();
}
