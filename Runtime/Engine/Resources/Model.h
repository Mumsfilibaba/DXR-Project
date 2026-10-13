#pragma once
#include "AssetCore/Asset.h"
#include "AssetCore/AssetRef.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Math/AABB.h"
#include "RHI/RHIResources.h"
#include "RHI/RHICommandList.h"
#include "Engine/EngineModule.h"
#include "Engine/Assets/MeshData.h"
#include "Engine/Assets/ModelData.h"
#include "Engine/Resources/Material.h"
#include "RendererCore/VertexDeclaration.h"

struct FVertexStreamBinding;

struct FMeshMaterialSlot
{
    /** @brief The source's name for the material, such as "Bricks" */
    FString Name;

    TAssetRef<FMaterial> Material;
};

class ENGINE_API FMesh : public FAsset
{
public:
    static TSharedPtr<FMesh> Create(const FMeshData& MeshData, bool bCreateRayTracingResources = false);

public:
    FMesh();
    ~FMesh();

    bool BuildAccelerationStructure(FRHICommandList& CommandList);

    bool EnsureRayTracingResources();
    void ReleaseRayTracingResources();

    void SetVertexBuffers(FRHICommandList& CommandList, const FVertexStreamBinding& Binding) const;

    const FVertexDeclaration& GetVertexDeclaration() const
    {
        return Declaration;
    }

    FRHIBuffer* GetPositionBuffer() const
    {
        return VertexStreams[EVertexStreamIndex::Position].Get();
    }

    FRHIBuffer* GetAttributeBuffer() const
    {
        return VertexStreams[EVertexStreamIndex::Attributes].Get();
    }

    FRHIShaderResourceView* GetAttributeBufferSRV() const
    {
        return AttributeBufferSRV.Get();
    }

    FRHIBuffer* GetIndexBuffer() const
    {
        return IndexBuffer.Get();
    }
    
    FRHIShaderResourceView* GetIndexBufferSRV() const
    {
        return IndexBufferSRV.Get();
    }
    
    FRHIGeometryAccelerationStructure* GetRayTracingGeometry() const
    {
        return RayTracingGeometry.Get();
    }
    
    const FSubMesh& GetSubMesh(int32 Index) const
    {
        return SubMeshes[Index];
    }

    const FAABB& GetAABB() const
    {
        return BoundingBox;
    }

    int32 GetVertexCount() const
    {
        return VertexCount;
    }
    
    int32 GetIndexCount() const
    {
        return IndexCount;
    }
    
    int32 GetNumSubMeshes() const
    {
        return SubMeshes.Size();
    }
    
    EIndexFormat GetIndexFormat() const
    {
        return IndexFormat;
    }
    
    const FString& GetName() const
    {
        return MeshName;
    }

    /** @return Returns the materials the submeshes' MaterialIndex picks from, when the mesh was loaded from an asset */
    const TArray<FMeshMaterialSlot>& GetMaterialSlots() const
    {
        return MaterialSlots;
    }

    void SetMaterialSlots(const TArray<FMeshMaterialSlot>& InMaterialSlots)
    {
        MaterialSlots = InMaterialSlots;
    }
    
private:
    bool Initialize(const FMeshData& MeshData, bool bCreateRayTracingResources);
    void CreateBoundingBox(const FMeshData& MeshData);
    bool CreateVertexStreams(const FMeshData& MeshData);

    void AddSubMesh(const FSubMesh& InSubMesh)
    {
        SubMeshes.Add(InSubMesh);
    }

    FString                              MeshName;
    FVertexDeclaration                   Declaration;
    FRHIBufferRef                        VertexStreams[VERTEX_MAX_STREAMS];
    FRHIShaderResourceViewRef            AttributeBufferSRV;
    FRHIBufferRef                        IndexBuffer;
    FRHIShaderResourceViewRef            IndexBufferSRV;
    FRHIGeometryAccelerationStructureRef RayTracingGeometry;
    EIndexFormat                         IndexFormat;
    int32                                IndexCount;
    int32                                VertexCount;
    FAABB                                BoundingBox;
    TArray<FSubMesh>                     SubMeshes;
    TArray<FMeshMaterialSlot>            MaterialSlots;
};

template<>
struct TAssetTypeOf<FMesh>
{
    static constexpr EAssetType Value = EAssetType::Mesh;
};

class ENGINE_API FModel : public FAsset
{
public:
    static TSharedPtr<FModel> Create(const FModelData& ModelData);

public:
    FModel();
    ~FModel();

    bool BuildAccelerationStructure(FRHICommandList& CommandList);
    void AddToWorld(class FWorld* World);

    TSharedPtr<FMesh> GetMesh(int32 Index) const
    {
        return Meshes[Index];
    }

    TSharedPtr<FMaterial> GetMaterial(int32 Index) const
    {
        return Materials[Index];
    }

    int32 GetNumMeshes() const
    {
        return Meshes.Size();
    }
    
    int32 GetNumMaterials() const
    {
        return Materials.Size();
    }

    const FAABB& GetAABB() const
    {
        return BoundingBox;
    }

    void SetUniformScale(float InUniformScale)
    {
        UniformScale = InUniformScale;
    }

private:
    bool Initialize(const FModelData& ModelData);

    TArray<TSharedPtr<FMesh>>     Meshes;
    TArray<TSharedPtr<FMaterial>> Materials;
    float                         UniformScale;
    FAABB                         BoundingBox;
};

template<>
struct TAssetTypeOf<FModel>
{
    static constexpr EAssetType Value = EAssetType::Model;
};
