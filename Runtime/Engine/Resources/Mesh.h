#pragma once
#include "Engine/EngineModule.h"
#include "Engine/Assets/MeshFactory.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Math/AABB.h"
#include "RHI/RHIResources.h"
#include "RHI/RHICommandList.h"

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
};

class ENGINE_API FMesh
{
public:
    static TSharedPtr<FMesh> Create(const FMeshData& Data);
    
    FMesh();
    ~FMesh();

    bool Init(const FMeshData& Data);
    bool BuildAccelerationStructure(FRHICommandList& CommandList);
    
    void AddSubMesh(const FSubMesh& InSubMesh)
    {
        SubMeshes.Add(InSubMesh);
    }

    int32 GetNumSubMeshes() const
    {
        return SubMeshes.Size();
    }
    
public:
    void CreateBoundingBox(const FMeshData& Data);

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
    uint32                    IndexCount;
    uint32                    VertexCount;
    FAABB                     BoundingBox;
    TArray<FSubMesh>          SubMeshes;
};
