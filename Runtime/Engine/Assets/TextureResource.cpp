#include "TextureResource.h"

#include "Engine/Resources/TextureFactory.h"

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
    EFormat InFormat)
    : FTextureResource()
    , TextureRHI(nullptr)
    , TextureData()
    , Format(InFormat)
    , Width(static_cast<uint16>(InWidth))
    , Height(static_cast<uint16>(InHeight))
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
    TextureRHI = FTextureFactory::LoadFromMemory(
        reinterpret_cast<uint8*>(GetData()),
        Width,
        Height,
        bGenerateMips ? TextureFactoryFlag_GenerateMips : TextureFactoryFlag_None,
        Format);

    if (!TextureRHI)
    {
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
