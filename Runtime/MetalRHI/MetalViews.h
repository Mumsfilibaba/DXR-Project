#pragma once
#include "RHI/RHIResources.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "MetalRHI/MetalRelocatable.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FMetalTextureRHI;
class FMetalBufferRHI;
class FMetalSceneAccelerationStructureRHI;
struct FMetalResidencyEntry;

class FMetalView : public FMetalDeviceChild
{
public:
    FMetalView(FMetalDevice* InDevice);
    virtual ~FMetalView();

    bool InitializeTextureView(FRHITexture* InTexture, EFormat InFormat, EViewDimension InViewDimension, 
        uint32 InFirstMip, uint32 InNumMips, uint32 InFirstSlice, uint32 InNumSlices);
    bool InitializeBufferView(FRHIBuffer* InBuffer, uint64 InOffset, uint64 InSize);
    bool InitializeBufferTextureView(EFormat InFormat, bool bWritable);
    bool InitializeAccelerationStructureView(FMetalSceneAccelerationStructureRHI* InAccelerationStructure);

    // Both are called under the source's relocation lock
    void OnResourceRelocated(EMetalRelocation Relocation);
    void OnResourceReleased();

    void RefreshBindlessHandle();

    id<MTLAccelerationStructure> GetMTLAccelerationStructure() const;

    FMetalResidencyEntry* GetResidencyEntry() const
    {
        return ResidencyEntry;
    }

    id<MTLTexture> GetMTLTexture() const
    {
        return TextureView;
    }

    id<MTLBuffer> GetMTLBuffer() const
    {
        return BufferView;
    }

    uint64 GetBufferOffset() const
    {
        return BufferOffset;
    }

    uint64 GetBufferSize() const
    {
        return BufferSize;
    }

    FMetalBufferRHI* GetSourceBuffer() const
    {
        return SourceBuffer;
    }

protected:
    FRHIDescriptorHandle EnsureBindlessHandle(EDescriptorType DescriptorType, bool bWritable) const;
    void DeclareBindlessResidency();

private:
    FMetalRelocatable* GetSource() const;

    bool CreateBufferTexture();
    bool CreateTextureView(id<MTLTexture> ParentTexture);
    void ReleaseTextureView();
    void WriteBindlessHandle() const;
    void FreeBindlessHandle();

    FMetalBufferRHI*                     SourceBuffer;
    FMetalTextureRHI*                    SourceTexture;
    FMetalSceneAccelerationStructureRHI* AccelerationStructureSource;
    FMetalResidencyEntry*                ResidencyEntry;
    mutable FMetalResidencyEntry*        PinnedEntry;
    id<MTLTexture>                       TextureView;
    id<MTLBuffer>                        BufferView;
    uint64                               BufferOffset;
    uint64                               SourceOffset;
    uint64                               BufferSize;
    mutable FRHIDescriptorHandle         BindlessHandle;
    EFormat                              BufferTextureFormat;
    MTLPixelFormat                       TextureViewFormat;
    MTLTextureType                       TextureViewType;
    NSRange                              TextureViewLevels;
    NSRange                              TextureViewSlices;
    bool                                 bTextureViewIsParent;
    bool                                 bBufferTextureWritable;
    mutable bool                         bBindlessWritable;
    bool                                 bOwnsTextureView;
    bool                                 bDeclaredResident;
};

class FMetalShaderResourceViewRHI : public FRHIShaderResourceView, public FMetalView
{
public:
    FMetalShaderResourceViewRHI(FMetalDevice* InDevice, FRHIResource* InResource, const FRHIShaderResourceViewDesc& InRHIDesc);
    virtual ~FMetalShaderResourceViewRHI();

    // FRHIShaderResourceView Interface
    virtual void* GetRHINativeHandle() const override final;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;

    bool Initialize();
};

class FMetalUnorderedAccessViewRHI : public FRHIUnorderedAccessView, public FMetalView
{
public:
    FMetalUnorderedAccessViewRHI(FMetalDevice* InDevice, FRHIResource* InResource, const FRHIUnorderedAccessViewDesc& InRHIDesc);
    virtual ~FMetalUnorderedAccessViewRHI();

    // FRHIUnorderedAccessView Interface
    virtual void* GetRHINativeHandle() const override final;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;

    bool Initialize();
};

struct FMetalSubresource
{
    uint16 ArrayIndex = 0;
    uint16 NumSlices  = 1;
    uint8  MipLevel   = 0;
};

class FMetalAttachmentView : public FMetalView
{
public:
    FMetalAttachmentView(FMetalDevice* InDevice, FRHITexture* InTexture, const FMetalSubresource& InSubresource);
    virtual ~FMetalAttachmentView();

    bool InitializeAttachment(EFormat InFormat, EViewDimension InViewDimension);

    void ApplyToAttachment(MTLRenderPassAttachmentDescriptor* Attachment) const;

    id<MTLTexture>        GetAttachmentTexture() const;
    FMetalResidencyEntry* GetAttachmentResidencyEntry() const;

    uint16 GetArrayIndex() const
    {
        return Subresource.ArrayIndex;
    }

    uint16 GetNumSlices() const
    {
        return Subresource.NumSlices;
    }

private:
    FMetalTextureRHI* Texture;
    FMetalSubresource Subresource;
};

class FMetalRenderTargetViewRHI : public FRHIRenderTargetView, public FMetalAttachmentView
{
public:
    FMetalRenderTargetViewRHI(FMetalDevice* InDevice, FRHITexture* InTexture, const FRHIRenderTargetViewDesc& InDesc);
    virtual ~FMetalRenderTargetViewRHI();

    // FRHIRenderTargetView Interface
    virtual void* GetRHINativeHandle() const override final;

    bool Initialize();
};

class FMetalDepthStencilViewRHI : public FRHIDepthStencilView, public FMetalAttachmentView
{
public:
    FMetalDepthStencilViewRHI(FMetalDevice* InDevice, FRHITexture* InTexture, const FRHIDepthStencilViewDesc& InDesc);
    virtual ~FMetalDepthStencilViewRHI();

    // FRHIDepthStencilView Interface
    virtual void* GetRHINativeHandle() const override final;

    bool Initialize();

    EDepthStencilViewFlags GetFlags() const
    {
        return Flags;
    }

private:
    EDepthStencilViewFlags Flags;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
