#include "Core/Misc/EngineRegistry.h"
#include "Core/Misc/EngineDescriptor.h"
#include "Core/Misc/Paths.h"
#include "Core/Misc/ProjectDescriptor.h"
#include "Core/Filesystem/File.h"
#include "Core/Json/Json.h"
#include "Core/Json/JsonArchive.h"
#include "Core/Json/JsonSerializer.h"
#include "Core/Platform/PlatformFile.h"

static bool IsEngineDirNamed(const FString& Directory, const FString& Name)
{
    const FString DescriptorFile = File::CombinePath(Directory, Paths::EngineIdentityFilename);
    if (!FPlatformFile::IsFile(*DescriptorFile))
    {
        return false;
    }

    FEngineDescriptor Descriptor;
    FString           Error;
    return Descriptor.LoadFromFile(DescriptorFile, Error) && Descriptor.Name.Equals(Name, EStringCaseType::NoCase);
}

void FEngineRegistryEntry::Serialize(FJsonArchive& Archive)
{
    Archive.Field(TEXT("Name"), Name);
    Archive.Field(TEXT("Path"), Path);
}

FString FEngineRegistry::GetDefaultFilename()
{
    const FString UserSettingsDir = Paths::GetUserSettingsDir();
    return UserSettingsDir.IsEmpty() ? FString() : File::CombinePath(UserSettingsDir, RegistryFilename);
}

bool FEngineRegistry::RegisterRunningEngine(FString& OutError)
{
    const FString EngineDir = Paths::GetEngineDir();

    FEngineDescriptor Engine;
    if (!Engine.LoadFromFile(File::CombinePath(EngineDir, Paths::EngineIdentityFilename), OutError))
    {
        return false;
    }

    const FString Filename = GetDefaultFilename();
    if (Filename.IsEmpty())
    {
        OutError = FString(TEXT("there is no per-user settings directory to keep the engine registry in"));
        return false;
    }

    FEngineRegistry Registry;
    if (!Registry.LoadFromFile(Filename, OutError))
    {
        return false;
    }

    if (!Registry.Register(Engine.Name, EngineDir))
    {
        return true;
    }

    if (!File::CreateDirectoryTree(File::GetDirectoryOf(Filename)) || !Registry.SaveToFile(Filename))
    {
        OutError = FString::Printf(TEXT("could not write '%s'"), *Filename);
        return false;
    }

    return true;
}

bool FEngineRegistry::LoadFromFile(const FString& Filename, FString& OutError)
{
    if (!FPlatformFile::IsFile(*Filename))
    {
        Engines.Clear();
        return true;
    }

    FJsonValue Document;
    FJsonError ParseError;

    if (!Json::LoadFromFile(Filename, Document, &ParseError))
    {
        OutError = FString::Printf(TEXT("'%s': %s"), *Filename, *ParseError.ToString());
        return false;
    }

    FJsonArchive Loader = FJsonArchive::Loader(Document);

    const int32 Version = Loader.GetVersion();
    if (Version < 1)
    {
        OutError = FString::Printf(TEXT("'%s' has no Version"), *Filename);
        return false;
    }

    if (Version > CurrentVersion)
    {
        OutError = FString::Printf(TEXT("'%s' is version %d, but this engine reads up to version %d"), *Filename, Version, CurrentVersion);
        return false;
    }

    FEngineRegistry Loaded;
    Loaded.Serialize(Loader);

    if (Loader.HasErrors())
    {
        OutError = FString::Printf(TEXT("'%s': %s"), *Filename, *Loader.GetErrorString());
        return false;
    }

    *this = Move(Loaded);
    return true;
}

bool FEngineRegistry::SaveToFile(const FString& Filename) const
{
    FJsonValue Document;
    TJsonSerializer<FEngineRegistry>::Save(Document, *this);
    return Json::SaveToFile(Filename, Document);
}

void FEngineRegistry::Serialize(FJsonArchive& Archive)
{
    if (Archive.IsSaving())
    {
        Archive.SetVersion(CurrentVersion);
    }

    Archive.Field(TEXT("Engines"), Engines);
}

bool FEngineRegistry::Register(const FString& Name, const FString& EngineDir)
{
    const FString Path = File::MakeAbsolute(EngineDir);

    int32 Position = 0;
    while (Position < Engines.Size() && Engines[Position].Name.Compare(Name, EStringCaseType::NoCase) < 0)
    {
        ++Position;
    }

    if (Position < Engines.Size() && Engines[Position].Name.Equals(Name, EStringCaseType::NoCase))
    {
        if (Engines[Position].Name.Equals(Name) && Engines[Position].Path.Equals(Path))
        {
            return false;
        }

        Engines[Position].Name = Name;
        Engines[Position].Path = Path;
        return true;
    }

    FEngineRegistryEntry Entry;
    Entry.Name = Name;
    Entry.Path = Path;
    Engines.Insert(Position, Move(Entry));
    return true;
}

FString FEngineRegistry::Find(const FString& Name) const
{
    for (const FEngineRegistryEntry& Entry : Engines)
    {
        if (Entry.Name.Equals(Name, EStringCaseType::NoCase))
        {
            return Entry.Path;
        }
    }

    return FString();
}

FString FEngineRegistry::ResolveProjectEngine(const FProjectDescriptor& Project, const FString& ProjectDir) const
{
    // A registered folder that was moved, deleted or replaced by another engine no longer counts
    if (!Project.EngineAssociation.IsEmpty())
    {
        const FString Registered = Find(Project.EngineAssociation);
        if (!Registered.IsEmpty() && IsEngineDirNamed(Registered, Project.EngineAssociation))
        {
            return Registered;
        }
    }

    if (!Project.EnginePath.IsEmpty())
    {
        const FString EngineDir = File::IsAbsolutePath(Project.EnginePath)
            ? File::NormalizePath(Project.EnginePath)
            : File::NormalizePath(File::CombinePath(File::MakeAbsolute(ProjectDir), Project.EnginePath));

        if (FPlatformFile::IsFile(*File::CombinePath(EngineDir, Paths::EngineIdentityFilename)))
        {
            return EngineDir;
        }
    }

    return FString();
}
