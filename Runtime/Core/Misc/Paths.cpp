#include "Core/Misc/Paths.h"
#include "Core/Misc/CommandLine.h"
#include "Core/Filesystem/File.h"
#include "Core/Platform/PlatformFile.h"

static FString          GEngineDir;
static EEngineDirSource GEngineDirSource     = EEngineDirSource::None;
static bool             GIsEngineDirResolved = false;

static bool IsEngineDir(const FString& Directory)
{
    return !Directory.IsEmpty() && FPlatformFile::IsFile(*File::CombinePath(Directory, Paths::EngineIdentityFilename));
}

static FString FindEngineDirAbove(const FString& Directory)
{
    for (FString Current = Directory; !Current.IsEmpty(); Current = File::GetDirectoryOf(Current))
    {
        if (IsEngineDir(Current))
        {
            return Current;
        }
    }

    return FString();
}

bool Paths::Initialize()
{
    FString     EngineOption;
    FStringView EngineOptionValue;

    if (CommandLine::FindOption(TEXT("engine"), EngineOptionValue))
    {
        if (EngineOptionValue.IsEmpty())
        {
            GIsEngineDirResolved = true;
            GEngineDirSource     = EEngineDirSource::None;

            GEngineDir.Clear();
            return false;
        }

        EngineOption = FString(EngineOptionValue.Data(), EngineOptionValue.Length());
    }

    const FString ExecutableDir = File::GetDirectoryOf(FPlatformFile::GetExecutablePath());

    GEngineDir           = FindEngineDir(EngineOption, ExecutableDir, FString(TEXT(ENGINE_LOCATION)), &GEngineDirSource);
    GIsEngineDirResolved = true;

    return !GEngineDir.IsEmpty();
}

FString Paths::FindEngineDir(
    const FString&    EngineOption,
    const FString&    ExecutableDir,
    const FString&    FallbackDir,
    EEngineDirSource* OutSource)
{
    EEngineDirSource Source = EEngineDirSource::None;
    FString          Result;

    if (!EngineOption.IsEmpty())
    {
        const FString Directory = File::MakeAbsolute(EngineOption);
        if (IsEngineDir(Directory))
        {
            Source = EEngineDirSource::CommandLine;
            Result = Directory;
        }
    }
    else
    {
        const FString FoundAbove = ExecutableDir.IsEmpty() ? FString() : FindEngineDirAbove(File::MakeAbsolute(ExecutableDir));
        const FString Fallback   = File::NormalizePath(FallbackDir);

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

const CHAR_T* Paths::GetEngineDirSourceName()
{
    switch (GetEngineDirSource())
    {
        case EEngineDirSource::CommandLine:         return TEXT("-engine=");
        case EEngineDirSource::ExecutableLocation:  return TEXT("found above the executable");
        case EEngineDirSource::CompileTimeFallback: return TEXT("compile-time fallback");
        default:                                    return TEXT("not found");
    }
}

FString Paths::GetEngineDir()
{
    if (!GIsEngineDirResolved)
    {
        Initialize();
    }

    return GEngineDir.IsEmpty() ? File::NormalizePath(FString(TEXT(ENGINE_LOCATION))) : GEngineDir;
}

FString Paths::GetAssetDir()
{
    return Paths::GetEngineDir() + FString(TEXT("/Assets"));
}

FString Paths::GetProjectDir()
{
    return FString(TEXT(PROJECT_LOCATION));
}

FString Paths::GetProjectName()
{
    return FString(TEXT(PROJECT_NAME));
}

FString Paths::GetProjectModuleName()
{
    return FString(TEXT(PROJECT_NAME));
}

FString Paths::GetUserSettingsDir()
{
    const FString BaseDirectory = FPlatformFile::GetUserSettingsDirectory();
    return BaseDirectory.IsEmpty() ? FString() : BaseDirectory + FString(TEXT("/DXR-Engine"));
}
