#pragma once
#include "Core/Core.h"
#include "Core/Containers/SharedRef.h"
#include "Core/RefCounted.h"

#include "RHI/RHITexture.h"

#include "Engine/EngineModule.h"

struct FTextureResource;
class FTextureResource2D;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<FTextureResource>   FTextureResourceRef;
typedef TSharedRef<FTextureResource2D> FTextureResource2DRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FTextureResource 

struct ENGINE_API FTextureResource
    : public FRefCounted
{
    FTextureResource()          = default;
    virtual ~FTextureResource() = default;

    virtual FTextureResource2D* GetTexture2D() { return nullptr; }

    virtual bool CreateRHITexture(bool bGenerateMips) = 0;

    virtual void SetName(const FString& InName) = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FTextureResource2D 

class ENGINE_API FTextureResource2D
    : public FTextureResource
{
public:
    FTextureResource2D();
    
    FTextureResource2D(
        void* InTextureData,
        uint32 InWidth,
        uint32 InHeight,
        uint32 InRowPitch,
        EFormat InFormat);

    ~FTextureResource2D();

    virtual FTextureResource2D* GetTexture2D() { return this; }

    virtual bool CreateRHITexture(bool bGenerateMips) override final;

    virtual void SetName(const FString& InName) override final;

    FRHITexture2DRef GetRHITexture() const
    {
        return TextureRHI;
    }

    EFormat GetFormat() const
    {
        return Format;
    }

    uint16 GetWidth() const
    {
        return Width;
    }

    uint16 GetHeight() const
    {
        return Height;
    }

    uint32 GetRowPitch() const
    {
        return RowPitch;
    }

    void* GetData(int32 MipLevel = 0) const
    {
        if (TextureData.IsValidIndex(MipLevel))
        {
            return TextureData[MipLevel];
        }

        return nullptr;
    }

private:
    void DeleteData();

    FRHITexture2DRef TextureRHI;

    TArray<void*> TextureData;

    EFormat Format;

    uint16  Width;
    uint16  Height;

    uint32  RowPitch;
};