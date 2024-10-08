#include "Mesh.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "Core/Templates/NumericLimits.h"

FMesh::FMesh()
    : VertexBuffer(nullptr)
    , PosOnlyVertexBuffer(nullptr)
    , VertexBufferSRV(nullptr)
    , IndexBuffer(nullptr)
    , IndexBufferSRV(nullptr)
    , RTGeometry(nullptr)
    , VertexCount(0)
    , IndexFormat(EIndexFormat::Unknown)
    , IndexCount(0)
    , BoundingBox()
{
}

bool FMesh::Init(const FMeshData& Data)
{
    const bool bEnableRayTracing = false; //GRHISupportsRayTracing ;

    VertexCount = static_cast<uint32>(Data.Vertices.Size());
    IndexCount  = static_cast<uint32>(Data.Indices.Size());

    const EBufferUsageFlags BufferFlags = bEnableRayTracing ? EBufferUsageFlags::ShaderResource | EBufferUsageFlags::Default : EBufferUsageFlags::Default;

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

    // Create VertexBuffer with only positions
    TArray<FVector3> ShadowVertices(VertexCount);
    for (uint32 Index = 0; Index < VertexCount; Index++)
    {
        const FVertex& Vertex = Data.Vertices[Index];
        ShadowVertices[Index] = Vertex.Position;
    }

    VBInfo = FRHIBufferInfo(VertexCount * sizeof(FVector3), sizeof(FVector3), BufferFlags | EBufferUsageFlags::VertexBuffer);
    PosOnlyVertexBuffer = RHICreateBuffer(VBInfo, EResourceAccess::VertexBuffer, ShadowVertices.Data());
    if (!PosOnlyVertexBuffer)
    {
        return false;
    }
    else
    {
        PosOnlyVertexBuffer->SetDebugName("Position Only VertexBuffer");
    }

    // Create VertexBuffer with only positions and texcoords
    TArray<FVertexMasked> MaskedVertices(VertexCount);
    for (uint32 Index = 0; Index < VertexCount; Index++)
    {
        const FVertex& Vertex = Data.Vertices[Index];
        MaskedVertices[Index].Position = Vertex.Position;
        MaskedVertices[Index].TexCoord = Vertex.TexCoord;
    }

    VBInfo = FRHIBufferInfo(VertexCount * sizeof(FVertexMasked), sizeof(FVertexMasked), BufferFlags | EBufferUsageFlags::VertexBuffer);
    MaskedVertexBuffer = RHICreateBuffer(VBInfo, EResourceAccess::VertexBuffer, MaskedVertices.Data());
    if (!MaskedVertexBuffer)
    {
        return false;
    }
    else
    {
        MaskedVertexBuffer->SetDebugName("Masked VertexBuffer");
    }

    // If we can get away with 16-bit indices, store them in this array
    TArray<uint16> NewIndicies;

    // Initial data
    const void* InitialIndicies = nullptr;

    IndexFormat = IndexCount < TNumericLimits<uint16>::Max() && !bEnableRayTracing ? EIndexFormat::uint16 : EIndexFormat::uint32;
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
