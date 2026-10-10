#include "ProjectDescriptorTests.h"

#include <Core/Misc/ProjectDescriptor.h>
#include <Core/Filesystem/File.h>
#include <Core/Containers/String.h>
#include <Core/Platform/PlatformFile.h>

#include "TestCommon/TestMacros.h"

static bool WriteText(const String& Filename, const CHAR* Text)
{
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(Filename);
    if (!FileHandle)
    {
        return false;
    }

    return File::WriteTextFile(FileHandle.Get(), String(Text));
}

static String ReadText(const String& Filename)
{
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(Filename);
    if (!FileHandle)
    {
        return String();
    }

    TArray<CHAR> Text;
    if (!File::ReadTextFile(FileHandle.Get(), Text))
    {
        return String();
    }

    return String(Text.Data());
}

static FProjectDescriptor CreateDescriptor()
{
    FProjectDescriptor Descriptor;
    Descriptor.Name              = "MyGame";
    Descriptor.EngineAssociation = "1.0";

    FProjectModuleDescriptor& GameModule = Descriptor.Modules.Emplace();
    GameModule.Name = "MyGame";

    FProjectModuleDescriptor& EditorModule = Descriptor.Modules.Emplace();
    EditorModule.Name = "MyGameEditor";
    EditorModule.Type = EProjectModuleType::Editor;
    return Descriptor;
}

static bool FailsToLoad(const String& Filename, const CHAR* Text)
{
    if (!WriteText(Filename, Text))
    {
        return false;
    }

    FProjectDescriptor Descriptor = CreateDescriptor();

    String Error;
    const bool bLoaded = Descriptor.LoadFromFile(Filename, Error);
    return !bLoaded && !Error.IsEmpty() && Descriptor.Name.Equals("MyGame") && (Descriptor.Modules.Size() == 2);
}

bool ProjectDescriptor_Test()
{
    TEST_BEGIN();

    const String Root("ProjectDescriptorTests.Scratch");
    File::DeleteDirectoryTree(Root);
    TEST_EXPECT(File::CreateDirectoryTree(Root));

    TEST_SECTION("A descriptor round trips through a file");
    {
        const String Filename = Root + "/MyGame.dxrproject";
        const FProjectDescriptor Source = CreateDescriptor();
        TEST_EXPECT(Source.SaveToFile(Filename));

        FProjectDescriptor Loaded;
        String Error;
        TEST_EXPECT(Loaded.LoadFromFile(Filename, Error));
        TEST_EXPECT(Error.IsEmpty());
        TEST_EXPECT(Loaded.Name.Equals("MyGame"));
        TEST_EXPECT(Loaded.EngineAssociation.Equals("1.0"));
        TEST_EXPECT(Loaded.EnginePath.IsEmpty());
        TEST_EXPECT(Loaded.Modules.Size() == 2);
        TEST_EXPECT(Loaded.Modules[0].Name.Equals("MyGame"));
        TEST_EXPECT(Loaded.Modules[0].Type == EProjectModuleType::Game);
        TEST_EXPECT(Loaded.Modules[1].Name.Equals("MyGameEditor"));
        TEST_EXPECT(Loaded.Modules[1].Type == EProjectModuleType::Editor);
    }

    TEST_SECTION("The file follows the text format rules");
    {
        const String Filename = Root + "/MyGame.dxrproject";
        const String Text     = ReadText(Filename);

        TEST_EXPECT(Text.StartsWith("{"));
        TEST_EXPECT(Text.EndsWith("}\n"));
        TEST_EXPECT(!Text.Contains('\r'));

        const int32 VersionPosition = Text.Find("\"Version\": 1");
        TEST_EXPECT(VersionPosition != String::InvalidIndex);
        TEST_EXPECT(VersionPosition < Text.Find("\"Name\""));

        TEST_EXPECT(Text.Find("\"Type\": \"Editor\"") != String::InvalidIndex);
        TEST_EXPECT(Text.Find("\"Type\": \"Game\"") == String::InvalidIndex);
        TEST_EXPECT(Text.Find("EnginePath") == String::InvalidIndex);
    }

    TEST_SECTION("Saving an unchanged descriptor writes identical bytes");
    {
        const String First  = Root + "/First.dxrproject";
        const String Second = Root + "/Second.dxrproject";

        FProjectDescriptor Descriptor = CreateDescriptor();
        Descriptor.EnginePath = "../DXR-Project";
        TEST_EXPECT(Descriptor.SaveToFile(First));

        FProjectDescriptor Reloaded;
        String Error;
        TEST_EXPECT(Reloaded.LoadFromFile(First, Error));
        TEST_EXPECT(Reloaded.SaveToFile(Second));

        const String FirstText = ReadText(First);
        TEST_EXPECT(!FirstText.IsEmpty());
        TEST_EXPECT(FirstText.Equals(ReadText(Second)));
        TEST_EXPECT(FirstText.Find("\"EnginePath\": \"../DXR-Project\"") != String::InvalidIndex);
    }

    TEST_SECTION("A missing file fails and names the file");
    {
        FProjectDescriptor Descriptor;
        String Error;
        TEST_EXPECT(!Descriptor.LoadFromFile(Root + "/Missing.dxrproject", Error));
        TEST_EXPECT(Error.Find("Missing.dxrproject") != String::InvalidIndex);
    }

    TEST_SECTION("Broken or unsupported files fail and leave the descriptor untouched");
    {
        const String Filename = Root + "/Broken.dxrproject";
        TEST_EXPECT(FailsToLoad(Filename, "{ \"Version\": 1, \"Name\": "));
        TEST_EXPECT(FailsToLoad(Filename, "{ \"Name\": \"MyGame\" }"));
        TEST_EXPECT(FailsToLoad(Filename, "{ \"Version\": 2, \"Name\": \"MyGame\" }"));
        TEST_EXPECT(FailsToLoad(Filename, "{ \"Version\": 1 }"));
        TEST_EXPECT(FailsToLoad(Filename, "{ \"Version\": 1, \"Name\": \"MyGame\", \"Modules\": 5 }"));
        TEST_EXPECT(FailsToLoad(Filename, "{ \"Version\": 1, \"Name\": \"MyGame\", "
            "\"Modules\": [ { \"Name\": \"A\", \"Type\": \"Plugin\" } ] }"));
    }

    TEST_SECTION("A field with the wrong type is reported by its path");
    {
        const String Filename = Root + "/WrongType.dxrproject";
        TEST_EXPECT(WriteText(Filename, "{ \"Version\": 1, \"Name\": \"MyGame\", \"Modules\": [ { \"Name\": 7 } ] }"));

        FProjectDescriptor Descriptor;
        String Error;
        TEST_EXPECT(!Descriptor.LoadFromFile(Filename, Error));
        TEST_EXPECT(Error.Find("Modules[0].Name") != String::InvalidIndex);
    }

    TEST_EXPECT(File::DeleteDirectoryTree(Root));

    TEST_END();
}
