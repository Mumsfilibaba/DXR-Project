#include "TextureResource.h"
#include "Engine/Resources/TextureFactory.h"
#include "RHI/RHIInterface.h"

FTextureResourceData::FTextureResourceData()
    : TextureData()
    , TextureDataRowPitch()
    , TextureDataSlicePitch()
{
    MemzeroData();
}

FTextureResourceData::~FTextureResourceData()
{
    for (void* Data : TextureData)
    {
        if (Data)
        {
            FMemory::Free(Data);
        }
        else
        {
            break;
        }
    }

    MemzeroData();
}

void FTextureResourceData::InitMipData(const void* InTextureData, int64 InTextureDataRowPitch, int64 InTextureDataSlicePitch, uint32 MipLevel)
{
    CHECK(MipLevel < MAX_TEXTURE_MIPS);

    TextureData[MipLevel] = FMemory::Malloc(InTextureDataSlicePitch);
    FMemory::Memcpy(TextureData[MipLevel], InTextureData, InTextureDataSlicePitch);

    TextureDataRowPitch[MipLevel]   = InTextureDataRowPitch;
    TextureDataSlicePitch[MipLevel] = InTextureDataSlicePitch;
}

void FTextureResourceData::MemzeroData()
{
    FMemory::Memzero(TextureData, sizeof(TextureData));
    FMemory::Memzero(TextureDataRowPitch, sizeof(TextureDataRowPitch));
    FMemory::Memzero(TextureDataSlicePitch, sizeof(TextureDataSlicePitch));
}


FTexture2D::FTexture2D()
    : FTexture()
    , TextureRHI(nullptr)
    , TextureData(nullptr)
    , Format(EFormat::Unknown)
    , Width(0)
    , Height(0)
    , NumMips(0)
{ }

FTexture2D::FTexture2D(EFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumMips)
    : FTexture()
    , TextureRHI(nullptr)
    , TextureData(nullptr)
    , Format(InFormat)
    , Width(InWidth)
    , Height(InHeight)
    , NumMips(InNumMips)
{ }

FTexture2D::~FTexture2D()
{
    ReleaseData();
}

bool FTexture2D::CreateRHITexture(bool bGenerateMips)
{
    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(
        Format,
        Width,
        Height,
        NumMips,
        1,
        ETextureUsageFlags::ShaderResource);

    TextureRHI = RHICreateTexture(TextureDesc, EResourceAccess::PixelShaderResource, TextureData);
    if (!TextureRHI)
    {
        DEBUG_BREAK();
        return false;
    }

    if (bGenerateMips)
    {
        CHECK(!IsCompressed(Format));

        FRHICommandList CommandList;
        CommandList.TransitionTexture(TextureRHI.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::CopyDest);
        CommandList.GenerateMips(TextureRHI.Get());
        CommandList.TransitionTexture(TextureRHI.Get(), EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource);

        GRHICommandExecutor.ExecuteCommandList(CommandList);
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

void FTexture2D::SetName(const FString& InName)
{
    if (TextureRHI)
    {
        TextureRHI->SetName(InName);
    }
}
