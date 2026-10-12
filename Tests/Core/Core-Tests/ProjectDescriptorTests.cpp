#include "ProjectDescriptorTests.h"

#include <Core/Misc/ProjectDescriptor.h>
#include <Core/Misc/Paths.h>
#include <Core/Filesystem/File.h>
#include <Core/Containers/String.h>
#include <Core/Platform/PlatformFile.h>

#include "TestCommon/TestMacros.h"

static bool WriteText(const FString& Filename, const CHAR_T* Text)
{
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(Filename);
    if (!FileHandle)
    {
        return false;
    }

    return File::WriteTextFile(FileHandle.Get(), FString(Text));
}

static FString ReadText(const FString& Filename)
{
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(Filename);
    if (!FileHandle)
    {
        return FString();
    }

    TArray<CHAR_T> Text;
    if (!File::ReadTextFile(FileHandle.Get(), Text))
    {
        return FString();
    }

    return FString(Text.Data());
}

static FProjectDescriptor CreateDescriptor()
{
    FProjectDescriptor Descriptor;
    Descriptor.Name              = TEXT("MyGame");
    Descriptor.EngineAssociation = TEXT("1.0");

    FProjectModuleDescriptor& GameModule = Descriptor.Modules.Emplace();
    GameModule.Name = TEXT("MyGame");

    FProjectModuleDescriptor& EditorModule = Descriptor.Modules.Emplace();
    EditorModule.Name = TEXT("MyGameEditor");
    EditorModule.Type = EProjectModuleType::Editor;
    return Descriptor;
}

static bool FailsToLoad(const FString& Filename, const CHAR_T* Text)
{
    if (!WriteText(Filename, Text))
    {
        return false;
    }

    FProjectDescriptor Descriptor = CreateDescriptor();

    FString Error;
    const bool bLoaded = Descriptor.LoadFromFile(Filename, Error);
    return !bLoaded && !Error.IsEmpty() && Descriptor.Name.Equals(TEXT("MyGame")) && (Descriptor.Modules.Size() == 2);
}

bool ProjectDescriptor_Test()
{
    TEST_BEGIN();

    const FString Root(TEXT("ProjectDescriptorTests.Scratch"));
    File::DeleteDirectoryTree(Root);
    TEST_EXPECT(File::CreateDirectoryTree(Root));

    TEST_SECTION(TEXT("A descriptor round trips through a file"));
    {
        const FString Filename = Root + TEXT("/MyGame.dxrproject");
        const FProjectDescriptor Source = CreateDescriptor();
        TEST_EXPECT(Source.SaveToFile(Filename));

        FProjectDescriptor Loaded;
        FString Error;
        TEST_EXPECT(Loaded.LoadFromFile(Filename, Error));
        TEST_EXPECT(Error.IsEmpty());
        TEST_EXPECT(Loaded.Name.Equals(TEXT("MyGame")));
        TEST_EXPECT(Loaded.EngineAssociation.Equals(TEXT("1.0")));
        TEST_EXPECT(Loaded.EnginePath.IsEmpty());
        TEST_EXPECT(Loaded.Modules.Size() == 2);
        TEST_EXPECT(Loaded.Modules[0].Name.Equals(TEXT("MyGame")));
        TEST_EXPECT(Loaded.Modules[0].Type == EProjectModuleType::Game);
        TEST_EXPECT(Loaded.Modules[1].Name.Equals(TEXT("MyGameEditor")));
        TEST_EXPECT(Loaded.Modules[1].Type == EProjectModuleType::Editor);
    }

    TEST_SECTION(TEXT("The file follows the text format rules"));
    {
        const FString Filename = Root + TEXT("/MyGame.dxrproject");
        const FString Text     = ReadText(Filename);

        TEST_EXPECT(Text.StartsWith(TEXT("{")));
        TEST_EXPECT(Text.EndsWith(TEXT("}\n")));
        TEST_EXPECT(!Text.Contains('\r'));

        const int32 VersionPosition = Text.Find(TEXT("\"Version\": 1"));
        TEST_EXPECT(VersionPosition != FString::InvalidIndex);
        TEST_EXPECT(VersionPosition < Text.Find(TEXT("\"Name\"")));

        TEST_EXPECT(Text.Find(TEXT("\"Type\": \"Editor\"")) != FString::InvalidIndex);
        TEST_EXPECT(Text.Find(TEXT("\"Type\": \"Game\"")) == FString::InvalidIndex);
        TEST_EXPECT(Text.Find(TEXT("EnginePath")) == FString::InvalidIndex);
    }

    TEST_SECTION(TEXT("Saving an unchanged descriptor writes identical bytes"));
    {
        const FString First  = Root + TEXT("/First.dxrproject");
        const FString Second = Root + TEXT("/Second.dxrproject");

        FProjectDescriptor Descriptor = CreateDescriptor();
        Descriptor.EnginePath = TEXT("../DXR-Project");
        TEST_EXPECT(Descriptor.SaveToFile(First));

        FProjectDescriptor Reloaded;
        FString Error;
        TEST_EXPECT(Reloaded.LoadFromFile(First, Error));
        TEST_EXPECT(Reloaded.SaveToFile(Second));

        const FString FirstText = ReadText(First);
        TEST_EXPECT(!FirstText.IsEmpty());
        TEST_EXPECT(FirstText.Equals(ReadText(Second)));
        TEST_EXPECT(FirstText.Find(TEXT("\"EnginePath\": \"../DXR-Project\"")) != FString::InvalidIndex);
    }

    TEST_SECTION(TEXT("Sandbox's descriptor loads and is written the way the engine writes it"));
    {
        const FString SandboxFile = Paths::GetEngineDir() + TEXT("/Sandbox/Sandbox.dxrproject");
        const FString Resaved     = Root + TEXT("/Sandbox.dxrproject");

        FProjectDescriptor Descriptor;
        FString Error;
        TEST_EXPECT(Descriptor.LoadFromFile(SandboxFile, Error));
        TEST_EXPECT(Descriptor.Name.Equals(TEXT("Sandbox")));
        TEST_EXPECT(Descriptor.EngineAssociation.Equals(TEXT("DXR-Engine")));
        TEST_EXPECT(Descriptor.Modules.Size() == 1 && Descriptor.Modules[0].Name.Equals(TEXT("Sandbox")));

        TEST_EXPECT(Descriptor.SaveToFile(Resaved));
        TEST_EXPECT(ReadText(Resaved).Equals(ReadText(SandboxFile)));
    }

    TEST_SECTION(TEXT("A missing file fails and names the file"));
    {
        FProjectDescriptor Descriptor;
        FString Error;
        TEST_EXPECT(!Descriptor.LoadFromFile(Root + TEXT("/Missing.dxrproject"), Error));
        TEST_EXPECT(Error.Find(TEXT("Missing.dxrproject")) != FString::InvalidIndex);
    }

    TEST_SECTION(TEXT("Broken or unsupported files fail and leave the descriptor untouched"));
    {
        const FString Filename = Root + TEXT("/Broken.dxrproject");
        TEST_EXPECT(FailsToLoad(Filename, TEXT("{ \"Version\": 1, \"Name\": ")));
        TEST_EXPECT(FailsToLoad(Filename, TEXT("{ \"Name\": \"MyGame\" }")));
        TEST_EXPECT(FailsToLoad(Filename, TEXT("{ \"Version\": 2, \"Name\": \"MyGame\" }")));
        TEST_EXPECT(FailsToLoad(Filename, TEXT("{ \"Version\": 1 }")));
        TEST_EXPECT(FailsToLoad(Filename, TEXT("{ \"Version\": 1, \"Name\": \"MyGame\", \"Modules\": 5 }")));
        TEST_EXPECT(FailsToLoad(Filename, TEXT("{ \"Version\": 1, \"Name\": \"MyGame\", "
            "\"Modules\": [ { \"Name\": \"A\", \"Type\": \"Plugin\" } ] }")));
    }

    TEST_SECTION(TEXT("A field with the wrong type is reported by its path"));
    {
        const FString Filename = Root + TEXT("/WrongType.dxrproject");
        TEST_EXPECT(WriteText(Filename, TEXT("{ \"Version\": 1, \"Name\": \"MyGame\", \"Modules\": [ { \"Name\": 7 } ] }")));

        FProjectDescriptor Descriptor;
        FString Error;
        TEST_EXPECT(!Descriptor.LoadFromFile(Filename, Error));
        TEST_EXPECT(Error.Find(TEXT("Modules[0].Name")) != FString::InvalidIndex);
    }

    TEST_EXPECT(File::DeleteDirectoryTree(Root));

    TEST_END();
}
