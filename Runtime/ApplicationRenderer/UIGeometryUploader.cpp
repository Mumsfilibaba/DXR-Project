#include "ApplicationRenderer/UIGeometryUploader.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"

static constexpr int32 GVertexGrowth = 4096;
static constexpr int32 GIndexGrowth  = 8192;

bool FUIGeometryStream::Reserve(int32 Count, int32 InStride, bool bInTransient, TArray<FUIRetiredBuffer>& OutRetired, uint64 FrameCounter)
{
    if (Buffer && Count <= Capacity && Stride == InStride && bIsTransient == bInTransient)
    {
        return true;
    }

    const int32 NewCapacity = Count + (Kind == EUIGeometryStreamKind::Index ? GIndexGrowth : GVertexGrowth);

    const EBufferFlags Flags = bInTransient ? EBufferFlags::Transient : (EBufferFlags::Default | EBufferFlags::CopyDest);

    const FRHIBufferDesc Desc = Kind == EUIGeometryStreamKind::Index
        ? FRHIBufferDesc::CreateIndexBuffer(static_cast<uint32>(InStride), static_cast<uint32>(NewCapacity), Flags)
        : FRHIBufferDesc::CreateVertexBuffer(static_cast<uint32>(InStride), static_cast<uint32>(NewCapacity), Flags);

    FRHIBufferRef NewBuffer = RHI::CreateBuffer(Desc, GetDrawState(), nullptr);
    if (!NewBuffer)
    {
        return false;
    }

    NewBuffer->SetDebugName(DebugName);

    Retire(OutRetired, FrameCounter);

    Buffer       = Move(NewBuffer);
    Stride       = InStride;
    Capacity     = NewCapacity;
    bIsTransient = bInTransient;
    return true;
}

void FUIGeometryStream::Retire(TArray<FUIRetiredBuffer>& OutRetired, uint64 FrameCounter)
{
    if (Buffer)
    {
        OutRetired.Add(FUIRetiredBuffer{ Move(Buffer), FrameCounter });
    }

    Buffer   = nullptr;
    Stride   = 0;
    Capacity = 0;
}

void FUIGeometryUploader::Add(FUIGeometryStream& Stream, const void* Data, int32 Count)
{
    CHECK(Stream.GetBuffer() != nullptr);

    if (Count > 0)
    {
        Pending.Add(FPendingUpload{ &Stream, Data, Count * Stream.GetStride() });
    }
}

void FUIGeometryUploader::Flush(FRHICommandList& InCommandList)
{
    TArray<FRHITransitionBarrierDesc, TInlineArrayAllocator<FRHITransitionBarrierDesc, 4>> ToCopyDest;
    TArray<FRHITransitionBarrierDesc, TInlineArrayAllocator<FRHITransitionBarrierDesc, 4>> ToDrawState;

    for (const FPendingUpload& Upload : Pending)
    {
        if (!Upload.Stream->IsTransient())
        {
            FRHIBuffer* const       Buffer    = Upload.Stream->GetBuffer();
            const ERHIResourceState DrawState = Upload.Stream->GetDrawState();

            ToCopyDest.Add(FRHITransitionBarrierDesc::CreateBuffer(Buffer, DrawState, ERHIResourceState::CopyDest));
            ToDrawState.Add(FRHITransitionBarrierDesc::CreateBuffer(Buffer, ERHIResourceState::CopyDest, DrawState));
        }
    }

    if (!ToCopyDest.IsEmpty())
    {
        InCommandList.TransitionBarrier(ToCopyDest);
    }

    for (const FPendingUpload& Upload : Pending)
    {
        InCommandList.UpdateBuffer(Upload.Stream->GetBuffer(), FBufferRegion(0, Upload.Size), Upload.Data);
    }

    if (!ToDrawState.IsEmpty())
    {
        InCommandList.TransitionBarrier(ToDrawState);
    }

    Pending.Clear();
}
