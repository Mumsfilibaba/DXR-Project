#include "EngineRegistryTests.h"

#include <Core/Misc/EngineDescriptor.h>
#include <Core/Misc/EngineRegistry.h>
#include <Core/Misc/Paths.h>
#include <Core/Misc/ProjectDescriptor.h>
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

static bool CreateEngineDir(const FString& Directory, const CHAR_T* Name)
{
    FEngineDescriptor Descriptor;
    Descriptor.Name = Name;
    return File::CreateDirectoryTree(Directory) && Descriptor.SaveToFile(File::CombinePath(Directory, Paths::EngineIdentityFilename));
}

bool EngineDescriptor_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("The engine the tests run from names itself in its dxrengine.json"));
    {
        FEngineDescriptor Descriptor;
        FString           Error;
        TEST_EXPECT(Descriptor.LoadFromFile(File::CombinePath(Paths::GetEngineDir(), Paths::EngineIdentityFilename), Error));
        TEST_EXPECT(Descriptor.Name.Equals(TEXT("DXR-Engine")));
    }

    const FString Root = Paths::GetUserSettingsDir() + TEXT("/Tests/EngineDescriptorTests.Scratch");
    File::DeleteDirectoryTree(Root);
    TEST_EXPECT(File::CreateDirectoryTree(Root));

    TEST_SECTION(TEXT("A descriptor round trips, with the version first"));
    {
        FEngineDescriptor Saved;
        Saved.Name = TEXT("My Engine");
        TEST_EXPECT(Saved.SaveToFile(Root + TEXT("/RoundTrip.json")));

        FEngineDescriptor Loaded;
        FString           Error;
        TEST_EXPECT(Loaded.LoadFromFile(Root + TEXT("/RoundTrip.json"), Error));
        TEST_EXPECT(Loaded.Name.Equals(TEXT("My Engine")));
        TEST_EXPECT(ReadText(Root + TEXT("/RoundTrip.json")).Find(TEXT("\"Version\"")) < ReadText(Root + TEXT("/RoundTrip.json")).Find(TEXT("\"Name\"")));
    }

    TEST_SECTION(TEXT("A descriptor without a name, without a version or from a newer engine fails and is left untouched"));
    {
        TEST_EXPECT(WriteText(Root + TEXT("/NoName.json"), TEXT("{ \"Version\": 1 }")));
        TEST_EXPECT(WriteText(Root + TEXT("/NoVersion.json"), TEXT("{ \"Name\": \"A\" }")));
        TEST_EXPECT(WriteText(Root + TEXT("/Newer.json"), TEXT("{ \"Version\": 2, \"Name\": \"A\" }")));

        FEngineDescriptor Descriptor;
        Descriptor.Name = TEXT("Untouched");

        FString Error;
        TEST_EXPECT(!Descriptor.LoadFromFile(Root + TEXT("/NoName.json"), Error) && Error.Contains(TEXT("has no Name")));
        TEST_EXPECT(!Descriptor.LoadFromFile(Root + TEXT("/NoVersion.json"), Error) && Error.Contains(TEXT("has no Version")));
        TEST_EXPECT(!Descriptor.LoadFromFile(Root + TEXT("/Newer.json"), Error) && Error.Contains(TEXT("version 2")));
        TEST_EXPECT(!Descriptor.LoadFromFile(Root + TEXT("/Missing.json"), Error));
        TEST_EXPECT(Descriptor.Name.Equals(TEXT("Untouched")));
    }

    TEST_EXPECT(File::DeleteDirectoryTree(Root));
    FPlatformFile::RemoveDirectory(*File::GetDirectoryOf(Root));

    TEST_END();
}

bool EngineRegistry_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("A path is absolute when it starts at a root"));
    {
        TEST_EXPECT(File::IsAbsolutePath(TEXT("/Engine")));
        TEST_EXPECT(!File::IsAbsolutePath(TEXT("Engine/Content")));
        TEST_EXPECT(!File::IsAbsolutePath(TEXT("../Engine")));
        TEST_EXPECT(!File::IsAbsolutePath(TEXT("")));
    #if PLATFORM_WINDOWS
        TEST_EXPECT(File::IsAbsolutePath(TEXT("C:/Engine")));
        TEST_EXPECT(File::IsAbsolutePath(TEXT("C:\\Engine")));
        TEST_EXPECT(File::IsAbsolutePath(TEXT("//Server/Share/Engine")));
        TEST_EXPECT(!File::IsAbsolutePath(TEXT("C:Engine")));
    #endif
    }

    const FString Root     = Paths::GetUserSettingsDir() + TEXT("/Tests/EngineRegistryTests.Scratch");
    const FString EngineA  = Root + TEXT("/Engines/Engine A");
    const FString EngineB  = Root + TEXT("/Engines/Engine B");
    const FString Replaced = Root + TEXT("/Engines/Replaced");
    const FString Project  = Root + TEXT("/Projects/Game");
    const FString Registry = Root + TEXT("/EngineRegistry.json");

    File::DeleteDirectoryTree(Root);
    TEST_EXPECT(CreateEngineDir(EngineA, TEXT("EngineA")));
    TEST_EXPECT(CreateEngineDir(EngineB, TEXT("EngineB")));
    TEST_EXPECT(CreateEngineDir(Replaced, TEXT("SomethingElse")));
    TEST_EXPECT(File::CreateDirectoryTree(Project));

    TEST_SECTION(TEXT("A missing registry file is an empty registry"));
    {
        FEngineRegistry Loaded;
        Loaded.Register(TEXT("Stale"), EngineA);

        FString Error;
        TEST_EXPECT(Loaded.LoadFromFile(Registry, Error));
        TEST_EXPECT(Loaded.Engines.IsEmpty());
    }

    TEST_SECTION(TEXT("Registering adds an engine in name order, and refreshes the folder of one already there"));
    {
        FEngineRegistry Engines;
        TEST_EXPECT(Engines.Register(TEXT("EngineB"), EngineB));
        TEST_EXPECT(Engines.Register(TEXT("EngineA"), Root + TEXT("/Old")));
        TEST_EXPECT_EQ(Engines.Engines.Size(), 2);
        TEST_EXPECT(Engines.Engines[0].Name.Equals(TEXT("EngineA")));
        TEST_EXPECT(Engines.Engines[1].Name.Equals(TEXT("EngineB")));

        TEST_EXPECT(Engines.Register(TEXT("EngineA"), EngineA));
        TEST_EXPECT(!Engines.Register(TEXT("EngineA"), EngineA));
        TEST_EXPECT(!Engines.Register(TEXT("EngineA"), EngineA + TEXT("/Content/..")));
        TEST_EXPECT_EQ(Engines.Engines.Size(), 2);
        TEST_EXPECT(Engines.Find(TEXT("EngineA")).Equals(EngineA));
        TEST_EXPECT(Engines.Find(TEXT("enginea")).Equals(EngineA));
        TEST_EXPECT(Engines.Find(TEXT("EngineC")).IsEmpty());
    }

    TEST_SECTION(TEXT("The registry round trips through a text file that only changes where an entry does"));
    {
        FEngineRegistry Saved;
        Saved.Register(TEXT("EngineB"), EngineB);
        Saved.Register(TEXT("EngineA"), EngineA);
        TEST_EXPECT(Saved.SaveToFile(Registry));

        const FString FirstText = ReadText(Registry);
        TEST_EXPECT(FirstText.Find(TEXT("\"Version\"")) < FirstText.Find(TEXT("\"Engines\"")));
        TEST_EXPECT(FirstText.Find(TEXT("EngineA")) < FirstText.Find(TEXT("EngineB")));
        TEST_EXPECT(!FirstText.Contains('\r'));

        FEngineRegistry Loaded;
        FString         Error;
        TEST_EXPECT(Loaded.LoadFromFile(Registry, Error));
        TEST_EXPECT_EQ(Loaded.Engines.Size(), 2);
        TEST_EXPECT(Loaded.Find(TEXT("EngineB")).Equals(EngineB));

        TEST_EXPECT(Loaded.SaveToFile(Registry));
        TEST_EXPECT(ReadText(Registry).Equals(FirstText));
    }

    TEST_SECTION(TEXT("A broken or newer registry file fails instead of being taken as empty"));
    {
        TEST_EXPECT(WriteText(Root + TEXT("/Broken.json"), TEXT("{ \"Version\": 1, \"Engines\": [")));
        TEST_EXPECT(WriteText(Root + TEXT("/Newer.json"), TEXT("{ \"Version\": 2, \"Engines\": [] }")));

        FEngineRegistry Loaded;
        FString         Error;
        TEST_EXPECT(!Loaded.LoadFromFile(Root + TEXT("/Broken.json"), Error));
        TEST_EXPECT(!Loaded.LoadFromFile(Root + TEXT("/Newer.json"), Error) && Error.Contains(TEXT("version 2")));
    }

    TEST_SECTION(TEXT("A project's engine is the folder its association is registered under"));
    {
        FEngineRegistry Engines;
        Engines.Register(TEXT("EngineA"), EngineA);

        FProjectDescriptor Descriptor;
        Descriptor.Name              = TEXT("Game");
        Descriptor.EngineAssociation = TEXT("EngineA");
        Descriptor.EnginePath        = TEXT("../../Engines/Engine B");
        TEST_EXPECT(Engines.ResolveProjectEngine(Descriptor, Project).Equals(EngineA));
    }

    TEST_SECTION(TEXT("An association that is unknown, or registered to a folder that no longer is that engine, falls back to EnginePath"));
    {
        FEngineRegistry Engines;
        Engines.Register(TEXT("EngineA"), Replaced);
        Engines.Register(TEXT("Gone"), Root + TEXT("/Engines/Gone"));

        FProjectDescriptor Descriptor;
        Descriptor.Name       = TEXT("Game");
        Descriptor.EnginePath = TEXT("../../Engines/Engine B");

        Descriptor.EngineAssociation = TEXT("EngineA");
        TEST_EXPECT(Engines.ResolveProjectEngine(Descriptor, Project).Equals(EngineB));

        Descriptor.EngineAssociation = TEXT("Gone");
        TEST_EXPECT(Engines.ResolveProjectEngine(Descriptor, Project).Equals(EngineB));

        Descriptor.EngineAssociation = TEXT("Unknown");
        TEST_EXPECT(Engines.ResolveProjectEngine(Descriptor, Project).Equals(EngineB));

        Descriptor.EnginePath = EngineB;
        TEST_EXPECT(Engines.ResolveProjectEngine(Descriptor, Project).Equals(EngineB));
    }

    TEST_SECTION(TEXT("Neither leading to an engine folder resolves to nothing"));
    {
        FEngineRegistry    Engines;
        FProjectDescriptor Descriptor;
        Descriptor.Name = TEXT("Game");
        TEST_EXPECT(Engines.ResolveProjectEngine(Descriptor, Project).IsEmpty());

        Descriptor.EngineAssociation = TEXT("EngineA");
        Descriptor.EnginePath        = TEXT("../Nowhere");
        TEST_EXPECT(Engines.ResolveProjectEngine(Descriptor, Project).IsEmpty());
    }

    TEST_EXPECT(File::DeleteDirectoryTree(Root));
    FPlatformFile::RemoveDirectory(*File::GetDirectoryOf(Root));

    TEST_END();
}
