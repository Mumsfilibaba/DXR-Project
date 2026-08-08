#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "RendererCore/Shaders/ShaderType.h"

struct FShaderManifestEntry
{
    FShaderType*  Type = nullptr;
    TArray<int32> PermutationIDs;
};

class RENDERERCORE_API FShaderManifest
{
public:
    NODISCARD static String GetFilePath();

    bool Load();
    bool Save() const;

    void AddEntry(FShaderManifestEntry&& Entry)
    {
        Entries.Emplace(::Move(Entry));
    }

    NODISCARD const TArray<FShaderManifestEntry>& GetEntries() const
    {
        return Entries;
    }

    NODISCARD bool IsEmpty() const
    {
        return Entries.IsEmpty();
    }

private:
    TArray<FShaderManifestEntry> Entries;
};
