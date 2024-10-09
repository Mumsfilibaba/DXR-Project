#include "Mesh.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "Core/Templates/NumericLimits.h"

FMesh::FMesh()
    : VertexBuffer(nullptr)
    , VertexBufferSRV(nullptr)
    , VertexPositionBuffer(nullptr)
    , VertexPositionBufferSRV(nullptr)
    , VertexNormalBuffer(nullptr)
    , VertexNormalBufferSRV(nullptr)
    , VertexTexCoordBuffer(nullptr)
    , VertexTexCoordBufferSRV(nullptr)
    , IndexBuffer(nullptr)
    , IndexBufferSRV(nullptr)
    , RTGeometry(nullptr)
    , IndexFormat(EIndexFormat::Unknown)
    , IndexCount()
    , VertexCount(0)
    , BoundingBox()
    , SubMeshes()
{
}

FMesh::~FMesh()
{
}

bool FMesh::Init(const FMeshData& Data)
{
    const bool bEnableRayTracing = false; //GRHISupportsRayTracing ;

    VertexCount = static_cast<uint32>(Data.Vertices.Size());
    IndexCount  = static_cast<uint32>(Data.Indices.Size());

    const EBufferUsageFlags BufferFlags = bEnableRayTracing ? EBufferUsageFlags::ShaderResource | EBufferUsageFlags::Default : EBufferUsageFlags::Default;

    // Create VertexBuffer
    FRHIBufferInfo VBInfo(VertexCount * sizeof(FVertex), sizeof(FVertex), BufferFlags | EBufferUsageFlags::VertexBuffer);
    VertexBuffer = RHICreateBuffer(VBInfo, EResourceAccess::VertexBuffer, Data.Vertices.Data());
    if (!VertexBuffer)
    {
        return false;
    }
    else
    {
        VertexBuffer->SetDebugName("VertexBuffer");
    }

    // Create VertexPositionBuffer
    TArray<FVertexPosition> VertexPositions(VertexCount);
    for (uint32 Index = 0; Index < VertexCount; Index++)
    {
        const FVertex& Vertex = Data.Vertices[Index];
        VertexPositions[Index] = Vertex.Position;
    }

    VBInfo = FRHIBufferInfo(VertexCount * sizeof(FVertexPosition), sizeof(FVertexPosition), BufferFlags | EBufferUsageFlags::VertexBuffer);
    VertexPositionBuffer = RHICreateBuffer(VBInfo, EResourceAccess::VertexBuffer, VertexPositions.Data());
    if (!VertexPositionBuffer)
    {
        return false;
    }
    else
    {
        VertexPositionBuffer->SetDebugName("VertexPositionBuffer");
    }

    // Create VertexNormalBuffer
    TArray<FVertexNormal> VertexNormals(VertexCount);
    for (uint32 Index = 0; Index < VertexCount; Index++)
    {
        const FVertex& Vertex = Data.Vertices[Index];
        VertexNormals[Index] = FVertexNormal(Vertex.Normal, Vertex.Tangent);
    }

    VBInfo = FRHIBufferInfo(VertexCount * sizeof(FVertexNormal), sizeof(FVertexNormal), BufferFlags | EBufferUsageFlags::VertexBuffer);
    VertexNormalBuffer = RHICreateBuffer(VBInfo, EResourceAccess::VertexBuffer, VertexNormals.Data());
    if (!VertexNormalBuffer)
    {
        return false;
    }
    else
    {
        VertexNormalBuffer->SetDebugName("VertexNormalBuffer");
    }
    
    // Create VertexTexCoordBuffer
    TArray<FVertexTexCoord> VertexTexCoords(VertexCount);
    for (uint32 Index = 0; Index < VertexCount; Index++)
    {
        const FVertex& Vertex = Data.Vertices[Index];
        VertexTexCoords[Index] = Vertex.TexCoord;
    }

    VBInfo = FRHIBufferInfo(VertexCount * sizeof(FVertexTexCoord), sizeof(FVertexTexCoord), BufferFlags | EBufferUsageFlags::VertexBuffer);
    VertexTexCoordBuffer = RHICreateBuffer(VBInfo, EResourceAccess::VertexBuffer, VertexTexCoords.Data());
    if (!VertexTexCoordBuffer)
    {
        return false;
    }
    else
    {
        VertexTexCoordBuffer->SetDebugName("VertexTexCoordBuffer");
    }

    // If we can get away with 16-bit indices, store them in this array
    TArray<uint16> NewIndicies;

    // Initial data
    const void* InitialIndicies = nullptr;

    IndexFormat = (IndexCount < TNumericLimits<uint16>::Max()) && !bEnableRayTracing ? EIndexFormat::uint16 : EIndexFormat::uint32;
    if (IndexFormat == EIndexFormat::uint16)
    {
        NewIndicies.Reserve(Data.Indices.Size());

        for (uint32 Index : Data.Indices)
        {
            NewIndicies.Emplace(uint16(Index));
        }

        InitialIndicies = NewIndicies.Data();
    }
    else
    {
        InitialIndicies = Data.Indices.Data();
    }

    FRHIBufferInfo IBInfo(IndexCount * GetStrideFromIndexFormat(IndexFormat), GetStrideFromIndexFormat(IndexFormat), BufferFlags | EBufferUsageFlags::IndexBuffer);
    IndexBuffer = RHICreateBuffer(IBInfo, EResourceAccess::IndexBuffer, InitialIndicies);
    if (!IndexBuffer)
    {
        return false;
    }
    else
    {
        IndexBuffer->SetDebugName("IndexBuffer");
    }

    if (bEnableRayTracing)
    {
        FRHIRayTracingGeometryDesc GeometryInitializer(VertexBuffer.Get(), VertexCount, IndexBuffer.Get(), IndexCount, IndexFormat, EAccelerationStructureBuildFlags::None);
        RTGeometry = RHICreateRayTracingGeometry(GeometryInitializer);
        if (!RTGeometry)
        {
            return false;
        }
        else
        {
            RTGeometry->SetDebugName("RayTracing Geometry");
        }

        FRHIBufferSRVDesc SRVInitializer(VertexBuffer.Get(), 0, VertexCount);
        VertexBufferSRV = RHICreateShaderResourceView(SRVInitializer);
        if (!VertexBufferSRV)
        {
            return false;
        }
        
        SRVInitializer = FRHIBufferSRVDesc(VertexPositionBuffer.Get(), 0, VertexCount);
        VertexPositionBufferSRV = RHICreateShaderResourceView(SRVInitializer);
        if (!VertexPositionBufferSRV)
        {
            return false;
        }
        
        SRVInitializer = FRHIBufferSRVDesc(VertexNormalBuffer.Get(), 0, VertexCount);
        VertexNormalBufferSRV = RHICreateShaderResourceView(SRVInitializer);
        if (!VertexNormalBufferSRV)
        {
            return false;
        }
        
        SRVInitializer = FRHIBufferSRVDesc(VertexTexCoordBuffer.Get(), 0, VertexCount);
        VertexTexCoordBufferSRV = RHICreateShaderResourceView(SRVInitializer);
        if (!VertexTexCoordBufferSRV)
        {
            return false;
        }

        SRVInitializer = FRHIBufferSRVDesc(IndexBuffer.Get(), 0, IndexCount, EBufferSRVFormat::Uint32);
        IndexBufferSRV = RHICreateShaderResourceView(SRVInitializer);
        if (!IndexBufferSRV)
        {
            return false;
        }
    }
    
    // Add submeshes
    if (!Data.Partitions.IsEmpty())
    {
        SubMeshes.Reserve(Data.Partitions.Size());
        
        for (const FMeshPartition& MeshPartition : Data.Partitions)
        {
            FSubMesh NewSubMesh;
            NewSubMesh.BaseVertex  = MeshPartition.BaseVertex;
            NewSubMesh.VertexCount = MeshPartition.VertexCount;
            NewSubMesh.StartIndex  = MeshPartition.StartIndex;
            NewSubMesh.IndexCount  = MeshPartition.IndexCount;
            
            AddSubMesh(NewSubMesh);
        }
    }
    else
    {
        SubMeshes.Reserve(1);
        
        FSubMesh NewSubMesh;
        NewSubMesh.BaseVertex  = 0;
        NewSubMesh.VertexCount = VertexCount;
        NewSubMesh.StartIndex  = 0;
        NewSubMesh.IndexCount  = IndexCount;
        
        AddSubMesh(NewSubMesh);
    }

    CreateBoundingBox(Data);
    return true;
}

bool FMesh::BuildAccelerationStructure(FRHICommandList& CommandList)
{
    FRayTracingGeometryBuildInfo BuildInfo;
    BuildInfo.VertexBuffer = VertexBuffer.Get();
    BuildInfo.NumVertices  = VertexCount;
    BuildInfo.IndexBuffer  = IndexBuffer.Get();
    BuildInfo.NumIndices   = IndexCount;
    BuildInfo.IndexFormat  = IndexFormat;
    BuildInfo.bUpdate      = true;

    CommandList.BuildRayTracingGeometry(RTGeometry.Get(), BuildInfo);
    return true;
}

TSharedPtr<FMesh> FMesh::Create(const FMeshData& Data)
{
    TSharedPtr<FMesh> Result = MakeShared<FMesh>();
    if (Result->Init(Data))
    {
        return Result;
    }
    else
    {
        return TSharedPtr<FMesh>();
    }
}

void FMesh::CreateBoundingBox(const FMeshData& Data)
{
    constexpr float Inf = TNumericLimits<float>::Infinity();

    FVector3 MinBounds = FVector3( Inf,  Inf,  Inf);
    FVector3 MaxBounds = FVector3(-Inf, -Inf, -Inf);

    for (const FVertex& Vertex : Data.Vertices)
    {
        MinBounds = Min(MinBounds, Vertex.Position);
        MaxBounds = Max(MaxBounds, Vertex.Position);
    }

    BoundingBox.Top    = MaxBounds;
    BoundingBox.Bottom = MinBounds;
}
