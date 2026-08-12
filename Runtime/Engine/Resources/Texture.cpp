#include "RHI/RHI.h"
#include "RendererCore/TextureFactory.h"
#include "RendererCore/TextureHelpers.h"
#include "Engine/Resources/Texture.h"

FTexture2D::FTexture2D()
    : FTexture()
    , TextureRHI(nullptr)
    , TextureData(nullptr)
    , Format(EFormat::Unknown)
    , Width(0)
    , Height(0)
    , NumMips(0)
{
}

FTexture2D::FTexture2D(EFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumMips)
    : FTexture()
    , TextureRHI(nullptr)
    , TextureData(nullptr)
    , Format(InFormat)
    , Width(InWidth)
    , Height(InHeight)
    , NumMips(InNumMips)
{
}

FTexture2D::~FTexture2D()
{
    ReleaseData();
}

bool FTexture2D::CreateRHITexture(bool bGenerateMips)
{
    if (IsBlockCompressed(Format) && (!IsBlockCompressedAligned(Width) || !IsBlockCompressedAligned(Height)))
    {
        DEBUG_BREAK();
        return false;
    }

    uint32 NumMipsRHI = NumMips;
    if (bGenerateMips)
    {
        NumMipsRHI = TextureHelpers::TextureSizeToMiplevels(Math::Max(Width, Height));
    }

    ETextureUsageFlags TextureUsage = ETextureUsageFlags::ShaderResourceTexture;
    if (bGenerateMips)
    {
        TextureUsage |= ETextureUsageFlags::CopyDest;
    }

    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(Format, Width, Height, NumMipsRHI, 1, TextureUsage);

    if (!bGenerateMips)
    {
        TextureDesc.TrackingMode = ERHIResourceStateTrackingMode::Static;
    }

    TextureRHI = RHI::CreateTexture(TextureDesc, ERHIResourceState::ShaderResource, TextureData);
    if (!TextureRHI)
    {
        DEBUG_BREAK();
        return false;
    }

    if (bGenerateMips)
    {
        CHECK(!IsBlockCompressed(Format));

        FRHICommandList CommandList;
        if (FTextureFactory::Get().GenerateMiplevels(CommandList, TextureRHI.Get(), TextureData))
        {
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTextureModeChange(
                TextureRHI.Get(), ERHIResourceState::ShaderResource, ERHIResourceState::ShaderResource, ERHIResourceStateTrackingMode::Static));
        }

        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    }

    return true;
}

void FTexture2D::CreateData()
{
    if (!TextureData)
    {
        TextureData = new FTextureResourceData();
    }
}

void FTexture2D::ReleaseData()
{
    SAFE_DELETE(TextureData);
}

void FTexture2D::SetDebugName(const String& InName)
{
    if (TextureRHI)
    {
        TextureRHI->SetDebugName(InName);
    }
}
