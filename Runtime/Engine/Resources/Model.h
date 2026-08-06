#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Math/AABB.h"
#include "RHI/RHIResources.h"
#include "RHI/RHICommandList.h"
#include "Engine/EngineModule.h"
#include "Engine/Resources/Resource.h"
#include "Engine/Assets/ModelCreateInfo.h"
#include "RendererCore/VertexDeclaration.h"

struct FVertexStreamBinding;

struct FSubMesh
{
    FSubMesh()
        : BaseVertex(0)
        , VertexCount(0)
        , StartIndex(0)
        , IndexCount(0)
    {
    }
    
    uint32 BaseVertex;
    uint32 VertexCount;
    uint32 StartIndex;
    uint32 IndexCount;
    int32  MaterialIndex;
};

class ENGINE_API FMesh
{
public:
    FMesh();
    ~FMesh();

    bool Init(const FMeshCreateInfo& CreateInfo, bool bCreateRayTracingResources = false);
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
    
    void AddSubMesh(const FSubMesh& InSubMesh)
    {
        SubMeshes.Add(InSubMesh);
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
    void CreateBoundingBox(const FMeshCreateInfo& CreateInfo);
    bool CreateVertexStreams(const FMeshCreateInfo& CreateInfo);

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
    FModel();
    ~FModel();

    bool Init(const FModelCreateInfo& CreateInfo);
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
    TArray<TSharedPtr<FMesh>>     Meshes;
    TArray<TSharedPtr<FMaterial>> Materials;
    float                         UniformScale;
    FAABB                         BoundingBox;
};
