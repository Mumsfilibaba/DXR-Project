#include "Core/Templates/NumericLimits.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "Engine/Resources/Model.h"
#include "RendererCore/TextureFactory.h"
#include "RendererCore/TextureCompressor.h"
#include "RendererCore/VertexStreamCache.h"

FMesh::FMesh()
    : Declaration()
    , VertexStreams()
    , AttributeBufferSRV(nullptr)
    , IndexBuffer(nullptr)
    , IndexBufferSRV(nullptr)
    , RayTracingGeometry(nullptr)
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

TSharedPtr<FMesh> FMesh::Create(const FMeshData& MeshData, bool bCreateRayTracingResources)
{
    TSharedPtr<FMesh> Mesh = MakeSharedPtr<FMesh>();
    if (!Mesh->Initialize(MeshData, bCreateRayTracingResources))
    {
        return nullptr;
    }

    return Mesh;
}

bool FMesh::Initialize(const FMeshData& MeshData, bool bCreateRayTracingResources)
{
    const bool bEnableRayTracing = RHI::bSupportsRayTracing;

    Declaration = MeshData.Declaration;
    VertexCount = MeshData.PackedVertexCount > 0 ? MeshData.PackedVertexCount : MeshData.Vertices.Size();
    IndexCount  = MeshData.Indices.Size();

    if (!CreateVertexStreams(MeshData))
    {
        return false;
    }


    const EBufferFlags BufferFlags = bEnableRayTracing ? EBufferFlags::ShaderResourceBuffer | EBufferFlags::Default : EBufferFlags::Default;

    // If we can get away with 16-bit indices, store them in this array
    TArray<uint16> NewIndicies;

    // Initial data
    const void* InitialIndicies = nullptr;

    IndexFormat = (IndexCount < TNumericLimits<uint16>::Max()) && !bEnableRayTracing ? EIndexFormat::uint16 : EIndexFormat::uint32;
    if (IndexFormat == EIndexFormat::uint16)
    {
        NewIndicies.Reserve(MeshData.Indices.Size());

        for (uint32 Index : MeshData.Indices)
        {
            NewIndicies.Emplace(uint16(Index));
        }

        InitialIndicies = NewIndicies.Data();
    }
    else
    {
        InitialIndicies = MeshData.Indices.Data();
    }

    const FRHIBufferDesc IndexBufferDesc = FRHIBufferDesc::CreateIndexBuffer(IndexFormat, IndexCount, BufferFlags);

    IndexBuffer = RHI::CreateBuffer(IndexBufferDesc, ERHIResourceState::IndexBuffer, InitialIndicies);
    if (!IndexBuffer)
    {
        return false;
    }
    else
    {
        IndexBuffer->SetDebugName("IndexBuffer");
    }

    if (bCreateRayTracingResources)
    {
        if (!EnsureRayTracingResources())
        {
            return false;
        }
    }
    
    // Add submeshes
    if (!MeshData.SubMeshes.IsEmpty())
    {
        SubMeshes = MeshData.SubMeshes;
    }
    else
    {
        FSubMesh WholeMesh;
        WholeMesh.VertexCount = VertexCount;
        WholeMesh.IndexCount  = IndexCount;

        AddSubMesh(WholeMesh);
    }

    MeshName = MeshData.Name;
    CreateBoundingBox(MeshData);
    return true;
}

bool FMesh::CreateVertexStreams(const FMeshData& MeshData)
{
    static const CHAR* StreamDebugNames[VERTEX_MAX_STREAMS] =
    {
        "PositionStream",
        "AttributeStream",
        "ColorStream",
        "VertexStream3"
    };

    const EBufferFlags AttributeFlags = RHI::bSupportsRayTracing ? EBufferFlags::ShaderResourceBuffer : EBufferFlags::None;

    TArray<uint8> PackedStreams[VERTEX_MAX_STREAMS];
    const bool    bUsePrePacked = MeshData.PackedVertexCount > 0;

    if (!bUsePrePacked && !MeshData.PackVertexStreams(PackedStreams))
    {
        return false;
    }

    for (uint8 StreamIndex = 0; StreamIndex < Declaration.GetNumStreams(); StreamIndex++)
    {
        const uint16 Stride = Declaration.GetStreamStride(StreamIndex);
        if (Stride == 0)
        {
            continue;
        }

        const TArray<uint8>& StreamData   = bUsePrePacked ? MeshData.PackedStreams[StreamIndex] : PackedStreams[StreamIndex];
        const int32          ExpectedSize = VertexCount * Stride;

        if (StreamData.Size() != ExpectedSize)
        {
            LOG_ERROR("Mesh '%s' carries %d bytes for stream %u but its declaration needs %d",
                MeshData.Name.Data(), StreamData.Size(), uint32(StreamIndex), ExpectedSize);
            return false;
        }

        FRHIBufferDesc StreamDesc(EBufferFlags::VertexBuffer | EBufferFlags::Default, Stride, StreamData.SizeInBytes());

        if (StreamIndex == EVertexStreamIndex::Attributes)
        {
            StreamDesc.Flags |= AttributeFlags;
        }

        VertexStreams[StreamIndex] = RHI::CreateBuffer(StreamDesc, ERHIResourceState::VertexBuffer, StreamData.Data());
        if (!VertexStreams[StreamIndex])
        {
            return false;
        }

        VertexStreams[StreamIndex]->SetDebugName(StreamDebugNames[StreamIndex]);
    }

    return true;
}

void FMesh::SetVertexBuffers(FRHICommandList& CommandList, const FVertexStreamBinding& Binding) const
{
    FRHIBuffer* Buffers[VERTEX_MAX_STREAMS];
    for (uint8 Index = 0; Index < Binding.NumStreams; Index++)
    {
        Buffers[Index] = VertexStreams[Binding.StreamIndices[Index]].Get();
        CHECK(Buffers[Index] != nullptr);
    }

    CommandList.SetVertexBuffers(MakeArrayView(Buffers, Binding.NumStreams), 0);
}

bool FMesh::EnsureRayTracingResources()
{
    if (RayTracingGeometry)
    {
        return true;
    }

    if (!RHI::bSupportsRayTracing)
    {
        return false;
    }

    FRHIBuffer* AttributeBuffer = GetAttributeBuffer();
    if (!AttributeBuffer || !IndexBuffer || IndexFormat != EIndexFormat::uint32)
    {
        LOG_ERROR("Mesh '%s' was not created ray-tracing-ready and cannot be ray traced", MeshName.Data());
        CHECK(false);
        return false;
    }

    if (!AttributeBufferSRV)
    {
        AttributeBufferSRV = RHI::CreateShaderResourceView(AttributeBuffer, FRHIShaderResourceViewDesc::CreateBuffer(0, VertexCount));
        if (!AttributeBufferSRV)
        {
            return false;
        }
    }

    if (!IndexBufferSRV)
    {
        IndexBufferSRV = RHI::CreateShaderResourceView(IndexBuffer.Get(), FRHIShaderResourceViewDesc::CreateBuffer(0, IndexCount, EBufferViewType::ByteAddress));
        if (!IndexBufferSRV)
        {
            return false;
        }
    }

    FRHIGeometryAccelerationStructureDesc GeometryDesc(GetPositionBuffer(), VertexCount, IndexBuffer.Get(), IndexCount, IndexFormat, EAccelerationStructureBuildFlags::AllowCompaction);
    RayTracingGeometry = RHI::CreateGeometryAccelerationStructure(GeometryDesc);
    if (!RayTracingGeometry)
    {
        return false;
    }

    RayTracingGeometry->SetDebugName("RayTracing Geometry");
    return true;
}

void FMesh::ReleaseRayTracingResources()
{
    RayTracingGeometry.Reset();
    AttributeBufferSRV.Reset();
    IndexBufferSRV.Reset();
}

bool FMesh::BuildAccelerationStructure(FRHICommandList& CommandList)
{
    if (!RayTracingGeometry)
    {
        return false;
    }

    FRHIGeometryAccelerationStructureBuildDesc BuildDesc;
    BuildDesc.VertexBuffer = GetPositionBuffer();
    BuildDesc.NumVertices  = VertexCount;
    BuildDesc.IndexBuffer  = IndexBuffer.Get();
    BuildDesc.NumIndices   = IndexCount;
    BuildDesc.IndexFormat  = IndexFormat;
    BuildDesc.bUpdate      = true;

    CommandList.BuildGeometryAccelerationStructure(RayTracingGeometry.Get(), BuildDesc);
    return true;
}

void FMesh::CreateBoundingBox(const FMeshData& MeshData)
{
    static constexpr const float Inf = TNumericLimits<float>::Infinity();

    Vector3 MinBounds = Vector3( Inf,  Inf,  Inf);
    Vector3 MaxBounds = Vector3(-Inf, -Inf, -Inf);

    if (MeshData.PackedVertexCount > 0)
    {
        const TArray<uint8>& PositionStream = MeshData.PackedStreams[EVertexStreamIndex::Position];
        const uint16         Stride         = Declaration.GetStreamStride(EVertexStreamIndex::Position);

        for (int32 Index = 0; Index < MeshData.PackedVertexCount; Index++)
        {
            const Vector3& Position = *reinterpret_cast<const Vector3*>(PositionStream.Data() + (Index * Stride));
            MinBounds = Vector3::Min(MinBounds, Position);
            MaxBounds = Vector3::Max(MaxBounds, Position);
        }
    }
    else
    {
        for (const FSourceVertex& Vertex : MeshData.Vertices)
        {
            MinBounds = Vector3::Min(MinBounds, Vertex.Position);
            MaxBounds = Vector3::Max(MaxBounds, Vertex.Position);
        }
    }

    BoundingBox.Max = MaxBounds;
    BoundingBox.Min = MinBounds;
}

FModel::FModel()
    : Meshes()
    , Materials()
    , UniformScale(1.0f)
    , BoundingBox()
{
}

FModel::~FModel()
{
}

TSharedRef<FModel> FModel::Create(const FModelData& ModelData)
{
    TSharedRef<FModel> Model = new FModel();
    if (!Model->Initialize(ModelData))
    {
        return nullptr;
    }

    return Model;
}

bool FModel::Initialize(const FModelData& ModelData)
{
    UniformScale = ModelData.Scale;
    
    const int32 NumMeshes = ModelData.Meshes.Size();
    Meshes.Reserve(NumMeshes);
    
    for (int32 Index = 0; Index < NumMeshes; Index++)
    {
        TSharedPtr<FMesh> Mesh = FMesh::Create(ModelData.Meshes[Index]);
        if (!Mesh)
        {
            return false;
        }
        
        Meshes.Add(Mesh);
    }
    
    if (Meshes.Size() != ModelData.Meshes.Size())
    {
        DEBUG_BREAK();
        return false;
    }
    
    const int32 NumMaterials = ModelData.Materials.Size();
    Materials.Reserve(NumMaterials);
    
    const auto GetRHITexture = [=](const FModelData& InModelData, EMaterialTexture::Type MaterialTexture, int32 MaterialIndex)
    {
        const FTexture2DRef Texture = InModelData.Materials[MaterialIndex].Textures[MaterialTexture];
        return Texture ? Texture->GetRHITexture() : FEngine::Get()->BaseTexture;
    };
    
    for (int32 Index = 0; Index < NumMaterials; Index++)
    {
        FMaterialInfo MaterialInfo;
        MaterialInfo.Albedo           = FFloatColor(ModelData.Materials[Index].Diffuse);
        MaterialInfo.AmbientOcclusion = ModelData.Materials[Index].AmbientFactor;
        MaterialInfo.Metallic         = ModelData.Materials[Index].Metallic;
        MaterialInfo.Roughness        = ModelData.Materials[Index].Roughness;
        MaterialInfo.MaterialFlags    = ModelData.Materials[Index].MaterialFlags & (EMaterialFlags::EnableHeight | EMaterialFlags::EnableAlpha | EMaterialFlags::EnableNormalMapping | EMaterialFlags::DoubleSided | EMaterialFlags::ForceForwardPass);
        
        if (ModelData.Materials[Index].Textures[EMaterialTexture::Normal])
        {
            MaterialInfo.MaterialFlags |= EMaterialFlags::EnableNormalMapping;
        }

        TSharedPtr<FMaterial> Material = MakeSharedPtr<FMaterial>(MaterialInfo);
        Material->SetTexture(EMaterialTextureSlot::BaseColor, GetRHITexture(ModelData, EMaterialTexture::Diffuse, Index));

        if (ModelData.Materials[Index].Textures[EMaterialTexture::Normal])
        {
            Material->SetTexture(EMaterialTextureSlot::Normal, ModelData.Materials[Index].Textures[EMaterialTexture::Normal]->GetRHITexture());
        }

        FMaterialMaskSlots Masks(*Material);
        const auto SlotForSourceTexture = [&](EMaterialTexture::Type SourceTexture, const FTexture2DRef& Texture) -> EMaterialTextureSlot::Type
        {
            switch (SourceTexture)
            {
                case EMaterialTexture::Diffuse: return EMaterialTextureSlot::BaseColor;
                case EMaterialTexture::Normal:  return Texture ? EMaterialTextureSlot::Normal : EMaterialTextureSlot::Count;
                default:                        return Masks.Assign(Texture ? Texture->GetRHITexture() : FRHITextureRef());
            }
        };

        for (uint32 Scalar = 0; Scalar < EMaterialScalar::Count; ++Scalar)
        {
            const FMaterialSourceRoute& Source = ModelData.Materials[Index].Routes[Scalar];
            if (!Source.IsRouted())
            {
                Material->SetRoute(EMaterialScalar::Type(Scalar), FMaterialTextureRoute());
                continue;
            }

            const EMaterialTextureSlot::Type Slot = SlotForSourceTexture(Source.Texture, ModelData.Materials[Index].Textures[Source.Texture]);
            Material->SetRoute(EMaterialScalar::Type(Scalar), FMaterialTextureRoute(Slot, Source.Channel, Source.bInvert));
        }

        const FMaterialTextureRoute& OpacityRoute = Material->GetRoute(EMaterialScalar::Opacity);
        if (OpacityRoute.IsRouted() && OpacityRoute.Slot != EMaterialTextureSlot::BaseColor)
        {
            Material->EnableAlphaMask(true);
        }

        // Block-compress uncompressed textures for reduced memory usage
        FTextureCompressor& Compressor = FTextureFactory::Get().GetTextureCompressor();

        auto TryCompressBC1 = [&](FRHITextureRef& Texture)
        {
            if (Texture && !IsBlockCompressed(Texture->GetDesc().Format) && IsBlockCompressedAligned(Texture->GetDesc().Extent.X) && IsBlockCompressedAligned(Texture->GetDesc().Extent.Y))
            {
                FRHITextureRef Compressed;
                if (Compressor.CompressBC1(Texture, Compressed))
                {
                    LOG_INFO("[FModel] Compressed texture (%dx%d) %s -> BC1", Texture->GetDesc().Extent.X, Texture->GetDesc().Extent.Y, ToString(Texture->GetDesc().Format));
                    Texture = Compressed;
                }
            }
        };

        auto TryCompressBC3 = [&](FRHITextureRef& Texture)
        {
            if (Texture && !IsBlockCompressed(Texture->GetDesc().Format) && IsBlockCompressedAligned(Texture->GetDesc().Extent.X) && IsBlockCompressedAligned(Texture->GetDesc().Extent.Y))
            {
                FRHITextureRef Compressed;
                if (Compressor.CompressBC3(Texture, Compressed))
                {
                    LOG_INFO("[FModel] Compressed texture (%dx%d) %s -> BC3", Texture->GetDesc().Extent.X, Texture->GetDesc().Extent.Y, ToString(Texture->GetDesc().Format));
                    Texture = Compressed;
                }
            }
        };

        auto TryCompressBC5 = [&](FRHITextureRef& Texture)
        {
            if (Texture && !IsBlockCompressed(Texture->GetDesc().Format) && IsBlockCompressedAligned(Texture->GetDesc().Extent.X) && IsBlockCompressedAligned(Texture->GetDesc().Extent.Y))
            {
                FRHITextureRef Compressed;
                if (Compressor.CompressBC5(Texture, Compressed))
                {
                    LOG_INFO("[FModel] Compressed texture (%dx%d) %s -> BC5", Texture->GetDesc().Extent.X, Texture->GetDesc().Extent.Y, ToString(Texture->GetDesc().Format));
                    Texture = Compressed;
                }
            }
        };

        if (Material->HasAlphaMask() && Material->GetRoute(EMaterialScalar::Opacity).Slot == EMaterialTextureSlot::BaseColor)
        {
            TryCompressBC3(Material->GetTexture(EMaterialTextureSlot::BaseColor));
        }
        else
        {
            TryCompressBC1(Material->GetTexture(EMaterialTextureSlot::BaseColor));
        }
        
        TryCompressBC5(Material->GetTexture(EMaterialTextureSlot::Normal));

        for (uint32 Slot = EMaterialTextureSlot::MaskA; Slot < EMaterialTextureSlot::Count; ++Slot)
        {
            TryCompressBC1(Material->GetTexture(EMaterialTextureSlot::Type(Slot)));
        }

        Material->Initialize();
        Material->SetName(ModelData.Materials[Index].Name);
        
        Materials.Add(Material);
    }
    
    if (Materials.Size() != ModelData.Materials.Size())
    {
        DEBUG_BREAK();
        return false;
    }
    
    return true;
}

bool FModel::BuildAccelerationStructure(FRHICommandList& CommandList)
{
    for (const TSharedPtr<FMesh>& Mesh : Meshes)
    {
        if (!Mesh->BuildAccelerationStructure(CommandList))
        {
            return false;
        }
    }
    
    return true;
}

void FModel::AddToWorld(FWorld* World)
{
    const int32 NumMeshes = Meshes.Size();
    for (int32 MeshIdx = 0; MeshIdx < NumMeshes; MeshIdx++)
    {
        const TSharedPtr<FMesh>& Mesh = Meshes[MeshIdx];
        if (Mesh)
        {
            FActor* NewActor = World->CreateActor();
            NewActor->SetName(Mesh->GetName());
            NewActor->GetTransform().SetUniformScale(UniformScale);

            FStaticMeshComponent* MeshComponent = NewObject<FStaticMeshComponent>();
            if (MeshComponent)
            {
                MeshComponent->SetMesh(Mesh);

                const int32 NumSubMeshes = Mesh->GetNumSubMeshes();
                if (NumSubMeshes > 0)
                {
                    for (int32 SubMeshIdx = 0; SubMeshIdx < NumSubMeshes; SubMeshIdx++)
                    {
                        const FSubMesh& SubMesh = Mesh->GetSubMesh(SubMeshIdx);
                        if (SubMesh.MaterialIndex >= 0 && Materials.Size() >= SubMesh.MaterialIndex)
                        {
                            const TSharedPtr<FMaterial>& Material = Materials[SubMesh.MaterialIndex];
                            MeshComponent->SetMaterial(Material, SubMeshIdx);
                            CHECK(MeshComponent->GetMaterial(SubMeshIdx) == Material);
                        }
                        else
                        {
                            MeshComponent->SetMaterial(FEngine::Get()->BaseMaterial, SubMeshIdx);
                        }
                    }
                }
                else
                {
                    if (!Materials.IsEmpty())
                    {
                        MeshComponent->SetMaterial(Materials[0]);
                    }
                    else
                    {
                        MeshComponent->SetMaterial(FEngine::Get()->BaseMaterial);
                    }
                }
                
                NewActor->AddComponent(MeshComponent);
            }
        }
    }
}
