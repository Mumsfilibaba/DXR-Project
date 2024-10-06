#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Map.h"
#include "Core/Utilities/StringUtilities.h"
#include "Engine/Assets/SceneData.h"
#include "Engine/Assets/IMeshImporter.h"

// TODO: Move to the AssetManager
class ENGINE_API FMeshImporter : public IMeshImporter
{
    struct FCustomScene
    {
        enum
        {
            MaxModels    = 1000000,
            MaxMaterials = 1000000,
        };

        int64 NumTotalVertices;
        int64 NumTotalIndices;

        int32 NumModels;
        int32 NumMaterials;

        int32 NumTextures;
    };

    struct FCustomModel
    {
        enum
        {
            MaxNameLength = 256
        };

        CHAR Name[MaxNameLength];
        
        // TODO: Probably want sub-meshes
        int32 NumVertices;
        int32 NumIndices;
        int32 MaterialIndex;
    };

    struct FTextureHeader
    {
        enum
        {
            MaxNameLength = 256
        };

        CHAR Filepath[MaxNameLength];
    };

    struct FCustomMaterial
    {
        int32    DiffuseTexIndex;
        int32    NormalTexIndex;
        int32    SpecularTexIndex;
        int32    EmissiveTexIndex;
        int32    AOTexIndex;
        int32    RoughnessTexIndex;
        int32    MetallicTexIndex;
        int32    AlphaMaskTexIndex;

        FVector3 Diffuse;
        float    AO;
        float    Roughness;
        float    Metallic;
        int32    MaterialFlags;
    };

public:
    FMeshImporter();
    ~FMeshImporter();
    
    bool Initialize();
    void Release();

    virtual TSharedRef<FSceneData> ImportFromFile(const FStringView& Filename, EMeshImportFlags Flags) override final;
    virtual bool MatchExtenstion(const FStringView& FileName) override final;
    
    TSharedRef<FSceneData> LoadCustom(const FString& Filename);

    void LoadCacheFile();
    void UpdateCacheFile();
    bool AddCacheEntry(const FString& OriginalFile, const FString& NewFile, const TSharedRef<FSceneData>& Scene);
    
private:
    // Maps the original path to the custom mesh format
    TMap<FString, FString> Cache;
};
