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

static FString GetCompileTimeEngineDir()
{
#if WITH_COMPILE_TIME_PATHS
    return File::NormalizePath(FString(TEXT(ENGINE_LOCATION)));
#else
    return FString();
#endif
}

static FString GetCompileTimeProjectDir()
{
#if WITH_COMPILE_TIME_PATHS
    return File::NormalizePath(FString(TEXT(PROJECT_LOCATION)));
#else
    return FString();
#endif
}

static FString GetCompileTimeProjectName()
{
#if WITH_COMPILE_TIME_PATHS
    return FString(TEXT(PROJECT_NAME));
#else
    return FString();
#endif
}

static FString ReadPackageValue(const FString& PackageFile, const CHAR_T* Key)
{
    FIniFile Package;
    if (!Package.LoadFromFile(PackageFile))
    {
        return FString();
    }

    const FIniValue* Value = Package.FindValue(TEXT("Package"), Key);
    return Value ? Value->CurrentValue : FString();
}

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

bool Paths::Initialize(FString& OutError)
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
            OutError = FString(TEXT("-engine= needs the path of an engine folder"));
            return false;
        }

        EngineOption = FString(EngineOptionValue.Data(), EngineOptionValue.Length());
    }

    const FString ExecutableDir = File::GetDirectoryOf(FPlatformFile::GetExecutablePath());

    GEngineDir           = FindEngineDir(EngineOption, ExecutableDir, GetCompileTimeEngineDir(), &GEngineDirSource);
    GIsEngineDirResolved = true;

    if (!GEngineDir.IsEmpty())
    {
        return true;
    }

    const FString PackageFile = File::CombinePath(ExecutableDir, PackageFilename);
    if (!EngineOption.IsEmpty())
    {
        OutError = FString::Printf(TEXT("-engine=%s is not an engine folder, since it has no %s"), *EngineOption, EngineIdentityFilename);
    }
    else if (FPlatformFile::IsFile(*PackageFile))
    {
        OutError = FString::Printf(TEXT("%s next to the executable names no engine folder. Stage the engine, with its %s, in an Engine folder next to the executable, or name its folder relative to the executable with [Package] Engine=\"<path>\"."),
            PackageFilename, EngineIdentityFilename);
    }
    else
    {
        OutError = FString::Printf(TEXT("Could not find the engine folder. Pass -engine=<path> to a folder that contains %s, or run the executable from inside one."), EngineIdentityFilename);
    }

    return false;
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
    else if (!ExecutableDir.IsEmpty() && FPlatformFile::IsFile(*File::CombinePath(File::MakeAbsolute(ExecutableDir), PackageFilename)))
    {
        const FString AbsoluteExecutableDir = File::MakeAbsolute(ExecutableDir);
        const FString EngineValue           = ReadPackageValue(File::CombinePath(AbsoluteExecutableDir, PackageFilename), TEXT("Engine"));
        const FString Directory             = File::NormalizePath(File::CombinePath(AbsoluteExecutableDir, EngineValue.IsEmpty() ? FString(TEXT("Engine")) : EngineValue));
        if (IsEngineDir(Directory))
        {
            Source = EEngineDirSource::PackageFile;
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
    GProjectDir        = GetCompileTimeProjectDir();
    GProjectName       = GetCompileTimeProjectName();
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
    const FString FallbackFile  = GProjectDir.IsEmpty() ? FString() : File::CombinePath(GProjectDir, GProjectName + FProjectDescriptor::FileExtension);

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

        // Without a compile-time project there is nothing to fall back to
        if (GProjectDir.IsEmpty())
        {
            OutError = FString::Printf(TEXT("There is no project to run. Pass -project=<path to a %s file>, or put a %s naming one next to the executable."),
                FProjectDescriptor::FileExtension, PackageFilename);
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
        const FString ProjectValue = ReadPackageValue(PackageFile, TEXT("Project"));
        if (!ProjectValue.IsEmpty())
        {
            Result = ResolveProjectFile(File::NormalizePath(File::CombinePath(File::GetDirectoryOf(PackageFile), ProjectValue)));
            Source = Result.IsEmpty() ? EProjectSource::None : EProjectSource::PackageFile;
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

FString Paths::FindProjectBinariesDir(const FString& ProjectDir, const FString& ExecutablePath)
{
    static constexpr const CHAR_T* BinFolder = TEXT("/Build/bin/");

    const FString Executable  = File::NormalizePath(ExecutablePath);
    const int32   BinPosition = Executable.FindLast(BinFolder);

    if (ProjectDir.IsEmpty() || BinPosition == FString::InvalidIndex)
    {
        return FString();
    }

    const int32 ConfigurationStart = BinPosition + CString::Strlen(BinFolder);
    const int32 ConfigurationEnd   = Executable.FindChar('/', ConfigurationStart);

    if (ConfigurationEnd == FString::InvalidIndex || ConfigurationEnd <= ConfigurationStart)
    {
        return FString();
    }

    const FString Configuration = Executable.SubString(ConfigurationStart, ConfigurationEnd - ConfigurationStart);
    return File::CombinePath(File::NormalizePath(ProjectDir), FString(TEXT("Build/bin/")) + Configuration);
}

FString Paths::FindWritableDir(const FString& ProjectDir, const FString& ProjectName, bool bIsPackaged, const FString& UserSettingsDir)
{
    if (!bIsPackaged || UserSettingsDir.IsEmpty() || ProjectName.IsEmpty())
    {
        return ProjectDir;
    }

    return File::CombinePath(UserSettingsDir, ProjectName);
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
        case EEngineDirSource::PackageFile:         return TEXT("named in Package.ini");
        case EEngineDirSource::CompileTimeFallback: return TEXT("compile-time fallback");
        default:                                    return TEXT("not found");
    }
}

FString Paths::GetEngineDir()
{
    if (!GIsEngineDirResolved)
    {
        FString Error;
        Initialize(Error);
    }

    return GEngineDir.IsEmpty() ? GetCompileTimeEngineDir() : GEngineDir;
}

FString Paths::GetAssetDir()
{
    return Paths::GetEngineDir() + FString(TEXT("/Assets"));
}

FString Paths::GetEngineContentDir()
{
    return Paths::GetEngineDir() + FString(TEXT("/Content"));
}

FString Paths::GetEngineCacheDir()
{
    return IsPackaged() ? Paths::GetWritableDir() + FString(TEXT("/Cache")) : Paths::GetEngineDir() + FString(TEXT("/Cache"));
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

bool Paths::IsPackaged()
{
    return GetProjectSource() == EProjectSource::PackageFile;
}

FString Paths::GetWritableDir()
{
    return FindWritableDir(GetProjectDir(), GetProjectName(), IsPackaged(), GetUserSettingsDir());
}

FString Paths::GetProjectCacheDir()
{
    return Paths::GetWritableDir() + FString(TEXT("/Cache"));
}

FString Paths::GetProjectLogsDir()
{
    return Paths::GetWritableDir() + FString(TEXT("/Logs"));
}

FString Paths::GetProjectUserDir()
{
    return Paths::GetWritableDir() + FString(TEXT("/User"));
}

FString Paths::ResolveOutputPath(const FString& Path)
{
    return (Path.IsEmpty() || File::IsAbsolutePath(Path)) ? Path : File::CombinePath(GetProjectLogsDir(), Path);
}

FString Paths::GetProjectModuleName()
{
    const FProjectDescriptor* Descriptor = GetProjectDescriptor();
    if (!Descriptor)
    {
        return GetCompileTimeProjectName();
    }

    for (const FProjectModuleDescriptor& Module : Descriptor->Modules)
    {
        if (Module.Type == EProjectModuleType::Game)
        {
            return Module.Name;
        }
    }

    return FString();
}

FString Paths::GetProjectBinariesDir()
{
    return FindProjectBinariesDir(GetProjectDir(), FPlatformFile::GetExecutablePath());
}

FString Paths::GetUserSettingsDir()
{
    const FString BaseDirectory = FPlatformFile::GetUserSettingsDirectory();
    return BaseDirectory.IsEmpty() ? FString() : BaseDirectory + FString(TEXT("/DXR-Engine"));
}
