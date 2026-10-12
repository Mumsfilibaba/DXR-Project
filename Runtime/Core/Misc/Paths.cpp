#include "Core/Misc/Paths.h"
#include "Core/Misc/CommandLine.h"
#include "Core/Misc/IniFile.h"
#include "Core/Misc/ProjectDescriptor.h"
#include "Core/Filesystem/File.h"
#include "Core/Platform/PlatformFile.h"

static FString          GEngineDir;
static EEngineDirSource GEngineDirSource     = EEngineDirSource::None;
static bool             GIsEngineDirResolved = false;

static FProjectDescriptor GProjectDescriptor;
static FString            GProjectFile;
static FString            GProjectDir;
static FString            GProjectName;
static EProjectSource     GProjectSource      = EProjectSource::None;
static bool               GIsProjectResolved  = false;

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

static FString ResolveProjectFile(const FString& Path)
{
    if (FPlatformFile::IsFile(*Path))
    {
        return File::ExtractExtension(Path).Equals(FProjectDescriptor::FileExtension, EStringCaseType::NoCase) ? Path : FString();
    }

    if (FPlatformFile::IsDirectory(*Path))
    {
        const FString NamedAfterFolder = File::CombinePath(Path, File::ExtractFilename(Path) + FProjectDescriptor::FileExtension);
        return FPlatformFile::IsFile(*NamedAfterFolder) ? NamedAfterFolder : FString();
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

bool Paths::InitializeProject(FString& OutError)
{
    GIsProjectResolved = true;
    GProjectDescriptor = FProjectDescriptor();
    GProjectFile.Clear();
    GProjectDir        = File::NormalizePath(FString(TEXT(PROJECT_LOCATION)));
    GProjectName       = FString(TEXT(PROJECT_NAME));
    GProjectSource     = EProjectSource::None;

    FString     ProjectOption;
    FStringView ProjectOptionValue;

    const bool bHasProjectOption = CommandLine::FindOption(TEXT("project"), ProjectOptionValue);
    if (bHasProjectOption)
    {
        if (ProjectOptionValue.IsEmpty())
        {
            OutError = FString(TEXT("-project= needs the path of a .dxrproject file, or of the folder that holds one"));
            return false;
        }

        ProjectOption = FString(ProjectOptionValue.Data(), ProjectOptionValue.Length());
    }

    const FString ExecutableDir = File::GetDirectoryOf(FPlatformFile::GetExecutablePath());
    const FString FallbackFile  = File::CombinePath(FString(TEXT(PROJECT_LOCATION)), FString(TEXT(PROJECT_NAME)) + FProjectDescriptor::FileExtension);

    EProjectSource Source      = EProjectSource::None;
    const FString  ProjectFile = FindProjectFile(ProjectOption, ExecutableDir, FallbackFile, &Source);
    if (ProjectFile.IsEmpty())
    {
        if (bHasProjectOption)
        {
            OutError = FString::Printf(TEXT("-project=%s is not a project. Pass a .dxrproject file, or the folder that holds one named after it."), *ProjectOption);
            return false;
        }

        if (FPlatformFile::IsFile(*File::CombinePath(ExecutableDir, PackageFilename)))
        {
            OutError = FString::Printf(TEXT("%s next to the executable names no project that exists. Its [Package] section needs Project=\"<path relative to the executable>\"."), PackageFilename);
            return false;
        }

        return true;
    }

    FProjectDescriptor Descriptor;
    if (!Descriptor.LoadFromFile(ProjectFile, OutError))
    {
        return false;
    }

    GProjectDescriptor = ::Move(Descriptor);
    GProjectFile       = ProjectFile;
    GProjectDir        = File::GetDirectoryOf(ProjectFile);
    GProjectName       = GProjectDescriptor.Name;
    GProjectSource     = Source;
    return true;
}

FString Paths::FindProjectFile(
    const FString&  ProjectOption,
    const FString&  ExecutableDir,
    const FString&  FallbackFile,
    EProjectSource* OutSource)
{
    EProjectSource Source = EProjectSource::None;
    FString        Result;

    const FString PackageFile = ExecutableDir.IsEmpty() ? FString() : File::CombinePath(File::MakeAbsolute(ExecutableDir), PackageFilename);

    if (!ProjectOption.IsEmpty())
    {
        Result = ResolveProjectFile(File::MakeAbsolute(ProjectOption));
        Source = Result.IsEmpty() ? EProjectSource::None : EProjectSource::CommandLine;
    }
    else if (!PackageFile.IsEmpty() && FPlatformFile::IsFile(*PackageFile))
    {
        // A packaged game only ever looks where its Package.ini points, never at a compile-time path
        FIniFile Package;
        if (Package.LoadFromFile(PackageFile))
        {
            const FIniValue* ProjectValue = Package.FindValue(TEXT("Package"), TEXT("Project"));
            if (ProjectValue && !ProjectValue->CurrentValue.IsEmpty())
            {
                Result = ResolveProjectFile(File::NormalizePath(File::CombinePath(File::GetDirectoryOf(PackageFile), ProjectValue->CurrentValue)));
                Source = Result.IsEmpty() ? EProjectSource::None : EProjectSource::PackageFile;
            }
        }
    }
    else if (!FallbackFile.IsEmpty())
    {
        Result = ResolveProjectFile(File::MakeAbsolute(FallbackFile));
        Source = Result.IsEmpty() ? EProjectSource::None : EProjectSource::CompileTimeFallback;
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

FString Paths::GetEngineContentDir()
{
    return Paths::GetEngineDir() + FString(TEXT("/Content"));
}

FString Paths::GetEngineSavedDir()
{
    return Paths::GetEngineDir() + FString(TEXT("/Saved"));
}

static void EnsureProjectResolved()
{
    if (!GIsProjectResolved)
    {
        FString Error;
        Paths::InitializeProject(Error);
    }
}

EProjectSource Paths::GetProjectSource()
{
    EnsureProjectResolved();
    return GProjectSource;
}

const CHAR_T* Paths::GetProjectSourceName()
{
    switch (GetProjectSource())
    {
        case EProjectSource::CommandLine:         return TEXT("-project=");
        case EProjectSource::PackageFile:         return TEXT("named in Package.ini");
        case EProjectSource::CompileTimeFallback: return TEXT("compile-time fallback");
        default:                                  return TEXT("no descriptor, compile-time project");
    }
}

FString Paths::GetProjectFile()
{
    EnsureProjectResolved();
    return GProjectFile;
}

const FProjectDescriptor* Paths::GetProjectDescriptor()
{
    EnsureProjectResolved();
    return GProjectFile.IsEmpty() ? nullptr : &GProjectDescriptor;
}

FString Paths::GetProjectDir()
{
    EnsureProjectResolved();
    return GProjectDir;
}

FString Paths::GetProjectName()
{
    EnsureProjectResolved();
    return GProjectName;
}

FString Paths::GetProjectContentDir()
{
    return Paths::GetProjectDir() + FString(TEXT("/Content"));
}

FString Paths::GetProjectConfigDir()
{
    return Paths::GetProjectDir() + FString(TEXT("/Config"));
}

FString Paths::GetProjectSavedDir()
{
    return Paths::GetProjectDir() + FString(TEXT("/Saved"));
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
