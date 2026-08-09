#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/Map.h"
#include "Core/Templates/TypeTraits.h"
#include "Engine/Assets/ModelData.h"
#include "Engine/Assets/IModelImporter.h"

#define MODEL_FORMAT_VERSION_MAJOR (0)
#define MODEL_FORMAT_VERSION_MINOR (10)
#define MODEL_FORMAT_MAX_NAME_LENGTH (256)
#define MODEL_FORMAT_INVALID_TEXTURE_ID (-1)

namespace ModelFormat
{
    struct FFileHeader
    {
        CHAR   Magic[7];     // Always "DXRMESH"
        CHAR   Padding[1];   // Should me zero
        uint64 DataCRC;      // CRC of the data (Everything excluding the FFileHeader)
        uint64 DataSize;     // Total DataSize
        uint32 VersionMajor; // Major version of the file
        uint32 VersionMinor; // Minor version of the file
    };

    static_assert(TAlignmentOf<FFileHeader>::Value == sizeof(uint64));

    struct FModelHeader
    {
        int32 MeshDataOffset;                       // Offset to where the MeshData starts
        int32 NumMeshes;                            // Number of Meshes
        
        int32 MaterialDataOffset;                   // Offset to where the MaterialData starts
        int32 NumMaterials;                         // Number of Materials
        
        int32 SubMeshDataOffset;                    // Offset to where the SubMeshData starts
        int32 NumSubMeshes;                         // Number of SubMeshes
        
        int32 StreamDataOffset[VERTEX_MAX_STREAMS]; // Vertices are stored in the GPU stream layout, one blob per stream. A stream the declaration does not use has an offset of zero.
        int32 NumVertices;                          // Number of Vertices
        
        int32 IndexDataOffset;                      // Offset to where the IndexData starts
        int32 NumIndicies;                          // Number of Indicies
        
        int32 TextureDataOffset;                    // Offset to where the TextureData starts
        int32 NumTextures;                          // Number of Textures
    };

    struct FMeshInfo
    {
        CHAR  Name[MODEL_FORMAT_MAX_NAME_LENGTH]; // Name of the mesh
        
        int32 FirstSubMesh;                       // First SubMesh in the SubMeshArray
        int32 NumSubMeshes;                       // Number of SubMeshes for this Mesh
        
        int32 FirstVertex;                        // First Vertex in each stream
        int32 NumVertices;                        // Number of Vertices for this Mesh
        
        int32 FirstIndex;                         // First Index in the IndexArray
        int32 NumIndices;                         // Number of Indicies for this Mesh

        int32 AttributeFlags;                     // EVertexAttributeFlags this mesh's streams were packed for
    };

    struct FSubMeshInfo
    {
        int32 BaseVertex;    // BaseOffset relative to the mesh
        int32 NumVertices;   // Number of vertices
        
        int32 StartIndex;    // First index relative to the mesh
        int32 NumIndicies;   // Number of indicies
        
        int32 MaterialIndex; // Index into the MaterialArray in the file
    };

    struct FTextureInfo
    {
        CHAR Filepath[MODEL_FORMAT_MAX_NAME_LENGTH];
    };

    struct FMaterialInfo
    {
        int32   DiffuseTextureIdx;
        int32   NormalTextureIdx;
        int32   SpecularTextureIdx;
        int32   EmissiveTextureIdx;
        int32   AmbientOcclusionTextureIdx;
        int32   RoughnessTextureIdx;
        int32   MetallicTextureIdx;
        int32   AlphaMaskTextureIdx;
        uint32  ScalarRoutes[EMaterialScalar::Count];
        Vector3 Diffuse;
        float   AO;
        float   Roughness;
        float   Metallic;
        int32   MaterialFlags;
    };

    inline uint32 PackSourceRoute(const FMaterialSourceRoute& Route)
    {
        const uint32 Texture = Route.IsRouted() ? uint32(Route.Texture) : uint32(EMaterialTexture::Count);
        return Texture | (uint32(Route.Channel) << 8) | (Route.bInvert ? (1u << 10) : 0u);
    }

    inline FMaterialSourceRoute UnpackSourceRoute(uint32 Packed)
    {
        const uint32 Texture = Packed & 0xFFu;
        if (Texture >= uint32(EMaterialTexture::Count))
        {
            return FMaterialSourceRoute();
        }

        return FMaterialSourceRoute(EMaterialTexture::Type(Texture), ETextureChannel((Packed >> 8) & 0x3u), ((Packed >> 10) & 0x1u) != 0);
    }
}

struct ENGINE_API FModelImporter : public IModelImporter
{
    virtual ~FModelImporter() = default;

    virtual bool ImportFromFile(const StringView& Filename, EMeshImportFlags Flags, FModelData& OutModelData) override final;
    virtual bool MatchExtenstion(const StringView& FileName) override final;
};

struct ENGINE_API FModelSerializer
{
    virtual ~FModelSerializer() = default;

    bool Serialize(const String& Filename, const FModelData& ModelData);
};
