#pragma once
#include "Core/Containers/Array.h"
#include "RHI/RHI.h"
#include "RHI/RHIBuffer.h"
#include "RHI/RHICommandList.h"
#include "RHI/RayTracing/RHIAccelerationStructure.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FAccelerationStructureCompactionHelper
{
    struct FPendingCompaction
    {
        FRHIRayTracingAccelerationStructure* Source          = nullptr;
        FRHIBufferRef                        SizeReadbackBuffer;
        uint32                               FramesRemaining = 0;
    };

public:
    explicit FAccelerationStructureCompactionHelper(uint32 InReadbackLatencyInFrames = 3)
        : ReadbackLatencyInFrames(InReadbackLatencyInFrames)
    {
    }

    ~FAccelerationStructureCompactionHelper()
    {
        ReleaseAll();
    }

    void RequestCompaction(FRHICommandList& CommandList, FRHIRayTracingAccelerationStructure* AccelerationStructure)
    {
        if (!AccelerationStructure)
        {
            return;
        }

        if (!IsEnumFlagSet(AccelerationStructure->GetFlags(), EAccelerationStructureBuildFlags::AllowCompaction))
        {
            return;
        }

        FRHIBufferDesc ReadbackDesc;
        ReadbackDesc.Flags  = EBufferFlags::ReadBack;
        ReadbackDesc.Size   = sizeof(uint64);
        ReadbackDesc.Stride = sizeof(uint64);

        FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(ReadbackDesc, EResourceAccess::CopyDest, nullptr);
        if (!ReadbackBuffer)
        {
            return;
        }

        FRHIRayTracingAccelerationStructure* Sources[] = { AccelerationStructure };
        CommandList.WriteAccelerationStructurePostBuildInfo(ReadbackBuffer.Get(), 0, EAccelerationStructurePostBuildInfoType::CompactedSize, Sources, 1);

        FPendingCompaction Pending;
        Pending.Source             = AccelerationStructure;
        Pending.SizeReadbackBuffer = ::Move(ReadbackBuffer);
        Pending.FramesRemaining    = ReadbackLatencyInFrames;
        PendingCompactions.Emplace(::Move(Pending));
    }

    void Tick(FRHICommandList& CommandList)
    {
        for (int32 Index = PendingCompactions.Size() - 1; Index >= 0; --Index)
        {
            FPendingCompaction& Pending = PendingCompactions[Index];
            if (Pending.FramesRemaining > 0)
            {
                --Pending.FramesRemaining;
                continue;
            }

            uint64 CompactedSize = 0;
            if (void* Mapped = Pending.SizeReadbackBuffer->Map(0, sizeof(uint64)))
            {
                CompactedSize = *reinterpret_cast<const uint64*>(Mapped);
                Pending.SizeReadbackBuffer->Unmap(0, sizeof(uint64));
            }

            if (CompactedSize > 0)
            {
                CommandList.CompactAccelerationStructure(Pending.Source, CompactedSize);
            }

            PendingCompactions.RemoveAt(Index);
        }
    }

    NODISCARD bool HasPendingWork() const
    {
        return !PendingCompactions.IsEmpty();
    }

    void ReleaseAll()
    {
        PendingCompactions.Clear();
    }

private:
    TArray<FPendingCompaction> PendingCompactions;
    uint32                     ReadbackLatencyInFrames;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
