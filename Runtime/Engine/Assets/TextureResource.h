#pragma once
#include "Core/Core.h"
#include "Core/Containers/SharedRef.h"
#include "Core/RefCounted.h"

#include "RHI/RHITexture.h"

#include "Engine/EngineModule.h"

struct FTexture;
class FTexture2D;

// This is based on the max texture-size 32678
#define MAX_TEXTURE_MIPS (15)

typedef TSharedRef<FTexture>   FTextureResourceRef;
typedef TSharedRef<FTexture2D> FTextureResource2DRef;


class ENGINE_API FTextureResourceData
    : public IRHITextureData
{
public:
    FTextureResourceData();
    ~FTextureResourceData();

    void InitMipData(const void* InTextureData, int64 InTextureDataRowPitch, int64 InTextureDataSlicePitch, uint32 MipLevel = 0);

    virtual void* GetMipData(uint32 MipLevel = 0) const override final
    {
        CHECK(MipLevel < MAX_TEXTURE_MIPS);
        return TextureData[MipLevel];
    }

    virtual int64 GetMipRowPitch(uint32 MipLevel = 0) const override final
    {
        CHECK(MipLevel < MAX_TEXTURE_MIPS);
        return TextureDataRowPitch[MipLevel];
    }

    virtual int64 GetMipSlicePitch(uint32 MipLevel = 0) const override final
    {
        CHECK(MipLevel < MAX_TEXTURE_MIPS);
        return TextureDataSlicePitch[MipLevel];
    }

private:
    void MemzeroData();

    void* TextureData[MAX_TEXTURE_MIPS];
    int64 TextureDataRowPitch[MAX_TEXTURE_MIPS];
    int64 TextureDataSlicePitch[MAX_TEXTURE_MIPS];
};


struct ENGINE_API FTexture
    : public FRefCounted
{
    FTexture()          = default;
    virtual ~FTexture() = default;

    virtual FTexture2D* GetTexture2D() { return nullptr; }

    virtual bool CreateRHITexture(bool bGenerateMips) = 0;

    virtual void CreateData()  = 0;
    virtual void ReleaseData() = 0;

    virtual void SetName(const FString& InName) = 0;
};


class ENGINE_API FTexture2D
    : public FTexture
{
public:
    FTexture2D();
    FTexture2D(EFormat InFormat, uint32 InWidth, uint32 InHeight, uint32 InNumMips);
    ~FTexture2D();

    virtual FTexture2D* GetTexture2D() { return this; }

    virtual bool CreateRHITexture(bool bGenerateMips) override final;
    
    virtual void CreateData()  override final;
    virtual void ReleaseData() override final;

    virtual void SetName(const FString& InName) override final;

    FRHITextureRef GetRHITexture() const { return TextureRHI; }

    FTextureResourceData* GetTextureResourceData() const { return TextureData; }

    EFormat GetFormat() const { return Format; }

    uint32 GetWidth() const { return Width; }
    uint32 GetHeight() const { return Height; }

private:

    FRHITextureRef      TextureRHI;
    FTextureResourceData* TextureData;

    EFormat Format;

    uint32  Width;
    uint32  Height;
    uint32  NumMips;
};