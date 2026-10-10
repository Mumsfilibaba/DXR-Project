#include "Core/Misc/OutputDeviceManager.h"
#include "RHI/RHI.h"
#include "RHI/ValidationLayer/RHIValidationInternal.h"
#include "RendererCore/RenderGraph/RenderGraphBuilder.h"
#include "RendererCore/RenderGraph/RenderGraphPass.h"
#include "RendererCore/RenderGraph/RenderGraphViewValidation.h"

static bool ValidateTextureViewDesc(FRenderGraphBuilder& Builder, FRenderGraphTexture* Parent, EFormat ViewFormat, 
    EViewDimension ViewDimension, uint32 BaseLayer, uint32 LayerCount, uint32 FirstMip, uint32 NumMips, const CHAR* ViewName)
{
    if (!Parent)
    {
        LOG_ERROR("Graph '%s' view '%s' has a null parent texture", Builder.GetName(), ViewName ? ViewName : "Unnamed");
        Builder.SetHasErrors(true);
        return false;
    }

    const FRHITextureDesc& TextureDesc = Parent->GetDesc().TextureDesc;
    if (!RHIValidationInternal::ValidateTextureSlicesAndMips("RenderGraphView", TextureDesc, BaseLayer, LayerCount, FirstMip, NumMips, ViewFormat, ViewDimension))
    {
        Builder.SetHasErrors(true);
        return false;
    }

    return true;
}

static bool ValidateBufferViewDesc(FRenderGraphBuilder& Builder, FRenderGraphBuffer* Parent, EBufferViewType ViewType, 
    uint32 FirstElement, uint32 NumElements, EFormat Format, const CHAR* ViewName)
{
    if (!Parent)
    {
        LOG_ERROR("Graph '%s' view '%s' has a null parent buffer", Builder.GetName(), ViewName ? ViewName : "Unnamed");
        Builder.SetHasErrors(true);
        return false;
    }

    if (!RHIValidationInternal::ValidateBufferView("RenderGraphView", Parent->GetDesc().BufferDesc, ViewType, FirstElement, NumElements, Format))
    {
        Builder.SetHasErrors(true);
        return false;
    }

    return true;
}

static bool StatesConflict(ERHIResourceState A, ERHIResourceState B, bool bAWrite, bool bBWrite)
{
    if (bAWrite && bBWrite)
    {
        return A != B;
    }

    if (bAWrite || bBWrite)
    {
        const ERHIResourceState ReadState  = bAWrite ? B : A;
        const ERHIResourceState WriteState = bAWrite ? A : B;

        if (WriteState == ERHIResourceState::UnorderedAccess && ReadState == ERHIResourceState::UnorderedAccess)
        {
            return false;
        }

        return WriteState == ERHIResourceState::UnorderedAccess || ReadState == ERHIResourceState::RenderTarget || ReadState == ERHIResourceState::DepthWrite;
    }

    return false;
}

bool RenderGraphViewValidation::ValidateShaderResourceView(FRenderGraphBuilder& Builder, FRenderGraphShaderResourceView* View)
{
    if (!View)
    {
        return false;
    }

    if (View->GetParentKind() == ERenderGraphParentKind::Texture)
    {
        FRenderGraphTexture* Parent = View->GetParentTexture();
        const FRHIShaderResourceViewDesc& Desc = View->GetDesc();

        if (Parent && !Parent->GetDesc().TextureDesc.IsShaderResourceTexture())
        {
            LOG_ERROR("Graph '%s' SRV '%s' parent '%s' lacks ETextureUsageFlags::ShaderResourceTexture", Builder.GetName(), View->GetName(), Parent->GetName());
            Builder.SetHasErrors(true);
            return false;
        }

        switch (Desc.ViewDimension)
        {
            case EViewDimension::Buffer:
                return ValidateBufferViewDesc(Builder, View->GetParentBuffer(), Desc.Buffer.Type, 
                Desc.Buffer.FirstElement, Desc.Buffer.NumElements, Desc.Buffer.Format, View->GetName());

            case EViewDimension::Texture1D:
                return ValidateTextureViewDesc(Builder, Parent, Desc.Texture1D.Format, Desc.ViewDimension, 0, 1, 
                    Desc.Texture1D.FirstMipLevel, Desc.Texture1D.NumMips, View->GetName());

            case EViewDimension::Texture1DArray:
                return ValidateTextureViewDesc(Builder, Parent, Desc.Texture1DArray.Format, Desc.ViewDimension, 
                    Desc.Texture1DArray.FirstArraySlice, Desc.Texture1DArray.NumSlices, Desc.Texture1DArray.FirstMipLevel, Desc.Texture1DArray.NumMips, View->GetName());

            case EViewDimension::Texture2D:
                return ValidateTextureViewDesc(Builder, Parent, Desc.Texture2D.Format, Desc.ViewDimension, 0, 1, 
                    Desc.Texture2D.FirstMipLevel, Desc.Texture2D.NumMips, View->GetName());

            case EViewDimension::Texture2DArray:
                return ValidateTextureViewDesc(Builder, Parent, Desc.Texture2DArray.Format, Desc.ViewDimension, 
                    Desc.Texture2DArray.FirstArraySlice, Desc.Texture2DArray.NumSlices, Desc.Texture2DArray.FirstMipLevel, Desc.Texture2DArray.NumMips, View->GetName());

            case EViewDimension::TextureCube:
                return ValidateTextureViewDesc(Builder, Parent, Desc.TextureCube.Format, Desc.ViewDimension, 0, 6, 
                    Desc.TextureCube.FirstMipLevel, Desc.TextureCube.NumMips, View->GetName());

            case EViewDimension::TextureCubeArray:
                return ValidateTextureViewDesc(Builder, Parent, Desc.TextureCubeArray.Format, Desc.ViewDimension, 
                    Desc.TextureCubeArray.FirstCube * 6u, Desc.TextureCubeArray.NumCubes * 6u, Desc.TextureCubeArray.FirstMipLevel, Desc.TextureCubeArray.NumMips, View->GetName());

            case EViewDimension::Texture3D:
                return ValidateTextureViewDesc(Builder, Parent, Desc.Texture3D.Format, Desc.ViewDimension, 0, 1, 
                    Desc.Texture3D.FirstMipLevel, Desc.Texture3D.NumMips, View->GetName());

            default:
                LOG_ERROR("Graph '%s' SRV '%s' has unsupported ViewDimension '%s'", Builder.GetName(), View->GetName(), ToString(Desc.ViewDimension));
                Builder.SetHasErrors(true);
                return false;
        }
    }

    const FRHIShaderResourceViewDesc& Desc = View->GetDesc();
    if (Desc.ViewDimension != EViewDimension::Buffer)
    {
        LOG_ERROR("Graph '%s' buffer SRV '%s' must use EViewDimension::Buffer", Builder.GetName(), View->GetName());
        Builder.SetHasErrors(true);
        return false;
    }

    return ValidateBufferViewDesc(Builder, View->GetParentBuffer(), Desc.Buffer.Type, 
        Desc.Buffer.FirstElement, Desc.Buffer.NumElements, Desc.Buffer.Format, View->GetName());
}

bool RenderGraphViewValidation::ValidateUnorderedAccessView(FRenderGraphBuilder& Builder, FRenderGraphUnorderedAccessView* View)
{
    if (!View)
    {
        return false;
    }

    if (View->GetParentKind() == ERenderGraphParentKind::Texture)
    {
        FRenderGraphTexture* Parent = View->GetParentTexture();
        const FRHIUnorderedAccessViewDesc& Desc = View->GetDesc();

        if (Parent && !Parent->GetDesc().TextureDesc.IsUnorderedAccessTexture())
        {
            LOG_ERROR("Graph '%s' UAV '%s' parent '%s' lacks ETextureUsageFlags::UnorderedAccessTexture", Builder.GetName(), View->GetName(), Parent->GetName());
            Builder.SetHasErrors(true);
            return false;
        }

        switch (Desc.ViewDimension)
        {
            case EViewDimension::Buffer:
                return ValidateBufferViewDesc(Builder, View->GetParentBuffer(), Desc.Buffer.Type, 
                Desc.Buffer.FirstElement, Desc.Buffer.NumElements, Desc.Buffer.Format, View->GetName());

            case EViewDimension::Texture1D:
                return ValidateTextureViewDesc(Builder, Parent, Desc.Texture1D.Format, Desc.ViewDimension, 0, 1, 
                    Desc.Texture1D.MipLevel, 1, View->GetName());

            case EViewDimension::Texture1DArray:
                return ValidateTextureViewDesc(Builder, Parent, Desc.Texture1DArray.Format, Desc.ViewDimension, 
                    Desc.Texture1DArray.FirstArraySlice, Desc.Texture1DArray.NumSlices, Desc.Texture1DArray.MipLevel, 1, View->GetName());

            case EViewDimension::Texture2D:
                return ValidateTextureViewDesc(Builder, Parent, Desc.Texture2D.Format, Desc.ViewDimension, 0, 1, 
                    Desc.Texture2D.MipLevel, 1, View->GetName());

            case EViewDimension::Texture2DArray:
                return ValidateTextureViewDesc(Builder, Parent, Desc.Texture2DArray.Format, Desc.ViewDimension, 
                    Desc.Texture2DArray.FirstArraySlice, Desc.Texture2DArray.NumSlices, Desc.Texture2DArray.MipLevel, 1, View->GetName());

            case EViewDimension::TextureCube:
                return ValidateTextureViewDesc(Builder, Parent, Desc.TextureCube.Format, Desc.ViewDimension, 0, 6, 
                    Desc.TextureCube.MipLevel, 1, View->GetName());

            case EViewDimension::TextureCubeArray:
                return ValidateTextureViewDesc(Builder, Parent, Desc.TextureCubeArray.Format, Desc.ViewDimension, 
                    Desc.TextureCubeArray.FirstCube * 6u, Desc.TextureCubeArray.NumCubes * 6u, Desc.TextureCubeArray.MipLevel, 1, View->GetName());

            case EViewDimension::Texture3D:
                return ValidateTextureViewDesc(Builder, Parent, Desc.Texture3D.Format, Desc.ViewDimension, 
                    Desc.Texture3D.FirstWSlice, Desc.Texture3D.WSize, Desc.Texture3D.MipLevel, 1, View->GetName());

            default:
                LOG_ERROR("Graph '%s' UAV '%s' has unsupported ViewDimension '%s'", Builder.GetName(), View->GetName(), ToString(Desc.ViewDimension));
                Builder.SetHasErrors(true);
                return false;
        }
    }

    const FRHIUnorderedAccessViewDesc& Desc = View->GetDesc();
    return ValidateBufferViewDesc(Builder, View->GetParentBuffer(), Desc.Buffer.Type, 
        Desc.Buffer.FirstElement, Desc.Buffer.NumElements, Desc.Buffer.Format, View->GetName());
}

bool RenderGraphViewValidation::ValidateRenderTargetView(FRenderGraphBuilder& Builder, FRenderGraphRenderTargetView* View)
{
    if (!View)
    {
        return false;
    }

    FRenderGraphTexture* Parent = View->GetParent();
    const FRHIRenderTargetViewDesc& Desc = View->GetDesc();

    if (Parent && !Parent->GetDesc().TextureDesc.IsRenderTarget())
    {
        LOG_ERROR("Graph '%s' RTV '%s' parent '%s' lacks ETextureUsageFlags::RenderTarget", Builder.GetName(), View->GetName(), Parent->GetName());
        Builder.SetHasErrors(true);
        return false;
    }

    switch (Desc.ViewDimension)
    {
        case EViewDimension::Texture1D:
            return ValidateTextureViewDesc(Builder, Parent, Desc.Texture1D.Format, Desc.ViewDimension, 0, 1, 
                Desc.Texture1D.MipLevel, 1, View->GetName());

        case EViewDimension::Texture1DArray:
            return ValidateTextureViewDesc(Builder, Parent, Desc.Texture1DArray.Format, Desc.ViewDimension, 
                Desc.Texture1DArray.FirstArraySlice, Desc.Texture1DArray.NumSlices, Desc.Texture1DArray.MipLevel, 1, View->GetName());

        case EViewDimension::Texture2D:
            return ValidateTextureViewDesc(Builder, Parent, Desc.Texture2D.Format, Desc.ViewDimension, 0, 1, 
                Desc.Texture2D.MipLevel, 1, View->GetName());

        case EViewDimension::Texture2DArray:
            return ValidateTextureViewDesc(Builder, Parent, Desc.Texture2DArray.Format, Desc.ViewDimension, 
                Desc.Texture2DArray.FirstArraySlice, Desc.Texture2DArray.NumSlices, Desc.Texture2DArray.MipLevel, 1, View->GetName());

        case EViewDimension::TextureCube:
            return ValidateTextureViewDesc(Builder, Parent, Desc.TextureCube.Format, Desc.ViewDimension, 0, 6, 
                Desc.TextureCube.MipLevel, 1, View->GetName());

        case EViewDimension::TextureCubeArray:
            return ValidateTextureViewDesc(Builder, Parent, Desc.TextureCubeArray.Format, Desc.ViewDimension, 
                Desc.TextureCubeArray.FirstCube * 6u, Desc.TextureCubeArray.NumCubes * 6u, Desc.TextureCubeArray.MipLevel, 1, View->GetName());

        case EViewDimension::Texture3D:
            return ValidateTextureViewDesc(Builder, Parent, Desc.Texture3D.Format, Desc.ViewDimension, 
                Desc.Texture3D.FirstWSlice, Desc.Texture3D.WSize, Desc.Texture3D.MipLevel, 1, View->GetName());

        default:
            LOG_ERROR("Graph '%s' RTV '%s' has unsupported ViewDimension '%s'", Builder.GetName(), View->GetName(), ToString(Desc.ViewDimension));
            Builder.SetHasErrors(true);
            return false;
    }
}

bool RenderGraphViewValidation::ValidateDepthStencilView(FRenderGraphBuilder& Builder, FRenderGraphDepthStencilView* View)
{
    if (!View)
    {
        return false;
    }

    FRenderGraphTexture* Parent = View->GetParent();
    const FRHIDepthStencilViewDesc& Desc = View->GetDesc();

    if (Parent && !Parent->GetDesc().TextureDesc.IsDepthStencil())
    {
        LOG_ERROR("Graph '%s' DSV '%s' parent '%s' lacks ETextureUsageFlags::DepthStencil", Builder.GetName(), View->GetName(), Parent->GetName());
        Builder.SetHasErrors(true);
        return false;
    }

    switch (Desc.ViewDimension)
    {
        case EViewDimension::Texture1D:
            return ValidateTextureViewDesc(Builder, Parent, Desc.Texture1D.Format, Desc.ViewDimension, 0, 1, 
                Desc.Texture1D.MipLevel, 1, View->GetName());

        case EViewDimension::Texture1DArray:
            return ValidateTextureViewDesc(Builder, Parent, Desc.Texture1DArray.Format, Desc.ViewDimension, 
                Desc.Texture1DArray.FirstArraySlice, Desc.Texture1DArray.NumSlices, Desc.Texture1DArray.MipLevel, 1, View->GetName());

        case EViewDimension::Texture2D:
            return ValidateTextureViewDesc(Builder, Parent, Desc.Texture2D.Format, Desc.ViewDimension, 0, 1, 
                Desc.Texture2D.MipLevel, 1, View->GetName());

        case EViewDimension::Texture2DArray:
            return ValidateTextureViewDesc(Builder, Parent, Desc.Texture2DArray.Format, Desc.ViewDimension, 
                Desc.Texture2DArray.FirstArraySlice, Desc.Texture2DArray.NumSlices, Desc.Texture2DArray.MipLevel, 1, View->GetName());

        case EViewDimension::TextureCube:
            return ValidateTextureViewDesc(Builder, Parent, Desc.TextureCube.Format, Desc.ViewDimension, 0, 6, 
                Desc.TextureCube.MipLevel, 1, View->GetName());

        case EViewDimension::TextureCubeArray:
            return ValidateTextureViewDesc(Builder, Parent, Desc.TextureCubeArray.Format, Desc.ViewDimension, 
                Desc.TextureCubeArray.FirstCube * 6u, Desc.TextureCubeArray.NumCubes * 6u, Desc.TextureCubeArray.MipLevel, 1, View->GetName());

        default:
            LOG_ERROR("Graph '%s' DSV '%s' has unsupported ViewDimension '%s'", Builder.GetName(), View->GetName(), ToString(Desc.ViewDimension));
            Builder.SetHasErrors(true);
            return false;
    }
}

bool RenderGraphViewValidation::ValidatePassViewAccesses(FRenderGraphBuilder& Builder, const FRenderGraphPass& Pass)
{
    bool bValid = true;

    for (int32 FirstIndex = 0; FirstIndex < Pass.GetViewAccesses().Size(); ++FirstIndex)
    {
        const FRenderGraphViewAccess& FirstAccess = Pass.GetViewAccesses()[FirstIndex];
        for (int32 SecondIndex = FirstIndex + 1; SecondIndex < Pass.GetViewAccesses().Size(); ++SecondIndex)
        {
            const FRenderGraphViewAccess& SecondAccess = Pass.GetViewAccesses()[SecondIndex];
            if (FirstAccess.bIsTextureParent != SecondAccess.bIsTextureParent)
            {
                continue;
            }

            if (FirstAccess.bIsTextureParent)
            {
                if (FirstAccess.ParentTexture != SecondAccess.ParentTexture)
                {
                    continue;
                }

                if (!RenderGraphViewRanges::Overlap(FirstAccess.SubresourceRange, SecondAccess.SubresourceRange))
                {
                    continue;
                }
            }
            else
            {
                if (FirstAccess.ParentBuffer != SecondAccess.ParentBuffer)
                {
                    continue;
                }

                if (FirstAccess.BufferRange.Offset != SecondAccess.BufferRange.Offset || 
                    FirstAccess.BufferRange.Size != SecondAccess.BufferRange.Size)
                {
                    continue;
                }
            }

            if (StatesConflict(FirstAccess.State, SecondAccess.State, FirstAccess.bIsWrite, SecondAccess.bIsWrite))
            {
                LOG_ERROR("Pass '%s' declares conflicting view accesses on '%s'", Pass.GetName(), 
                    FirstAccess.bIsTextureParent ? FirstAccess.ParentTexture->GetName() : FirstAccess.ParentBuffer->GetName());

                Builder.SetHasErrors(true);
                bValid = false;
            }
        }
    }

    return bValid;
}
