#include "Core/Misc/EngineDescriptor.h"
#include "Core/Json/Json.h"
#include "Core/Json/JsonArchive.h"
#include "Core/Json/JsonSerializer.h"

bool FEngineDescriptor::LoadFromFile(const FString& Filename, FString& OutError)
{
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

    FEngineDescriptor Loaded;
    Loaded.Serialize(Loader);

    if (Loader.HasErrors())
    {
        OutError = FString::Printf(TEXT("'%s': %s"), *Filename, *Loader.GetErrorString());
        return false;
    }

    if (Loaded.Name.IsEmpty())
    {
        OutError = FString::Printf(TEXT("'%s' has no Name"), *Filename);
        return false;
    }

    *this = Move(Loaded);
    return true;
}

bool FEngineDescriptor::SaveToFile(const FString& Filename) const
{
    FJsonValue Document;
    TJsonSerializer<FEngineDescriptor>::Save(Document, *this);
    return Json::SaveToFile(Filename, Document);
}

void FEngineDescriptor::Serialize(FJsonArchive& Archive)
{
    if (Archive.IsSaving())
    {
        Archive.SetVersion(CurrentVersion);
    }

    Archive.Field(TEXT("Name"), Name);
}
