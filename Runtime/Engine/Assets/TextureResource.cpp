#include "TextureResource.h"

#include "Engine/Resources/TextureFactory.h"

#include "RHI/RHIInterface.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FTextureResource2D 

FTextureResource2D::FTextureResource2D()
    : FTextureResource()
    , TextureRHI(nullptr)
    , TextureData() 
    , Format(EFormat::Unknown)
    , Width(0)
    , Height(0)
{ }

FTextureResource2D::FTextureResource2D(
    void* InTextureData,
    uint32 InWidth,
    uint32 InHeight,
    uint32 InRowPitch,
    EFormat InFormat)
    : FTextureResource()
    , TextureRHI(nullptr)
    , TextureData()
    , Format(InFormat)
    , Width(static_cast<uint16>(InWidth))
    , Height(static_cast<uint16>(InHeight))
    , RowPitch(InRowPitch)
{
    if (InTextureData)
    {
        TextureData.Emplace(InTextureData);
    }
}

FTextureResource2D::~FTextureResource2D()
{
    DeleteData();
}

bool FTextureResource2D::CreateRHITexture(bool bGenerateMips)
{
    if (!IsCompressed(Format) && !bGenerateMips)
    {
        TextureRHI = FTextureFactory::LoadFromMemory(
            reinterpret_cast<uint8*>(GetData()),
            Width,
            Height,
            bGenerateMips ? TextureFactoryFlag_GenerateMips : TextureFactoryFlag_None,
            Format);
    }
    else
    {
        FRHITextureDataInitializer InitalData(TextureData[0], RowPitch, 0);

        FRHITexture2DInitializer Initializer(
            Format,
            Width,
            Height,
            1,
            1,
            ETextureUsageFlags::AllowSRV,
            EResourceAccess::PixelShaderResource,
            &InitalData);

        TextureRHI = RHICreateTexture2D(Initializer);
    }

    if (!TextureRHI)
    {
        DEBUG_BREAK();
        return false;
    }

    DeleteData();
    return true;
}

void FTextureResource2D::SetName(const FString& InName)
{
    if (TextureRHI)
    {
        TextureRHI->SetName(InName);
    }
}

void FTextureResource2D::DeleteData()
{
    for (void* Data : TextureData)
    {
        SAFE_DELETE(Data);
    }

    TextureData.Clear(true);
}
