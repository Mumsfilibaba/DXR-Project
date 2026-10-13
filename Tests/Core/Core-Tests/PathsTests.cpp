#include "PathsTests.h"

#include <Core/Misc/CommandLine.h>
#include <Core/Misc/Paths.h>
#include <Core/Misc/ProjectDescriptor.h>
#include <Core/Filesystem/File.h>
#include <Core/Containers/String.h>
#include <Core/Platform/PlatformFile.h>
#include <Core/Templates/CString.h>

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

bool Paths_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("The engine directory of the running tests is the engine folder they were built in"));
    {
        const FString EngineDir     = Paths::GetEngineDir();
        const FString ExecutableDir = File::GetDirectoryOf(FPlatformFile::GetExecutablePath());
        TEST_EXPECT(FPlatformFile::IsFile(*File::CombinePath(EngineDir, Paths::EngineIdentityFilename)));
        TEST_EXPECT(File::IsUnderDirectory(ExecutableDir, EngineDir));
        TEST_EXPECT(Paths::GetAssetDir().Equals(EngineDir + TEXT("/Assets")));
        TEST_EXPECT(Paths::GetEngineDirSource() == EEngineDirSource::ExecutableLocation);
        TEST_EXPECT(CString::Strchr(FPlatformFile::GetExecutablePath(), '\\') == nullptr);
        TEST_EXPECT(!FPlatformFile::GetCurrentWorkingDirectory().Contains('\\'));
        TEST_EXPECT(!EngineDir.Contains('\\'));
        TEST_EXPECT(CString::Strcmp(Paths::GetEngineDirSourceName(), TEXT("found above the executable")) == 0);
    }

    // Outside the engine folder, so walking up never reaches the real engine
    const FString Root   = Paths::GetUserSettingsDir() + TEXT("/Tests/PathsTests.Scratch");
    const FString Engine = Root + TEXT("/Engine");
    const FString Inside = Engine + TEXT("/Build/bin/Debug");
    const FString Other  = Root + TEXT("/Other/bin");

    File::DeleteDirectoryTree(Root);
    TEST_EXPECT(File::CreateDirectoryTree(Inside));
    TEST_EXPECT(File::CreateDirectoryTree(Other));
    TEST_EXPECT(WriteText(File::CombinePath(Engine, Paths::EngineIdentityFilename), TEXT("{ \"Version\": 1, \"Name\": \"Test\" }")));

    TEST_SECTION(TEXT("An executable inside an engine folder finds it by walking up"));
    {
        EEngineDirSource Source = EEngineDirSource::None;
        TEST_EXPECT(Paths::FindEngineDir(TEXT(""), Inside, Root + TEXT("/Missing"), &Source).Equals(Engine));
        TEST_EXPECT(Source == EEngineDirSource::ExecutableLocation);
        TEST_EXPECT(Paths::FindEngineDir(TEXT(""), Engine, TEXT("")).Equals(Engine));
    }

    TEST_SECTION(TEXT("-engine= wins, and an -engine= that is not an engine folder fails instead of falling through"));
    {
        EEngineDirSource Source = EEngineDirSource::None;
        TEST_EXPECT(Paths::FindEngineDir(Engine, Other, TEXT(""), &Source).Equals(Engine));
        TEST_EXPECT(Source == EEngineDirSource::CommandLine);
        TEST_EXPECT(Paths::FindEngineDir(Engine + TEXT("/Build/../"), Other, TEXT("")).Equals(Engine));
        TEST_EXPECT(Paths::FindEngineDir(Other, Inside, Engine, &Source).IsEmpty());
        TEST_EXPECT(Source == EEngineDirSource::None);
    }

    TEST_SECTION(TEXT("An executable outside any engine folder uses the fallback, if it is one"));
    {
        EEngineDirSource Source = EEngineDirSource::None;
        TEST_EXPECT(Paths::FindEngineDir(TEXT(""), Other, Engine, &Source).Equals(Engine));
        TEST_EXPECT(Source == EEngineDirSource::CompileTimeFallback);
        TEST_EXPECT(Paths::FindEngineDir(TEXT(""), Other, Root + TEXT("/Missing"), &Source).IsEmpty());
        TEST_EXPECT(Source == EEngineDirSource::None);
        TEST_EXPECT(Paths::FindEngineDir(TEXT(""), Other, TEXT("")).IsEmpty());
    }

    TEST_EXPECT(File::DeleteDirectoryTree(Root));
    FPlatformFile::RemoveDirectory(*File::GetDirectoryOf(Root));

    TEST_END();
}

static bool InitializeProjectWith(const CHAR_T* const* Args, int32 NumArgs, FString& OutError)
{
    CommandLine::Initialize(const_cast<const CHAR_T**>(Args), NumArgs);
    return Paths::InitializeProject(OutError);
}

bool ProjectPaths_Test()
{
    TEST_BEGIN();

    const FString CompileTimeDir  = Paths::GetProjectDir();
    const FString CompileTimeName = Paths::GetProjectName();

    TEST_SECTION(TEXT("Without a descriptor the project is the compile-time one"));
    {
        FString Error;
        TEST_EXPECT(InitializeProjectWith(nullptr, 0, Error));
        TEST_EXPECT(Paths::GetProjectSource() == EProjectSource::None);
        TEST_EXPECT(Paths::GetProjectFile().IsEmpty());
        TEST_EXPECT(Paths::GetProjectDescriptor() == nullptr);
        TEST_EXPECT(!CompileTimeDir.IsEmpty() && Paths::GetProjectDir().Equals(CompileTimeDir));
        TEST_EXPECT(!CompileTimeName.IsEmpty() && Paths::GetProjectName().Equals(CompileTimeName));
        TEST_EXPECT(Paths::GetProjectModuleName().Equals(CompileTimeName));
        TEST_EXPECT(!CompileTimeDir.Contains('\\'));
    }

    TEST_SECTION(TEXT("The content, config and saved directories sit under the project and the engine"));
    {
        TEST_EXPECT(Paths::GetProjectContentDir().Equals(Paths::GetProjectDir() + TEXT("/Content")));
        TEST_EXPECT(Paths::GetProjectConfigDir().Equals(Paths::GetProjectDir() + TEXT("/Config")));
        TEST_EXPECT(Paths::GetProjectCacheDir().Equals(Paths::GetProjectDir() + TEXT("/Cache")));
        TEST_EXPECT(Paths::GetProjectLogsDir().Equals(Paths::GetProjectDir() + TEXT("/Logs")));
        TEST_EXPECT(Paths::GetProjectUserDir().Equals(Paths::GetProjectDir() + TEXT("/User")));
        TEST_EXPECT(Paths::GetEngineContentDir().Equals(Paths::GetEngineDir() + TEXT("/Content")));
        TEST_EXPECT(Paths::GetEngineCacheDir().Equals(Paths::GetEngineDir() + TEXT("/Cache")));
    }

    TEST_SECTION(TEXT("Outside a package, logs, user state and caches are written in the project"));
    {
        TEST_EXPECT(!Paths::IsPackaged());
        TEST_EXPECT(Paths::GetWritableDir().Equals(Paths::GetProjectDir()));
        TEST_EXPECT(Paths::ResolveOutputPath(TEXT("Dumps/Dump.txt")).Equals(Paths::GetProjectLogsDir() + TEXT("/Dumps/Dump.txt")));
        TEST_EXPECT(Paths::ResolveOutputPath(TEXT("C:/Dumps/Dump.txt")).Equals(TEXT("C:/Dumps/Dump.txt")));
        TEST_EXPECT(Paths::ResolveOutputPath(TEXT("")).IsEmpty());
    }

    TEST_SECTION(TEXT("A packaged game writes to a folder named after it in the per-user settings directory"));
    {
        const FString Project = TEXT("D:/Package/Game");
        const FString User    = TEXT("C:/Users/Someone/AppData/Local/DXR-Engine");
        TEST_EXPECT(Paths::FindWritableDir(Project, TEXT("My Game"), false, User).Equals(Project));
        TEST_EXPECT(Paths::FindWritableDir(Project, TEXT("My Game"), true, User).Equals(User + TEXT("/My Game")));
        TEST_EXPECT(Paths::FindWritableDir(Project, TEXT("My Game"), true, TEXT("")).Equals(Project));
    }

    const FString Root        = Paths::GetUserSettingsDir() + TEXT("/Tests/ProjectPathsTests.Scratch");
    const FString Game        = Root + TEXT("/My Projects/Game");
    const FString GameFile    = Game + TEXT("/Game.dxrproject");
    const FString Empty       = Root + TEXT("/Empty");
    const FString Package     = Root + TEXT("/Package");
    const FString Packaged    = Package + TEXT("/Packaged Game/Packaged Game.dxrproject");
    const FString Broken      = Root + TEXT("/Broken/Broken.dxrproject");
    const FString Coded       = Root + TEXT("/Coded/Coded.dxrproject");
    const FString NotAProject = Root + TEXT("/Notes.txt");

    File::DeleteDirectoryTree(Root);
    TEST_EXPECT(File::CreateDirectoryTree(Game));
    TEST_EXPECT(File::CreateDirectoryTree(Empty));
    TEST_EXPECT(File::CreateDirectoryTree(File::GetDirectoryOf(Packaged)));
    TEST_EXPECT(File::CreateDirectoryTree(File::GetDirectoryOf(Broken)));
    TEST_EXPECT(File::CreateDirectoryTree(File::GetDirectoryOf(Coded)));
    TEST_EXPECT(WriteText(GameFile, TEXT("{ \"Version\": 1, \"Name\": \"Game\" }")));
    TEST_EXPECT(WriteText(Packaged, TEXT("{ \"Version\": 1, \"Name\": \"Packaged Game\" }")));
    TEST_EXPECT(WriteText(Broken, TEXT("{ \"Version\": 1 }")));
    TEST_EXPECT(WriteText(Coded, TEXT("{ \"Version\": 1, \"Name\": \"Coded\", \"Modules\": [ ")
        TEXT("{ \"Name\": \"CodedEditor\", \"Type\": \"Editor\" }, { \"Name\": \"CodedGame\" }, { \"Name\": \"CodedExtra\" } ] }")));
    TEST_EXPECT(WriteText(NotAProject, TEXT("Not a project")));
    TEST_EXPECT(WriteText(File::CombinePath(Package, Paths::PackageFilename), TEXT("[Package]\nProject=\"Packaged Game/Packaged Game.dxrproject\"\n")));

    TEST_SECTION(TEXT("-project= takes a descriptor, or the folder that holds one named after it"));
    {
        EProjectSource Source = EProjectSource::None;
        TEST_EXPECT(Paths::FindProjectFile(GameFile, Empty, TEXT(""), &Source).Equals(GameFile));
        TEST_EXPECT(Source == EProjectSource::CommandLine);
        TEST_EXPECT(Paths::FindProjectFile(Game, Empty, TEXT("")).Equals(GameFile));
        TEST_EXPECT(Paths::FindProjectFile(Game + TEXT("/../Game/"), Empty, TEXT("")).Equals(GameFile));
    }

    TEST_SECTION(TEXT("-project= that is not a project finds nothing, instead of falling through"));
    {
        EProjectSource Source = EProjectSource::CommandLine;
        TEST_EXPECT(Paths::FindProjectFile(Empty, Empty, GameFile, &Source).IsEmpty());
        TEST_EXPECT(Source == EProjectSource::None);
        TEST_EXPECT(Paths::FindProjectFile(NotAProject, Empty, GameFile).IsEmpty());
        TEST_EXPECT(Paths::FindProjectFile(Root + TEXT("/Missing.dxrproject"), Empty, GameFile).IsEmpty());
    }

    TEST_SECTION(TEXT("A packaged layout uses the project its Package.ini names, and never the fallback"));
    {
        EProjectSource Source = EProjectSource::None;
        TEST_EXPECT(Paths::FindProjectFile(TEXT(""), Package, GameFile, &Source).Equals(Packaged));
        TEST_EXPECT(Source == EProjectSource::PackageFile);

        TEST_EXPECT(WriteText(File::CombinePath(Package, Paths::PackageFilename), TEXT("[Package]\nProject=Missing/Missing.dxrproject\n")));
        TEST_EXPECT(Paths::FindProjectFile(TEXT(""), Package, GameFile, &Source).IsEmpty());
        TEST_EXPECT(Source == EProjectSource::None);

        TEST_SECTION(TEXT("-project= wins over Package.ini"));
        TEST_EXPECT(Paths::FindProjectFile(GameFile, Package, TEXT(""), &Source).Equals(GameFile));
        TEST_EXPECT(Source == EProjectSource::CommandLine);
    }

    TEST_SECTION(TEXT("Otherwise the compile-time descriptor is used when it exists"));
    {
        EProjectSource Source = EProjectSource::None;
        TEST_EXPECT(Paths::FindProjectFile(TEXT(""), Empty, GameFile, &Source).Equals(GameFile));
        TEST_EXPECT(Source == EProjectSource::CompileTimeFallback);
        TEST_EXPECT(Paths::FindProjectFile(TEXT(""), Empty, Root + TEXT("/Missing.dxrproject"), &Source).IsEmpty());
        TEST_EXPECT(Source == EProjectSource::None);
    }

    TEST_SECTION(TEXT("-project= loads the descriptor, which then names and places the project"));
    {
        const FString ProjectArgument = FString(TEXT("-project=")) + Game;
        const CHAR_T* Args[]          = { *ProjectArgument };

        FString Error;
        TEST_EXPECT(InitializeProjectWith(Args, 1, Error));
        TEST_EXPECT(Error.IsEmpty());
        TEST_EXPECT(Paths::GetProjectSource() == EProjectSource::CommandLine);
        TEST_EXPECT(Paths::GetProjectFile().Equals(GameFile));
        TEST_EXPECT(Paths::GetProjectDir().Equals(Game));
        TEST_EXPECT(Paths::GetProjectName().Equals(TEXT("Game")));
        TEST_EXPECT(Paths::GetProjectDescriptor() != nullptr && Paths::GetProjectDescriptor()->Name.Equals(TEXT("Game")));
        TEST_EXPECT(Paths::GetProjectContentDir().Equals(Game + TEXT("/Content")));
        TEST_EXPECT(!Paths::IsPackaged() && Paths::GetWritableDir().Equals(Game));
    }

    TEST_SECTION(TEXT("Next to a Package.ini, the project's and the engine's writes go to the per-user folder"));
    {
        const FString ExecutableDir     = File::GetDirectoryOf(FPlatformFile::GetExecutablePath());
        const FString ExecutablePackage = File::CombinePath(ExecutableDir, Paths::PackageFilename);
        const FString PackagedDir       = File::CombinePath(ExecutableDir, TEXT("ProjectPathsTests.Packaged"));

        TEST_EXPECT(!FPlatformFile::IsFile(*ExecutablePackage));
        TEST_EXPECT(File::CreateDirectoryTree(PackagedDir));
        TEST_EXPECT(WriteText(PackagedDir + TEXT("/Packaged.dxrproject"), TEXT("{ \"Version\": 1, \"Name\": \"Packaged\" }")));
        TEST_EXPECT(WriteText(ExecutablePackage, TEXT("[Package]\nProject=\"ProjectPathsTests.Packaged/Packaged.dxrproject\"\n")));

        FString Error;
        const bool bInitialized = InitializeProjectWith(nullptr, 0, Error);
        FPlatformFile::DeleteFile(*ExecutablePackage);

        const FString Expected = Paths::GetUserSettingsDir() + TEXT("/Packaged");
        TEST_EXPECT(bInitialized && Paths::IsPackaged());
        TEST_EXPECT(Paths::GetProjectDir().Equals(File::NormalizePath(PackagedDir)));
        TEST_EXPECT(Paths::GetWritableDir().Equals(Expected));
        TEST_EXPECT(Paths::GetProjectLogsDir().Equals(Expected + TEXT("/Logs")));
        TEST_EXPECT(Paths::GetProjectUserDir().Equals(Expected + TEXT("/User")));
        TEST_EXPECT(Paths::GetProjectCacheDir().Equals(Expected + TEXT("/Cache")));
        TEST_EXPECT(Paths::GetEngineCacheDir().Equals(Expected + TEXT("/Cache")));
        TEST_EXPECT(Paths::ResolveOutputPath(TEXT("Dump.txt")).Equals(Expected + TEXT("/Logs/Dump.txt")));

        TEST_EXPECT(File::DeleteDirectoryTree(PackagedDir));
    }

    TEST_SECTION(TEXT("The game module is the descriptor's first Game module, and a descriptor without one names none"));
    {
        const FString CodedArgument = FString(TEXT("-project=")) + Coded;
        const CHAR_T* CodedArgs[]   = { *CodedArgument };

        FString Error;
        TEST_EXPECT(InitializeProjectWith(CodedArgs, 1, Error));
        TEST_EXPECT(Paths::GetProjectModuleName().Equals(TEXT("CodedGame")));

        const FString GameArgument = FString(TEXT("-project=")) + Game;
        const CHAR_T* GameArgs[]   = { *GameArgument };
        TEST_EXPECT(InitializeProjectWith(GameArgs, 1, Error));
        TEST_EXPECT(Paths::GetProjectModuleName().IsEmpty());
    }

    TEST_SECTION(TEXT("A project's binaries are in its Build/bin, in the folder named like the executable's configuration folder"));
    {
        const FString Project = TEXT("C:/My Projects/Game");
        TEST_EXPECT(Paths::FindProjectBinariesDir(Project, TEXT("E:/Engine/Build/bin/Development Editor-windows-x64/SandboxStandalone.exe"))
            .Equals(TEXT("C:/My Projects/Game/Build/bin/Development Editor-windows-x64")));
        TEST_EXPECT(Paths::FindProjectBinariesDir(Project, TEXT("E:\\Engine\\Build\\bin\\Debug-windows-x64\\Game.exe"))
            .Equals(TEXT("C:/My Projects/Game/Build/bin/Debug-windows-x64")));
        TEST_EXPECT(Paths::FindProjectBinariesDir(Project, TEXT("/Engine/Build/bin/Development Editor-macosx-x64/SandboxStandalone.app/Contents/MacOS/SandboxStandalone"))
            .Equals(TEXT("C:/My Projects/Game/Build/bin/Development Editor-macosx-x64")));
        TEST_EXPECT(Paths::FindProjectBinariesDir(Project, TEXT("D:/Package/Game.exe")).IsEmpty());
        TEST_EXPECT(Paths::FindProjectBinariesDir(Project, TEXT("D:/Build/bin/Game.exe")).IsEmpty());
        TEST_EXPECT(Paths::FindProjectBinariesDir(TEXT(""), TEXT("E:/Engine/Build/bin/Debug-windows-x64/Game.exe")).IsEmpty());
    }

    TEST_SECTION(TEXT("A -project= that names no project, or a descriptor that does not load, fails with a reason"));
    {
        const FString EmptyArgument  = FString(TEXT("-project=")) + Empty;
        const FString BrokenArgument = FString(TEXT("-project=")) + Broken;
        const CHAR_T* NoValueArgs[]  = { TEXT("-project=") };
        const CHAR_T* EmptyArgs[]    = { *EmptyArgument };
        const CHAR_T* BrokenArgs[]   = { *BrokenArgument };

        FString Error;
        TEST_EXPECT(!InitializeProjectWith(NoValueArgs, 1, Error));
        TEST_EXPECT(!Error.IsEmpty());

        Error.Clear();
        TEST_EXPECT(!InitializeProjectWith(EmptyArgs, 1, Error));
        TEST_EXPECT(Error.Contains(Empty));

        Error.Clear();
        TEST_EXPECT(!InitializeProjectWith(BrokenArgs, 1, Error));
        TEST_EXPECT(Error.Contains(TEXT("has no Name")));
    }

    FString RestoreError;
    TEST_EXPECT(InitializeProjectWith(nullptr, 0, RestoreError));
    TEST_EXPECT(Paths::GetProjectDir().Equals(CompileTimeDir));

    TEST_EXPECT(File::DeleteDirectoryTree(Root));
    FPlatformFile::RemoveDirectory(*File::GetDirectoryOf(Root));

    TEST_END();
}
