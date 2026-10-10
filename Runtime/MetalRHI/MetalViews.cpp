#include "MetalRHI/MetalViews.h"
#include "MetalRHI/MetalBindlessDescriptors.h"
#include "MetalRHI/MetalBuffer.h"
#include "MetalRHI/MetalCapabilities.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalRayTracing.h"
#include "MetalRHI/MetalResidencySet.h"
#include "MetalRHI/MetalRHI.h"
#include "MetalRHI/MetalTexture.h"
#include "Core/Math/Math.h"
#include "Core/Threading/ScopedLock.h"

struct FMetalSubresourceRange
{
    uint32 FirstMip   = 0;
    uint32 NumMips    = 1;
    uint32 FirstSlice = 0;
    uint32 NumSlices  = 1;
};

template<typename ViewDescType>
static FMetalSubresourceRange ResolveViewRange(const ViewDescType& InDesc)
{
    FMetalSubresourceRange Range;
    auto ResolveMips = [&Range](const auto& DimensionDesc)
    {
        if constexpr (TIsSame<ViewDescType, FRHIShaderResourceViewDesc>::Value)
        {
            Range.FirstMip = DimensionDesc.FirstMipLevel;
            Range.NumMips  = DimensionDesc.NumMips;
        }
        else
        {
            Range.FirstMip = DimensionDesc.MipLevel;
        }
    };

    switch (InDesc.ViewDimension)
    {
        case EViewDimension::Texture1D:
            ResolveMips(InDesc.Texture1D);
            break;

        case EViewDimension::Texture1DArray:
            ResolveMips(InDesc.Texture1DArray);
            Range.FirstSlice = InDesc.Texture1DArray.FirstArraySlice;
            Range.NumSlices  = InDesc.Texture1DArray.NumSlices;
            break;

        case EViewDimension::Texture2D:
            ResolveMips(InDesc.Texture2D);
            break;

        case EViewDimension::Texture2DArray:
            ResolveMips(InDesc.Texture2DArray);
            Range.FirstSlice = InDesc.Texture2DArray.FirstArraySlice;
            Range.NumSlices  = InDesc.Texture2DArray.NumSlices;
            break;

        case EViewDimension::TextureCube:
            ResolveMips(InDesc.TextureCube);
            Range.NumSlices = RHI_NUM_CUBE_FACES;
            break;

        case EViewDimension::TextureCubeArray:
            ResolveMips(InDesc.TextureCubeArray);
            Range.FirstSlice = InDesc.TextureCubeArray.FirstCube * RHI_NUM_CUBE_FACES;
            Range.NumSlices  = InDesc.TextureCubeArray.NumCubes * RHI_NUM_CUBE_FACES;
            break;

        case EViewDimension::Texture3D:
            ResolveMips(InDesc.Texture3D);
            break;

        default:
            break;
    }

    return Range;
}

static uint32 ResolveBufferViewStride(const FRHIBufferDesc& BufferDesc, EBufferViewType ViewType, EFormat ViewFormat)
{
    switch (ViewType)
    {
        case EBufferViewType::ByteAddress: return sizeof(uint32);
        case EBufferViewType::Typed:       return GetByteStrideFromFormat(ViewFormat);
        default:                           return BufferDesc.Stride;
    }
}

template<typename ViewDescType>
static FMetalSubresource ResolveAttachmentSubresource(const ViewDescType& InDesc)
{
    FMetalSubresource Result;
    switch (InDesc.ViewDimension)
    {
        case EViewDimension::Texture1D:
            Result.MipLevel = InDesc.Texture1D.MipLevel;
            break;

        case EViewDimension::Texture1DArray:
            Result.MipLevel   = InDesc.Texture1DArray.MipLevel;
            Result.ArrayIndex = InDesc.Texture1DArray.FirstArraySlice;
            Result.NumSlices  = InDesc.Texture1DArray.NumSlices;
            break;

        case EViewDimension::Texture2D:
            Result.MipLevel = InDesc.Texture2D.MipLevel;
            break;

        case EViewDimension::Texture2DArray:
            Result.MipLevel   = InDesc.Texture2DArray.MipLevel;
            Result.ArrayIndex = InDesc.Texture2DArray.FirstArraySlice;
            Result.NumSlices  = InDesc.Texture2DArray.NumSlices;
            break;

        case EViewDimension::TextureCube:
            Result.MipLevel  = InDesc.TextureCube.MipLevel;
            Result.NumSlices = RHI_NUM_CUBE_FACES;
            break;

        case EViewDimension::TextureCubeArray:
            Result.MipLevel   = InDesc.TextureCubeArray.MipLevel;
            Result.ArrayIndex = InDesc.TextureCubeArray.FirstCube * RHI_NUM_CUBE_FACES;
            Result.NumSlices  = InDesc.TextureCubeArray.NumCubes * RHI_NUM_CUBE_FACES;
            break;

        case EViewDimension::Texture3D:
            if constexpr (TIsSame<ViewDescType, FRHIRenderTargetViewDesc>::Value)
            {
                Result.MipLevel   = InDesc.Texture3D.MipLevel;
                Result.ArrayIndex = InDesc.Texture3D.FirstWSlice;
            }
            break;

        default:
            break;
    }

    Result.NumSlices = Math::Max<uint16>(Result.NumSlices, 1);
    return Result;
}

FMetalView::FMetalView(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
    , SourceBuffer(nullptr)
    , SourceTexture(nullptr)
    , AccelerationStructureSource(nullptr)
    , ResidencyEntry(nullptr)
    , PinnedEntry(nullptr)
    , TextureView(nil)
    , BufferView(nil)
    , BufferOffset(0)
    , SourceOffset(0)
    , BufferSize(0)
    , BindlessHandle()
    , BufferTextureFormat(EFormat::Unknown)
    , TextureViewFormat(MTLPixelFormatInvalid)
    , TextureViewType(MTLTextureType2D)
    , TextureViewLevels(NSMakeRange(0, 0))
    , TextureViewSlices(NSMakeRange(0, 0))
    , bTextureViewIsParent(false)
    , bBufferTextureWritable(false)
    , bBindlessWritable(false)
    , bOwnsTextureView(false)
    , bDeclaredResident(false)
{
}

FMetalView::~FMetalView()
{
    if (FMetalRelocatable* Source = GetSource())
    {
        Source->RemoveRelocationListener(this);
        SourceBuffer  = nullptr;
        SourceTexture = nullptr;
    }

    FreeBindlessHandle();
    ReleaseTextureView();

    if (BufferView)
    {
        FMetalDeviceRHI::DeferDeletion(BufferView);
        [BufferView release];
        BufferView = nil;
    }
}

void FMetalView::ReleaseTextureView()
{
    if (!TextureView)
    {
        return;
    }

    if (bDeclaredResident)
    {
        FMetalDeviceRHI::DeferDeletion(FMetalDeferredObject::EType::StandaloneResource, TextureView);
    }
    else
    {
        FMetalDeviceRHI::DeferDeletion(TextureView);
    }

    [TextureView release];
    TextureView       = nil;
    bOwnsTextureView  = false;
    bDeclaredResident = false;
}

void FMetalView::FreeBindlessHandle()
{
    if (!BindlessHandle.IsValid())
    {
        return;
    }

    if (FMetalBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager())
    {
        BindlessManager->Free(BindlessHandle);
    }

    GetDevice()->GetResidencyManager().Unpin(PinnedEntry);
    BindlessHandle = FRHIDescriptorHandle();
    PinnedEntry    = nullptr;
}

FMetalRelocatable* FMetalView::GetSource() const
{
    if (SourceBuffer)
    {
        return SourceBuffer;
    }

    return SourceTexture;
}

FRHIDescriptorHandle FMetalView::EnsureBindlessHandle(EDescriptorType DescriptorType, bool bWritable) const
{
    if (BindlessHandle.IsValid())
    {
        return BindlessHandle;
    }

    FMetalBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager();

    if (!BindlessManager || !BindlessManager->IsEnabled())
    {
        return FRHIDescriptorHandle();
    }

    auto AllocateHandle = [&]() -> FRHIDescriptorHandle
    {
        if (BindlessHandle.IsValid())
        {
            return BindlessHandle;
        }

        BindlessHandle = BindlessManager->Allocate(DescriptorType);

        if (!BindlessHandle.IsValid())
        {
            return FRHIDescriptorHandle();
        }

        bBindlessWritable = bWritable;
        WriteBindlessHandle();

        PinnedEntry = ResidencyEntry;
        GetDevice()->GetResidencyManager().Pin(PinnedEntry);
        return BindlessHandle;
    };

    if (FMetalRelocatable* Source = GetSource())
    {
        TScopedLock Lock(Source->GetRelocationLock());
        return AllocateHandle();
    }

    if (AccelerationStructureSource)
    {
        TScopedLock Lock(AccelerationStructureSource->GetBindlessLock());
        return AllocateHandle();
    }

    return AllocateHandle();
}

void FMetalView::WriteBindlessHandle() const
{
    FMetalBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager();

    if (!BindlessHandle.IsValid() || !BindlessManager)
    {
        return;
    }

    if (TextureView)
    {
        BindlessManager->WriteTexture(BindlessHandle, TextureView, bBindlessWritable, true);
    }
    else if (BufferView)
    {
        BindlessManager->WriteBuffer(BindlessHandle, BufferView, BufferOffset, false, true);
    }
    else if (AccelerationStructureSource)
    {
        BindlessManager->WriteAccelerationStructure(BindlessHandle, AccelerationStructureSource->GetMTLAccelerationStructure(), true);
    }
}

void FMetalView::RefreshBindlessHandle()
{
    if (!AccelerationStructureSource)
    {
        return;
    }

    TScopedLock Lock(AccelerationStructureSource->GetBindlessLock());
    WriteBindlessHandle();
}

id<MTLAccelerationStructure> FMetalView::GetMTLAccelerationStructure() const
{
    return AccelerationStructureSource ? AccelerationStructureSource->GetMTLAccelerationStructure() : nil;
}

bool FMetalView::InitializeAccelerationStructureView(FMetalSceneAccelerationStructureRHI* InAccelerationStructure)
{
    if (!InAccelerationStructure)
    {
        METAL_ERROR("Cannot create an acceleration structure view without a scene");
        return false;
    }

    // The structure pins its own storage, a pin through the view would outlive a SetStorage that replaced the entry
    AccelerationStructureSource = InAccelerationStructure;
    ResidencyEntry              = nullptr;
    return true;
}

void FMetalView::DeclareBindlessResidency()
{
    FMetalBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager();
    FMetalResidencySet&              ResidencySet    = GetDevice()->GetResidencySet();

    if (!bOwnsTextureView || bDeclaredResident || !ResidencySet.UsesEncoderFallback() || !BindlessManager || !BindlessManager->IsEnabled())
    {
        return;
    }

    ResidencySet.Add(TextureView, true);
    bDeclaredResident = true;
}

void FMetalView::OnResourceRelocated(EMetalRelocation Relocation)
{
    const bool bWasResident = bDeclaredResident;

    if (SourceBuffer)
    {
        id<MTLBuffer> NewBuffer = [SourceBuffer->GetMTLBuffer() retain];
        [BufferView release];
        BufferView     = NewBuffer;
        BufferOffset   = SourceBuffer->GetMetalBindOffset() + SourceOffset;
        ResidencyEntry = SourceBuffer->GetResourceStorage().GetResidencyEntry();

        if (bOwnsTextureView)
        {
            ReleaseTextureView();
            CreateBufferTexture();
        }
    }
    else if (SourceTexture)
    {
        ReleaseTextureView();
        CreateTextureView(SourceTexture->GetMTLTexture());
        ResidencyEntry = SourceTexture->GetResourceStorage().GetResidencyEntry();
    }

    if (bWasResident)
    {
        DeclareBindlessResidency();
    }

    if (Relocation == EMetalRelocation::Transient)
    {
        FreeBindlessHandle();
    }
    else if (BindlessHandle.IsValid())
    {
        WriteBindlessHandle();

        FMetalResidencyManager& ResidencyManager = GetDevice()->GetResidencyManager();
        ResidencyManager.Unpin(PinnedEntry);
        PinnedEntry = ResidencyEntry;
        ResidencyManager.Pin(PinnedEntry);
    }
}

void FMetalView::OnResourceReleased()
{
    GetDevice()->GetResidencyManager().Unpin(PinnedEntry);
    PinnedEntry                 = nullptr;
    ResidencyEntry              = nullptr;
    SourceBuffer                = nullptr;
    SourceTexture               = nullptr;
    AccelerationStructureSource = nullptr;
}

bool FMetalView::InitializeTextureView(FRHITexture* InTexture, EFormat InFormat, EViewDimension InViewDimension, 
    uint32 InFirstMip, uint32 InNumMips, uint32 InFirstSlice, uint32 InNumSlices)
{
    FMetalTextureRHI* MetalTexture = GetMetalTexture(InTexture);

    if (!MetalTexture)
    {
        METAL_ERROR("Cannot create a texture view without a texture");
        return false;
    }

    TScopedLock Lock(MetalTexture->GetRelocationLock());

    id<MTLTexture> ParentTexture = MetalTexture->GetMTLTexture();

    if (!ParentTexture)
    {
        METAL_ERROR("Cannot create a texture view of an unallocated texture");
        return false;
    }

    const FRHITextureDesc& TextureDesc = MetalTexture->GetDesc();

    const MTLPixelFormat ViewFormat = MetalRHI::ConvertFormat(InFormat);
    const MTLTextureType ViewType   = MetalRHI::GetMTLTextureType(InViewDimension, TextureDesc.IsMultisampled());

    if (ViewFormat == MTLPixelFormatInvalid || ViewType == MTLTextureType(-1))
    {
        METAL_ERROR("Texture view has an unsupported format or dimension");
        return false;
    }

    const uint32 ParentNumMips   = Math::Max(TextureDesc.NumMipLevels, 1u);
    const uint32 ParentNumSlices = RHIDimensionArrayLayers(TextureDesc.Dimension, Math::Max(TextureDesc.NumArraySlices, 1u));
    const uint32 NumMips         = (InNumMips   == 0) ? (ParentNumMips   - InFirstMip)   : InNumMips;
    const uint32 NumSlices       = (InNumSlices == 0) ? (ParentNumSlices - InFirstSlice) : InNumSlices;

    if ((InFirstMip + NumMips) > ParentNumMips || (InFirstSlice + NumSlices) > ParentNumSlices)
    {
        METAL_ERROR("Texture view range lies outside the texture");
        return false;
    }

    const bool bCoversWholeTexture = ViewFormat == ParentTexture.pixelFormat 
        && ViewType == ParentTexture.textureType
        && InFirstMip == 0 && NumMips == ParentNumMips 
        && InFirstSlice == 0 && NumSlices == ParentNumSlices;

    TextureViewFormat    = ViewFormat;
    TextureViewType      = ViewType;
    TextureViewLevels    = NSMakeRange(InFirstMip, NumMips);
    TextureViewSlices    = NSMakeRange(InFirstSlice, NumSlices);
    bTextureViewIsParent = bCoversWholeTexture;

    if (!CreateTextureView(ParentTexture))
    {
        return false;
    }

    SourceTexture  = MetalTexture;
    ResidencyEntry = MetalTexture->GetResourceStorage().GetResidencyEntry();
    SourceTexture->AddRelocationListener(this);
    return true;
}

bool FMetalView::CreateTextureView(id<MTLTexture> ParentTexture)
{
    if (bTextureViewIsParent)
    {
        TextureView = [ParentTexture retain];
        return true;
    }

    TextureView = [ParentTexture newTextureViewWithPixelFormat:TextureViewFormat
                                                   textureType:TextureViewType
                                                        levels:TextureViewLevels
                                                        slices:TextureViewSlices];

    if (!TextureView)
    {
        METAL_ERROR("Failed to create a texture view");
        return false;
    }

    bOwnsTextureView = true;
    return true;
}

bool FMetalView::InitializeBufferView(FRHIBuffer* InBuffer, uint64 InOffset, uint64 InSize)
{
    FMetalBufferRHI* MetalBuffer = GetMetalBuffer(InBuffer);

    if (!MetalBuffer)
    {
        METAL_ERROR("Cannot create a buffer view without a buffer");
        return false;
    }

    TScopedLock Lock(MetalBuffer->GetRelocationLock());

    id<MTLBuffer> ParentBuffer = MetalBuffer->GetMTLBuffer();

    if (!ParentBuffer)
    {
        METAL_ERROR("Cannot create a buffer view of an unallocated buffer");
        return false;
    }

    if ((InOffset + InSize) > MetalBuffer->GetDesc().Size)
    {
        METAL_ERROR("Buffer view range lies outside the buffer");
        return false;
    }

    BufferView     = [ParentBuffer retain];
    BufferOffset   = MetalBuffer->GetMetalBindOffset() + InOffset;
    SourceOffset   = InOffset;
    BufferSize     = InSize;
    SourceBuffer   = MetalBuffer;
    ResidencyEntry = MetalBuffer->GetResourceStorage().GetResidencyEntry();
    SourceBuffer->AddRelocationListener(this);
    return true;
}

bool FMetalView::InitializeBufferTextureView(EFormat InFormat, bool bWritable)
{
    if (!BufferView)
    {
        METAL_ERROR("Cannot create a texel-buffer texture without a buffer view");
        return false;
    }

    BufferTextureFormat    = InFormat;
    bBufferTextureWritable = bWritable;
    return CreateBufferTexture();
}

bool FMetalView::CreateBufferTexture()
{
    const EFormat        InFormat    = BufferTextureFormat;
    const bool           bWritable   = bBufferTextureWritable;
    const MTLPixelFormat PixelFormat = MetalRHI::ConvertFormat(InFormat);

    if (PixelFormat == MTLPixelFormatInvalid)
    {
        METAL_ERROR("Typed buffer view has an unsupported format '%s'", ToString(InFormat));
        return false;
    }

    if (bWritable && !MetalRHI::FormatSupportsShaderWrite(PixelFormat))
    {
        METAL_ERROR("Format '%s' cannot be written from a shader on this device", ToString(InFormat));
        return false;
    }

    const uint32 Stride = GetByteStrideFromFormat(InFormat);

    if (Stride == 0 || (BufferSize % Stride) != 0)
    {
        METAL_ERROR("Typed buffer view size %llu is not a multiple of the %u-byte format stride", BufferSize, Stride);
        return false;
    }

    const NSUInteger NumElements = static_cast<NSUInteger>(BufferSize / Stride);
    MTLTextureUsage  Usage       = MTLTextureUsageShaderRead;

    if (bWritable)
    {
        Usage |= MTLTextureUsageShaderWrite;
    }

    MTLTextureDescriptor* Descriptor = [MTLTextureDescriptor textureBufferDescriptorWithPixelFormat:PixelFormat
                                                                                              width:NumElements
                                                                                    resourceOptions:BufferView.resourceOptions
                                                                                              usage:Usage];
    const NSUInteger BytesPerRow = Math::AlignUp<NSUInteger>(NumElements * Stride, 32);
    TextureView = [BufferView newTextureWithDescriptor:Descriptor offset:BufferOffset bytesPerRow:BytesPerRow];

    if (!TextureView)
    {
        METAL_ERROR("Failed to create a texel-buffer texture");
        return false;
    }

    bOwnsTextureView = true;
    return true;
}

FMetalShaderResourceViewRHI::FMetalShaderResourceViewRHI(FMetalDevice* InDevice, FRHIResource* InResource, const FRHIShaderResourceViewDesc& InRHIDesc)
    : FRHIShaderResourceView(InResource, InRHIDesc)
    , FMetalView(InDevice)
{
}

FMetalShaderResourceViewRHI::~FMetalShaderResourceViewRHI() = default;

bool FMetalShaderResourceViewRHI::Initialize()
{
    if (Desc.IsAccelerationStructureSRV())
    {
        return InitializeAccelerationStructureView(static_cast<FMetalSceneAccelerationStructureRHI*>(GetResource()));
    }

    if (Desc.IsBufferSRV())
    {
        FRHIBuffer* Buffer = static_cast<FRHIBuffer*>(GetResource());

        if (!Buffer)
        {
            return false;
        }

        const uint32 Stride = ResolveBufferViewStride(Buffer->GetDesc(), Desc.Buffer.Type, Desc.Buffer.Format);

        if (!InitializeBufferView(Buffer, uint64(Desc.Buffer.FirstElement) * Stride, uint64(Desc.Buffer.NumElements) * Stride))
        {
            return false;
        }

        if (Desc.Buffer.Type == EBufferViewType::Typed && !InitializeBufferTextureView(Desc.Buffer.Format, false))
        {
            return false;
        }
    }
    else
    {
        const FMetalSubresourceRange Range = ResolveViewRange(Desc);

        if (!InitializeTextureView(static_cast<FRHITexture*>(GetResource()), Desc.GetFormat(), Desc.ViewDimension, Range.FirstMip, Range.NumMips, Range.FirstSlice, Range.NumSlices))
        {
            return false;
        }
    }

    DeclareBindlessResidency();
    return true;
}

FMetalUnorderedAccessViewRHI::FMetalUnorderedAccessViewRHI(FMetalDevice* InDevice, FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InRHIDesc)
    : FRHIUnorderedAccessView(InResource, InRHIDesc)
    , FMetalView(InDevice)
{
}

FMetalUnorderedAccessViewRHI::~FMetalUnorderedAccessViewRHI() = default;

bool FMetalUnorderedAccessViewRHI::Initialize()
{
    if (Desc.IsBufferUAV())
    {
        FRHIBuffer* Buffer = static_cast<FRHIBuffer*>(GetResource());

        if (!Buffer)
        {
            return false;
        }

        const uint32 Stride = ResolveBufferViewStride(Buffer->GetDesc(), Desc.Buffer.Type, Desc.Buffer.Format);

        if (!InitializeBufferView(Buffer, uint64(Desc.Buffer.FirstElement) * Stride, uint64(Desc.Buffer.NumElements) * Stride))
        {
            return false;
        }

        if (Desc.Buffer.Type == EBufferViewType::Typed && !InitializeBufferTextureView(Desc.Buffer.Format, true))
        {
            return false;
        }
    }
    else
    {
        if (!MetalRHI::FormatSupportsShaderWrite(MetalRHI::ConvertFormat(Desc.GetFormat())))
        {
            METAL_ERROR("Format '%s' cannot be written from a shader on this device", ToString(Desc.GetFormat()));
            return false;
        }

        const EViewDimension ViewDimension = IsCubeViewDimension(Desc.ViewDimension)
            ? EViewDimension::Texture2DArray
            : Desc.ViewDimension;

        const FMetalSubresourceRange Range = ResolveViewRange(Desc);

        if (!InitializeTextureView(static_cast<FRHITexture*>(GetResource()), Desc.GetFormat(), ViewDimension, Range.FirstMip, Range.NumMips, Range.FirstSlice, Range.NumSlices))
        {
            return false;
        }
    }

    DeclareBindlessResidency();
    return true;
}

FMetalAttachmentView::FMetalAttachmentView(FMetalDevice* InDevice, FRHITexture* InTexture, const FMetalSubresource& InSubresource)
    : FMetalView(InDevice)
    , Texture(GetMetalTexture(InTexture))
    , Subresource(InSubresource)
{
}

FMetalAttachmentView::~FMetalAttachmentView() = default;

bool FMetalAttachmentView::InitializeAttachment(EFormat InFormat, EViewDimension InViewDimension)
{
    if (!Texture)
    {
        return false;
    }

    if (InFormat == Texture->GetDesc().Format)
    {
        return true;
    }

    if (InViewDimension == EViewDimension::Texture3D)
    {
        return InitializeTextureView(Texture, InFormat, InViewDimension, Subresource.MipLevel, 1, 0, 1);
    }

    return InitializeTextureView(Texture, InFormat, InViewDimension, Subresource.MipLevel, 1, Subresource.ArrayIndex, Subresource.NumSlices);
}

id<MTLTexture> FMetalAttachmentView::GetAttachmentTexture() const
{
    id<MTLTexture> FormatView = GetMTLTexture();
    return FormatView ? FormatView : (Texture ? Texture->GetMTLTexture() : nil);
}

FMetalResidencyEntry* FMetalAttachmentView::GetAttachmentResidencyEntry() const
{
    return Texture ? Texture->GetResidencyEntry() : nullptr;
}

void FMetalAttachmentView::ApplyToAttachment(MTLRenderPassAttachmentDescriptor* Attachment) const
{
    const bool     bFormatView       = GetMTLTexture() != nil;
    id<MTLTexture> AttachmentTexture = GetAttachmentTexture();
    const bool     b3D               = AttachmentTexture.textureType == MTLTextureType3D;

    Attachment.texture    = AttachmentTexture;
    Attachment.level      = bFormatView ? 0 : Subresource.MipLevel;
    Attachment.slice      = (bFormatView || b3D) ? 0 : Subresource.ArrayIndex;
    Attachment.depthPlane = b3D ? Subresource.ArrayIndex : 0;
}

FMetalRenderTargetViewRHI::FMetalRenderTargetViewRHI(FMetalDevice* InDevice, FRHITexture* InTexture, const FRHIRenderTargetViewDesc& InDesc)
    : FRHIRenderTargetView(InTexture, InDesc)
    , FMetalAttachmentView(InDevice, InTexture, ResolveAttachmentSubresource(InDesc))
{
}

FMetalRenderTargetViewRHI::~FMetalRenderTargetViewRHI() = default;

bool FMetalRenderTargetViewRHI::Initialize()
{
    return InitializeAttachment(Desc.GetFormat(), Desc.ViewDimension);
}

FMetalDepthStencilViewRHI::FMetalDepthStencilViewRHI(FMetalDevice* InDevice, FRHITexture* InTexture, const FRHIDepthStencilViewDesc& InDesc)
    : FRHIDepthStencilView(InTexture, InDesc)
    , FMetalAttachmentView(InDevice, InTexture, ResolveAttachmentSubresource(InDesc))
    , Flags(InDesc.Flags)
{
}

FMetalDepthStencilViewRHI::~FMetalDepthStencilViewRHI() = default;

bool FMetalDepthStencilViewRHI::Initialize()
{
    return InitializeAttachment(Desc.GetFormat(), Desc.ViewDimension);
}

void* FMetalShaderResourceViewRHI::GetRHINativeHandle() const
{
    return nullptr;
}

FRHIDescriptorHandle FMetalShaderResourceViewRHI::GetBindlessHandle() const
{
    return EnsureBindlessHandle(EDescriptorType::ShaderResource, false);
}

void* FMetalUnorderedAccessViewRHI::GetRHINativeHandle() const
{
    return nullptr;
}

FRHIDescriptorHandle FMetalUnorderedAccessViewRHI::GetBindlessHandle() const
{
    return EnsureBindlessHandle(EDescriptorType::UnorderedAccess, true);
}

void* FMetalRenderTargetViewRHI::GetRHINativeHandle() const
{
    return nullptr;
}

void* FMetalDepthStencilViewRHI::GetRHINativeHandle() const
{
    return nullptr;
}
