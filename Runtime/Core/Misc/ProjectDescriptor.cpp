#include "Core/Misc/ProjectDescriptor.h"
#include "Core/Json/Json.h"
#include "Core/Json/JsonArchive.h"
#include "Core/Json/JsonSerializer.h"

JSON_ENUM_NAMES_BEGIN(EProjectModuleType)
    JSON_ENUM_NAME(EProjectModuleType, Game)
    JSON_ENUM_NAME(EProjectModuleType, Editor)
JSON_ENUM_NAMES_END()

void FProjectModuleDescriptor::Serialize(FJsonArchive& Archive)
{
    Archive.Field("Name", Name);
    Archive.Field("Type", Type, EProjectModuleType::Game);
}

bool FProjectDescriptor::LoadFromFile(const FString& Filename, FString& OutError)
{
    FJsonValue Document;
    FJsonError ParseError;

    if (!Json::LoadFromFile(Filename, Document, &ParseError))
    {
        OutError = FString::Printf("'%s': %s", *Filename, *ParseError.ToString());
        return false;
    }

    FJsonArchive Loader = FJsonArchive::Loader(Document);

    const int32 Version = Loader.GetVersion();
    if (Version < 1)
    {
        OutError = FString::Printf("'%s' has no Version", *Filename);
        return false;
    }

    if (Version > CurrentVersion)
    {
        OutError = FString::Printf("'%s' is version %d, but this engine reads up to version %d", *Filename, Version, CurrentVersion);
        return false;
    }

    FProjectDescriptor Loaded;
    Loaded.Serialize(Loader);

    if (Loader.HasErrors())
    {
        OutError = FString::Printf("'%s': %s", *Filename, *Loader.GetErrorString());
        return false;
    }

    if (Loaded.Name.IsEmpty())
    {
        OutError = FString::Printf("'%s' has no Name", *Filename);
        return false;
    }

    *this = Move(Loaded);
    return true;
}

bool FProjectDescriptor::SaveToFile(const FString& Filename) const
{
    FJsonValue Document;
    TJsonSerializer<FProjectDescriptor>::Save(Document, *this);
    return Json::SaveToFile(Filename, Document);
}

void FProjectDescriptor::Serialize(FJsonArchive& Archive)
{
    if (Archive.IsSaving())
    {
        Archive.SetVersion(CurrentVersion);
    }

    Archive.Field("Name", Name);
    Archive.Field("EngineAssociation", EngineAssociation, FString());
    Archive.Field("EnginePath", EnginePath, FString());
    Archive.Field("Modules", Modules);
}
