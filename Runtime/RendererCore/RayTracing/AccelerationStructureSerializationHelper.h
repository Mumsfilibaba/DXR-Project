#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Math/Math.h"
#include "RHI/RHI.h"
#include "RHI/RHIBuffer.h"
#include "RHI/RHIDevice.h"
#include "RHI/RHICommandList.h"
#include "RHI/RayTracing/RHIAccelerationStructure.h"
#include "RendererCore/RayTracing/AccelerationStructureCache.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FAccelerationStructureSerializationHelper
{
    enum class EStage : uint8
    {
        SizeQuery = 0,
        Serialize = 1,
    };

    struct FPendingSerialize
    {
        String                               CacheKey;
        FRHIRayTracingAccelerationStructure* Source              = nullptr;
        FRHIBuffer*                          SizeReadbackBuffer  = nullptr;
        FRHIBuffer*                          SerializeDestBuffer = nullptr;
        FRHIBuffer*                          BytesReadbackBuffer = nullptr;
        uint64                               SerializedSize      = 0;
        uint32                               FramesRemaining     = 0;
        EStage                               Stage               = EStage::SizeQuery;
    };

public:
    explicit FAccelerationStructureSerializationHelper(FAccelerationStructureCache* InCache, uint32 InReadbackLatencyInFrames = 3)
        : Cache(InCache)
        , ReadbackLatencyInFrames(InReadbackLatencyInFrames)
    {
    }

    ~FAccelerationStructureSerializationHelper()
    {
        ReleaseAll();
    }

    void RequestSerialize(FRHICommandList& CommandList, FRHIRayTracingAccelerationStructure* AccelerationStructure, const String& CacheKey)
    {
        if (!AccelerationStructure || !Cache || !Cache->HasBackend() || CacheKey.IsEmpty())
        {
            return;
        }

        FRHIBufferDesc ReadbackDesc;
        ReadbackDesc.Flags  = EBufferFlags::ReadBack;
        ReadbackDesc.Size   = sizeof(uint64) * 2;
        ReadbackDesc.Stride = sizeof(uint64);

        FRHIBuffer* ReadbackBuffer = RHI::CreateBuffer(ReadbackDesc, EResourceAccess::CopyDest, nullptr);
        if (!ReadbackBuffer)
        {
            return;
        }

        ReadbackBuffer->AddRef();

        FRHIRayTracingAccelerationStructure* Sources[] = { AccelerationStructure };
        CommandList.WriteAccelerationStructurePostBuildInfo(ReadbackBuffer, 0, EAccelerationStructurePostBuildInfoType::Serialization, Sources, 1);

        FPendingSerialize Pending;
        Pending.Source             = AccelerationStructure;
        Pending.CacheKey           = CacheKey;
        Pending.SizeReadbackBuffer = ReadbackBuffer;
        Pending.FramesRemaining    = ReadbackLatencyInFrames;
        Pending.Stage              = EStage::SizeQuery;
        PendingSerializes.Emplace(::Move(Pending));
    }

    void Tick(FRHICommandList& CommandList)
    {
        for (int32 Index = PendingSerializes.Size() - 1; Index >= 0; --Index)
        {
            FPendingSerialize& Pending = PendingSerializes[Index];
            if (Pending.FramesRemaining > 0)
            {
                --Pending.FramesRemaining;
                continue;
            }

            if (Pending.Stage == EStage::SizeQuery)
            {
                AdvanceFromSizeQuery(CommandList, Pending, Index);
            }
            else
            {
                AdvanceFromSerialize(Pending, Index);
            }
        }
    }

    NODISCARD bool HasPendingWork() const
    {
        return !PendingSerializes.IsEmpty();
    }

    void ReleaseAll()
    {
        for (FPendingSerialize& Pending : PendingSerializes)
        {
            ReleasePendingBuffers(Pending);
        }

        PendingSerializes.Clear();
    }

private:
    void AdvanceFromSizeQuery(FRHICommandList& CommandList, FPendingSerialize& Pending, int32 Index)
    {
        uint64 SerializedSize = 0;
        if (void* Mapped = Pending.SizeReadbackBuffer->Map(0, sizeof(uint64)))
        {
            SerializedSize = *reinterpret_cast<const uint64*>(Mapped);
            Pending.SizeReadbackBuffer->Unmap(0, sizeof(uint64));
        }

        Pending.SizeReadbackBuffer->Release();
        Pending.SizeReadbackBuffer = nullptr;

        if (SerializedSize == 0)
        {
            PendingSerializes.RemoveAt(Index);
            return;
        }

        FRHIBufferDesc DestDesc;
        DestDesc.Flags  = EBufferFlags::Default | EBufferFlags::UnorderedAccessBuffer | EBufferFlags::CopySource | EBufferFlags::AccelerationStructure;
        DestDesc.Size   = Math::AlignUp<uint64>(SerializedSize, RHI::AccelerationStructureBufferAlignment);
        DestDesc.Stride = 0;

        FRHIBufferDesc BytesReadbackDesc;
        BytesReadbackDesc.Flags  = EBufferFlags::ReadBack;
        BytesReadbackDesc.Size   = SerializedSize;
        BytesReadbackDesc.Stride = 0;

        FRHIBuffer* DestBuffer     = RHI::CreateBuffer(DestDesc, EResourceAccess::UnorderedAccess, nullptr);
        FRHIBuffer* ReadbackBuffer = RHI::CreateBuffer(BytesReadbackDesc, EResourceAccess::CopyDest, nullptr);

        if (!DestBuffer || !ReadbackBuffer)
        {
            if (DestBuffer)
            {
                DestBuffer->Release();
            }

            if (ReadbackBuffer)
            {
                ReadbackBuffer->Release();
            }

            PendingSerializes.RemoveAt(Index);
            return;
        }

        DestBuffer->AddRef();
        ReadbackBuffer->AddRef();

        CommandList.SerializeAccelerationStructure(Pending.Source, DestBuffer, 0);
        CommandList.TransitionBufferState(DestBuffer, EResourceAccess::UnorderedAccess, EResourceAccess::CopySource);

        const FRHIBufferCopyDesc CopyDesc(0, 0, static_cast<uint32>(SerializedSize));
        CommandList.CopyBuffer(ReadbackBuffer, DestBuffer, CopyDesc);

        Pending.SerializeDestBuffer = DestBuffer;
        Pending.BytesReadbackBuffer = ReadbackBuffer;
        Pending.SerializedSize      = SerializedSize;
        Pending.FramesRemaining     = ReadbackLatencyInFrames;
        Pending.Stage               = EStage::Serialize;
    }

    void AdvanceFromSerialize(FPendingSerialize& Pending, int32 Index)
    {
        const uint64 SerializedSize = Pending.SerializedSize;

        if (void* Mapped = Pending.BytesReadbackBuffer->Map(0, SerializedSize))
        {
            const uint8* SerializedBytes = reinterpret_cast<const uint8*>(Mapped);

            FRHIAccelerationStructureSerializationHeader Header;
            Header.MagicAndVersion           = FRHIAccelerationStructureSerializationHeader::MAGIC_AND_VERSION;
            Header.RHIType                   = RHI::Device ? RHI::Device->GetRHIType() : ERHIType::Unknown;
            Header.AccelerationStructureType = Pending.Source->GetAccelerationStructureType();
            Header.SerializedSizeInBytes     = SerializedSize;
            Header.DeserializedSizeInBytes   = 0;

            const uint64 IdentifierBytes = SerializedSize < FRHIAccelerationStructureSerializationHeader::DRIVER_MATCHING_IDENTIFIER_SIZE
                ? SerializedSize
                : FRHIAccelerationStructureSerializationHeader::DRIVER_MATCHING_IDENTIFIER_SIZE;

            Memory::Memcpy(Header.DriverMatchingIdentifier, SerializedBytes, static_cast<SIZE_T>(IdentifierBytes));

            TArray<uint8> Blob;
            Blob.Resize(int32(sizeof(FRHIAccelerationStructureSerializationHeader) + SerializedSize));

            Memory::Memcpy(Blob.Data(), &Header, sizeof(FRHIAccelerationStructureSerializationHeader));
            Memory::Memcpy(Blob.Data() + sizeof(FRHIAccelerationStructureSerializationHeader), SerializedBytes, static_cast<SIZE_T>(SerializedSize));

            Pending.BytesReadbackBuffer->Unmap(0, SerializedSize);

            if (Cache)
            {
                Cache->Store(StringView(Pending.CacheKey), Blob);
            }
        }

        ReleasePendingBuffers(Pending);
        PendingSerializes.RemoveAt(Index);
    }

    static void ReleasePendingBuffers(FPendingSerialize& Pending)
    {
        if (Pending.SizeReadbackBuffer)
        {
            Pending.SizeReadbackBuffer->Release();
            Pending.SizeReadbackBuffer = nullptr;
        }

        if (Pending.SerializeDestBuffer)
        {
            Pending.SerializeDestBuffer->Release();
            Pending.SerializeDestBuffer = nullptr;
        }

        if (Pending.BytesReadbackBuffer)
        {
            Pending.BytesReadbackBuffer->Release();
            Pending.BytesReadbackBuffer = nullptr;
        }
    }

    FAccelerationStructureCache* Cache;
    TArray<FPendingSerialize>    PendingSerializes;
    uint32                       ReadbackLatencyInFrames;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
