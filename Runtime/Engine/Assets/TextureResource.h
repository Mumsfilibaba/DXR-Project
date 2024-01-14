#pragma once
#include "Core/Core.h"
#include "Core/Containers/SharedRef.h"
#include "Core/RefCounted.h"
#include "RHI/RHIResources.h"
#include "Engine/EngineModule.h"
#include "RendererCore/TextureResourceData.h"

class FTexture;
class FTexture2D;

typedef TSharedRef<FTexture>   FTextureResourceRef;
typedef TSharedRef<FTexture2D> FTextureResource2DRef;

class ENGINE_API FTexture : public FRefCounted
{
public:
    virtual ~FTexture() = default;

    virtual FTexture2D* GetTexture2D() { return nullptr; }

    virtual bool CreateRHITexture(bool bGenerateMips) = 0;

    virtual void CreateData()  = 0;
    virtual void ReleaseData() = 0;

    virtual EFormat GetFormat() const = 0;

    virtual void SetName(const FString& InName) = 0;

    void SetFilename(const FString& InFilename)
    {
        Filename = InFilename;
    }

    const FString& GetFilename() const
    {
        return Filename;
    }

protected:
    FString Filename;
};


class ENGINE_API FTexture2D : public FTexture
{
public:
    FTexture2D();
    FTexture2D(EFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumMips);
    ~FTexture2D();

    virtual FTexture2D* GetTexture2D() override final { return this; }

    virtual bool CreateRHITexture(bool bGenerateMips) override final;
    
    virtual void CreateData() override final;
    
    virtual void ReleaseData() override final;

    virtual void SetName(const FString& InName) override final;

    virtual EFormat GetFormat() const override final { return Format; }

    FRHITextureRef GetRHITexture() const { return TextureRHI; }

    FTextureResourceData* GetTextureResourceData() const { return TextureData; }

    uint32 GetWidth() const
    {
        return Width;
    }
    
    uint32 GetHeight() const
    {
        return Height;
    }

private:
    FRHITextureRef        TextureRHI;
    FTextureResourceData* TextureData;

    EFormat Format;

    uint32  Width;
    uint32  Height;
    uint32  NumMips;
};
