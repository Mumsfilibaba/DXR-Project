#include "PathsTests.h"

#include <Core/Misc/Paths.h>
#include <Core/Filesystem/File.h>
#include <Core/Containers/String.h>
#include <Core/Platform/PlatformFile.h>
#include <Core/Templates/CString.h>

#include "TestCommon/TestMacros.h"

static bool WriteText(const String& Filename, const CHAR_T* Text)
{
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(Filename);
    if (!FileHandle)
    {
        return false;
    }

    return File::WriteTextFile(FileHandle.Get(), String(Text));
}

bool Paths_Test()
{
    TEST_BEGIN();

    TEST_SECTION("The engine directory of the running tests is the engine folder they were built in");
    {
        const String EngineDir     = Paths::GetEngineDir();
        const String ExecutableDir = File::GetDirectoryOf(FPlatformFile::GetExecutablePath());
        TEST_EXPECT(FPlatformFile::IsFile(*File::CombinePath(EngineDir, Paths::EngineIdentityFilename)));
        TEST_EXPECT(File::IsUnderDirectory(ExecutableDir, EngineDir));
        TEST_EXPECT(Paths::GetAssetDir().Equals(EngineDir + "/Assets"));
        TEST_EXPECT(Paths::GetEngineDirSource() == EEngineDirSource::ExecutableLocation);
        TEST_EXPECT(CString::Strchr(FPlatformFile::GetExecutablePath(), '\\') == nullptr);
        TEST_EXPECT(!FPlatformFile::GetCurrentWorkingDirectory().Contains('\\'));
        TEST_EXPECT(!EngineDir.Contains('\\'));
        TEST_EXPECT(CString::Strcmp(Paths::GetEngineDirSourceName(), "found above the executable") == 0);
    }

    // Outside the engine folder, so walking up never reaches the real engine
    const String Root   = Paths::GetUserSettingsDir() + "/Tests/PathsTests.Scratch";
    const String Engine = Root + "/Engine";
    const String Inside = Engine + "/Build/bin/Debug";
    const String Other  = Root + "/Other/bin";

    File::DeleteDirectoryTree(Root);
    TEST_EXPECT(File::CreateDirectoryTree(Inside));
    TEST_EXPECT(File::CreateDirectoryTree(Other));
    TEST_EXPECT(WriteText(File::CombinePath(Engine, Paths::EngineIdentityFilename), "{ \"Version\": 1, \"Name\": \"Test\" }"));

    TEST_SECTION("An executable inside an engine folder finds it by walking up");
    {
        EEngineDirSource Source = EEngineDirSource::None;
        TEST_EXPECT(Paths::FindEngineDir("", Inside, Root + "/Missing", &Source).Equals(Engine));
        TEST_EXPECT(Source == EEngineDirSource::ExecutableLocation);
        TEST_EXPECT(Paths::FindEngineDir("", Engine, "").Equals(Engine));
    }

    TEST_SECTION("-engine= wins, and an -engine= that is not an engine folder fails instead of falling through");
    {
        EEngineDirSource Source = EEngineDirSource::None;
        TEST_EXPECT(Paths::FindEngineDir(Engine, Other, "", &Source).Equals(Engine));
        TEST_EXPECT(Source == EEngineDirSource::CommandLine);
        TEST_EXPECT(Paths::FindEngineDir(Engine + "/Build/../", Other, "").Equals(Engine));
        TEST_EXPECT(Paths::FindEngineDir(Other, Inside, Engine, &Source).IsEmpty());
        TEST_EXPECT(Source == EEngineDirSource::None);
    }

    TEST_SECTION("An executable outside any engine folder uses the fallback, if it is one");
    {
        EEngineDirSource Source = EEngineDirSource::None;
        TEST_EXPECT(Paths::FindEngineDir("", Other, Engine, &Source).Equals(Engine));
        TEST_EXPECT(Source == EEngineDirSource::CompileTimeFallback);
        TEST_EXPECT(Paths::FindEngineDir("", Other, Root + "/Missing", &Source).IsEmpty());
        TEST_EXPECT(Source == EEngineDirSource::None);
        TEST_EXPECT(Paths::FindEngineDir("", Other, "").IsEmpty());
    }

    TEST_EXPECT(File::DeleteDirectoryTree(Root));
    FPlatformFile::RemoveDirectory(*File::GetDirectoryOf(Root));

    TEST_END();
}
