#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Math/AABB.h"
#include "RHI/RHIResources.h"
#include "RHI/RHICommandList.h"
#include "Engine/EngineModule.h"
#include "Engine/Resources/Resource.h"
#include "Engine/Assets/MeshData.h"
#include "Engine/Assets/ModelData.h"
#include "RendererCore/VertexDeclaration.h"

struct FVertexStreamBinding;

class ENGINE_API FMesh
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
    
    const String& GetName() const
    {
        return MeshName;
    }
    
private:
    bool Initialize(const FMeshData& MeshData, bool bCreateRayTracingResources);
    void CreateBoundingBox(const FMeshData& MeshData);
    bool CreateVertexStreams(const FMeshData& MeshData);

    void AddSubMesh(const FSubMesh& InSubMesh)
    {
        SubMeshes.Add(InSubMesh);
    }

    String                               MeshName;
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
};

class ENGINE_API FModel : public FResource
{
public:
    static TSharedRef<FModel> Create(const FModelData& ModelData);

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
