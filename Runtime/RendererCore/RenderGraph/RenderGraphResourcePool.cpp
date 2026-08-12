#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Threading/ScopedLock.h"
#include "RHI/RHI.h"
#include "RendererCore/RenderGraph/RenderGraphResourcePool.h"

FRenderGraphResourcePool* FRenderGraphResourcePool::ResourcePool = nullptr;

FRenderGraphResourcePool::FRenderGraphResourcePool()
    : Textures()
    , Buffers()
    , PoolCS()
    , FrameCounter(0)
{
}

FRenderGraphResourcePool::~FRenderGraphResourcePool()
{
    Textures.Clear();
    Buffers.Clear();
}

bool FRenderGraphResourcePool::Initialize()
{
    ResourcePool = new FRenderGraphResourcePool();
    return true;
}

void FRenderGraphResourcePool::Release()
{
    if (ResourcePool)
    {
        delete ResourcePool;
        ResourcePool = nullptr;
    }
}

FRHITexture* FRenderGraphResourcePool::AcquireTexture(const FRenderGraphTextureDesc& InDesc, const CHAR* Name, ERHIResourceState& OutCurrentState)
{
    FRenderGraphTextureDesc Desc  = InDesc;
    Desc.TextureDesc.TrackingMode = ERHIResourceStateTrackingMode::Manual;

    TScopedLock Lock(PoolCS);

    for (FPooledTexture& Pooled : Textures)
    {
        if (!Pooled.bIsAcquired && Pooled.Desc == Desc)
        {
            Pooled.bIsAcquired   = true;
            Pooled.LastUsedFrame = FrameCounter;
            OutCurrentState      = Pooled.CurrentState;
            return Pooled.Texture.Get();
        }
    }

    OutCurrentState = ERHIResourceState::Common;

    FRHITextureRef NewTexture = RHI::CreateTexture(Desc.TextureDesc, ERHIResourceState::Common);
    if (!NewTexture)
    {
        LOG_ERROR("Failed to create render-graph texture '%s'", Name ? Name : "Unnamed");
        return nullptr;
    }

    NewTexture->SetDebugName(Name ? Name : "RenderGraphTexture");

    FPooledTexture& Pooled = Textures.Emplace();
    Pooled.Texture         = NewTexture;
    Pooled.Desc            = Desc;
    Pooled.CurrentState    = ERHIResourceState::Common;
    Pooled.LastUsedFrame   = FrameCounter;
    Pooled.bIsAcquired     = true;
    return Pooled.Texture.Get();
}

FRHIBuffer* FRenderGraphResourcePool::AcquireBuffer(const FRenderGraphBufferDesc& InDesc, const CHAR* Name, ERHIResourceState& OutCurrentState)
{
    FRenderGraphBufferDesc Desc  = InDesc;
    Desc.BufferDesc.TrackingMode = ERHIResourceStateTrackingMode::Manual;

    TScopedLock Lock(PoolCS);

    for (FPooledBuffer& Pooled : Buffers)
    {
        if (!Pooled.bIsAcquired && Pooled.Desc == Desc)
        {
            Pooled.bIsAcquired   = true;
            Pooled.LastUsedFrame = FrameCounter;
            OutCurrentState      = Pooled.CurrentState;
            return Pooled.Buffer.Get();
        }
    }

    OutCurrentState = ERHIResourceState::Common;

    FRHIBufferRef NewBuffer = RHI::CreateBuffer(Desc.BufferDesc, ERHIResourceState::Common);
    if (!NewBuffer)
    {
        LOG_ERROR("Failed to create render-graph buffer '%s'", Name ? Name : "Unnamed");
        return nullptr;
    }

    NewBuffer->SetDebugName(Name ? Name : "RenderGraphBuffer");

    FPooledBuffer& Pooled = Buffers.Emplace();
    Pooled.Buffer         = NewBuffer;
    Pooled.Desc           = Desc;
    Pooled.CurrentState   = ERHIResourceState::Common;
    Pooled.LastUsedFrame  = FrameCounter;
    Pooled.bIsAcquired    = true;
    return Pooled.Buffer.Get();
}

void FRenderGraphResourcePool::ReleaseTexture(FRHITexture* Texture, ERHIResourceState CurrentState)
{
    if (!Texture)
    {
        return;
    }

    TScopedLock Lock(PoolCS);

    for (FPooledTexture& Pooled : Textures)
    {
        if (Pooled.Texture.Get() == Texture)
        {
            Pooled.bIsAcquired   = false;
            Pooled.CurrentState  = CurrentState;
            Pooled.LastUsedFrame = FrameCounter;
            return;
        }
    }
}

void FRenderGraphResourcePool::ReleaseBuffer(FRHIBuffer* Buffer, ERHIResourceState CurrentState)
{
    if (!Buffer)
    {
        return;
    }

    TScopedLock Lock(PoolCS);

    for (FPooledBuffer& Pooled : Buffers)
    {
        if (Pooled.Buffer.Get() == Buffer)
        {
            Pooled.bIsAcquired   = false;
            Pooled.CurrentState  = CurrentState;
            Pooled.LastUsedFrame = FrameCounter;
            return;
        }
    }
}

void FRenderGraphResourcePool::Tick()
{
    TScopedLock Lock(PoolCS);

    ++FrameCounter;

    for (int32 Index = Textures.Size() - 1; Index >= 0; --Index)
    {
        const FPooledTexture& Pooled = Textures[Index];
        if (!Pooled.bIsAcquired && (FrameCounter - Pooled.LastUsedFrame) > MaxUnusedFrames)
        {
            Textures.RemoveAt(Index);
        }
    }

    for (int32 Index = Buffers.Size() - 1; Index >= 0; --Index)
    {
        const FPooledBuffer& Pooled = Buffers[Index];
        if (!Pooled.bIsAcquired && (FrameCounter - Pooled.LastUsedFrame) > MaxUnusedFrames)
        {
            Buffers.RemoveAt(Index);
        }
    }
}

void FRenderGraphResourcePool::Flush()
{
    TScopedLock Lock(PoolCS);

    for (int32 Index = Textures.Size() - 1; Index >= 0; --Index)
    {
        if (!Textures[Index].bIsAcquired)
        {
            Textures.RemoveAt(Index);
        }
    }

    for (int32 Index = Buffers.Size() - 1; Index >= 0; --Index)
    {
        if (!Buffers[Index].bIsAcquired)
        {
            Buffers.RemoveAt(Index);
        }
    }
}
