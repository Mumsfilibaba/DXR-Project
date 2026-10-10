#include "Core/Misc/Paths.h"
#include "Core/Misc/CommandLine.h"
#include "Core/Filesystem/File.h"
#include "Core/Platform/PlatformFile.h"

static String           GEngineDir;
static EEngineDirSource GEngineDirSource     = EEngineDirSource::None;
static bool             GIsEngineDirResolved = false;

static bool IsEngineDir(const String& Directory)
{
    return !Directory.IsEmpty() && FPlatformFile::IsFile(*File::CombinePath(Directory, Paths::EngineIdentityFilename));
}

static String FindEngineDirAbove(const String& Directory)
{
    for (String Current = Directory; !Current.IsEmpty(); Current = File::GetDirectoryOf(Current))
    {
        if (IsEngineDir(Current))
        {
            return Current;
        }
    }

    return String();
}

bool Paths::Initialize()
{
    String     EngineOption;
    StringView EngineOptionValue;

    if (CommandLine::FindOption("engine", EngineOptionValue))
    {
        if (EngineOptionValue.IsEmpty())
        {
            GIsEngineDirResolved = true;
            GEngineDirSource     = EEngineDirSource::None;

            GEngineDir.Clear();
            return false;
        }

        EngineOption = String(EngineOptionValue.Data(), EngineOptionValue.Length());
    }

    const String ExecutableDir = File::GetDirectoryOf(FPlatformFile::GetExecutablePath());

    GEngineDir           = FindEngineDir(EngineOption, ExecutableDir, String(ENGINE_LOCATION), &GEngineDirSource);
    GIsEngineDirResolved = true;

    return !GEngineDir.IsEmpty();
}

String Paths::FindEngineDir(
    const String&     EngineOption,
    const String&     ExecutableDir,
    const String&     FallbackDir,
    EEngineDirSource* OutSource)
{
    EEngineDirSource Source = EEngineDirSource::None;
    String           Result;

    if (!EngineOption.IsEmpty())
    {
        const String Directory = File::MakeAbsolute(EngineOption);
        if (IsEngineDir(Directory))
        {
            Source = EEngineDirSource::CommandLine;
            Result = Directory;
        }
    }
    else
    {
        const String FoundAbove = ExecutableDir.IsEmpty() ? String() : FindEngineDirAbove(File::MakeAbsolute(ExecutableDir));
        const String Fallback   = File::NormalizePath(FallbackDir);

        if (!FoundAbove.IsEmpty())
        {
            Source = EEngineDirSource::ExecutableLocation;
            Result = FoundAbove;
        }
        else if (IsEngineDir(Fallback))
        {
            Source = EEngineDirSource::CompileTimeFallback;
            Result = Fallback;
        }
    }

    if (OutSource)
    {
        *OutSource = Source;
    }

    return Result;
}

EEngineDirSource Paths::GetEngineDirSource()
{
    GetEngineDir();
    return GEngineDirSource;
}

const CHAR* Paths::GetEngineDirSourceName()
{
    switch (GetEngineDirSource())
    {
        case EEngineDirSource::CommandLine:         return "-engine=";
        case EEngineDirSource::ExecutableLocation:  return "found above the executable";
        case EEngineDirSource::CompileTimeFallback: return "compile-time fallback";
        default:                                    return "not found";
    }
}

String Paths::GetEngineDir()
{
    if (!GIsEngineDirResolved)
    {
        Initialize();
    }

    return GEngineDir.IsEmpty() ? File::NormalizePath(String(ENGINE_LOCATION)) : GEngineDir;
}

String Paths::GetAssetDir()
{
    return Paths::GetEngineDir() + String("/Assets");
}

String Paths::GetProjectDir()
{
    return String(PROJECT_LOCATION);
}

String Paths::GetProjectName()
{
    return String(PROJECT_NAME);
}

String Paths::GetProjectModuleName()
{
    return String(PROJECT_NAME);
}

String Paths::GetUserSettingsDir()
{
    const String BaseDirectory = FPlatformFile::GetUserSettingsDirectory();
    return BaseDirectory.IsEmpty() ? String() : BaseDirectory + String("/DXR-Engine");
}
