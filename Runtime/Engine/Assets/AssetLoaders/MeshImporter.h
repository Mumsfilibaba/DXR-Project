#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Map.h"
#include "Core/Utilities/StringUtilities.h"
#include "Engine/Assets/SceneData.h"

enum class EMeshImportFlags : uint8
{
    None             = 0,
    ApplyScaleFactor = BIT(1),
    EnsureLeftHanded = BIT(2),

    Default = EnsureLeftHanded
};

ENUM_CLASS_OPERATORS(EMeshImportFlags);

// TODO: Move to the AssetManager
class ENGINE_API FMeshImporter
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
        int32 DiffuseTexIndex;
        int32 NormalTexIndex;
        int32 SpecularTexIndex;
        int32 EmissiveTexIndex;
        int32 AOTexIndex;
        int32 RoughnessTexIndex;
        int32 MetallicTexIndex;
        int32 AlphaMaskTexIndex;

        FVector3 Diffuse;

        float AO;
        float Roughness;
        float Metallic;

        bool bAlphaDiffuseCombined;
    };


    FMeshImporter();
    ~FMeshImporter() = default;

public:
    static bool Initialize();
    static void Release();

    static FORCEINLINE FMeshImporter& Get()
    {
        CHECK(GInstance != nullptr);
        return *GInstance;
    }

    bool LoadMesh(const FString& Filename, FSceneData& OutScene, EMeshImportFlags Flags = EMeshImportFlags::Default);

private:
    void LoadCacheFile();
    void UpdateCacheFile();

    bool AddCacheEntry(const FString& OriginalFile, const FString& NewFile, const FSceneData& Scene);

    bool LoadCustom(const FString& Filename, FSceneData& OutScene);

    // Maps the original path to the custom mesh format
    TMap<FString, FString> Cache; 

    static FMeshImporter* GInstance;
};