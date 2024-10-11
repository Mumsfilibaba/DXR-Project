#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Math/AABB.h"
#include "RHI/RHIResources.h"
#include "RHI/RHICommandList.h"
#include "Engine/EngineModule.h"
#include "Engine/Assets/MeshFactory.h"
#include "Engine/Assets/Resource.h"

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

enum class EVertexStream
{
    Packed    = 0,
    Positions = 1,
    Normals   = 2,
    TexCoords = 3,
};

class ENGINE_API FMesh
{
public:
    static TSharedPtr<FMesh> Create(const FMeshData& Data);
    
    FMesh();
    ~FMesh();

    bool Init(const FMeshData& Data);
    bool BuildAccelerationStructure(FRHICommandList& CommandList);
    
    FRHIBuffer* GetVertexBuffer(EVertexStream VertexStream)const;
    FRHIShaderResourceView* GetVertexBufferSRV(EVertexStream VertexStream) const;
    
    FRHIBuffer* GetIndexBuffer() const
    {
        return IndexBuffer.Get();
    }
    
    FRHIShaderResourceView* GetIndexBufferSRV() const
    {
        return IndexBufferSRV.Get();
    }
    
    FRHIRayTracingGeometry* GetRayTracingGeometry() const
    {
        return RTGeometry.Get();
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
    
    const FString& GetName() const
    {
        return MeshName;
    }
    
private:
    void CreateBoundingBox(const FMeshData& Data);

    FString                   MeshName;
    FRHIBufferRef             VertexBuffer;
    FRHIShaderResourceViewRef VertexBufferSRV;
    FRHIBufferRef             VertexPositionBuffer;
    FRHIShaderResourceViewRef VertexPositionBufferSRV;
    FRHIBufferRef             VertexNormalBuffer;
    FRHIShaderResourceViewRef VertexNormalBufferSRV;
    FRHIBufferRef             VertexTexCoordBuffer;
    FRHIShaderResourceViewRef VertexTexCoordBufferSRV;
    FRHIBufferRef             IndexBuffer;
    FRHIShaderResourceViewRef IndexBufferSRV;
    FRHIRayTracingGeometryRef RTGeometry;
    EIndexFormat              IndexFormat;
    int32                     IndexCount;
    int32                     VertexCount;
    FAABB                     BoundingBox;
    TArray<FSubMesh>          SubMeshes;
};

class ENGINE_API FModel : public FResource
{
public:
    FModel();
    ~FModel();
    
    bool Init(const TSharedPtr<FImportedModel>& ImportedModel);
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
    
    int32 GetNumMeshes()    const { return Meshes.Size(); }
    int32 GetNumMaterials() const { return Materials.Size(); }
    
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
