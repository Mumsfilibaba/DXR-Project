#include "RHI/RHI.h"
#include "RendererCore/TextureFactory.h"
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

    // Calculate how many miplevels we need in the RHI texture
    uint32 NumMipsRHI = NumMips;
    if (bGenerateMips)
    {
        NumMipsRHI = FMath::Max<uint32>(static_cast<uint32>(FMath::Log2(static_cast<float>(FMath::Max(Width, Height)))), 1u);
    }

    FRHITextureInfo TextureInfo = FRHITextureInfo::CreateTexture2D(Format, Width, Height, NumMipsRHI, 1, ETextureUsageFlags::ShaderResource);
    TextureRHI = RHICreateTexture(TextureInfo, EResourceAccess::PixelShaderResource, TextureData);
    if (!TextureRHI)
    {
        DEBUG_BREAK();
        return false;
    }

    if (bGenerateMips)
    {
        CHECK(!IsBlockCompressed(Format));

        FRHICommandList CommandList;
        CommandList.TransitionTexture(TextureRHI.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::CopyDest));
        CommandList.GenerateMips(TextureRHI.Get());
        CommandList.TransitionTexture(TextureRHI.Get(), FRHITextureTransition::Make(EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource));
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

void FTexture2D::SetDebugName(const FString& InName)
{
    if (TextureRHI)
    {
        TextureRHI->SetDebugName(InName);
    }
}
