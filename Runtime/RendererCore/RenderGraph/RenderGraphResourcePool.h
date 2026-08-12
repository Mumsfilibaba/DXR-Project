#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "RendererCore/RenderGraph/RenderGraphResources.h"

struct RENDERERCORE_API FRenderGraphResourcePool
{
public:
    static bool Initialize();
    static void Release();

    static FORCEINLINE bool IsInitialized()
    {
        return ResourcePool != nullptr;
    }

    static FORCEINLINE FRenderGraphResourcePool& Get()
    {
        return *ResourcePool;
    }

public:
    FRHIBuffer*  AcquireBuffer(const FRenderGraphBufferDesc& InDesc, const CHAR* Name, ERHIResourceState& OutCurrentState);
    FRHITexture* AcquireTexture(const FRenderGraphTextureDesc& InDesc, const CHAR* Name, ERHIResourceState& OutCurrentState);

    void ReleaseBuffer(FRHIBuffer* Buffer, ERHIResourceState CurrentState);
    void ReleaseTexture(FRHITexture* Texture, ERHIResourceState CurrentState);

    void Tick();
    void Flush();

    NODISCARD int32 GetNumTextures() const
    {
        return Textures.Size();
    }

    NODISCARD int32 GetNumBuffers() const
    {
        return Buffers.Size();
    }

private:
    static constexpr uint64 MaxUnusedFrames = 8;

    struct FPooledTexture
    {
        FRHITextureRef          Texture;
        FRenderGraphTextureDesc Desc          = { };
        ERHIResourceState       CurrentState  = ERHIResourceState::Common;
        uint64                  LastUsedFrame = 0;
        bool                    bIsAcquired   = false;
    };

    struct FPooledBuffer
    {
        FRHIBufferRef          Buffer;
        FRenderGraphBufferDesc Desc          = { };
        ERHIResourceState      CurrentState  = ERHIResourceState::Common;
        uint64                 LastUsedFrame = 0;
        bool                   bIsAcquired   = false;
    };

    FRenderGraphResourcePool();
    ~FRenderGraphResourcePool();

    TArray<FPooledTexture> Textures;
    TArray<FPooledBuffer>  Buffers;
    FCriticalSection       PoolCS;
    uint64                 FrameCounter;

    static FRenderGraphResourcePool* ResourcePool;
};
