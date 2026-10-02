#pragma once
#include "Core/Containers/Array.h"
#include "RHI/RHIResources.h"

class FRHICommandList;

struct FUIRetiredBuffer
{
    FRHIBufferRef Buffer;
    uint64        Frame;
};

enum class EUIGeometryStreamKind : uint8
{
    Vertex,
    Index,
};

class FUIGeometryStream
{
public:
    FUIGeometryStream(const CHAR* InDebugName, EUIGeometryStreamKind InKind)
        : Buffer(nullptr)
        , DebugName(InDebugName)
        , Stride(0)
        , Capacity(0)
        , Kind(InKind)
        , bIsTransient(false)
    {
    }

    /**
     * @brief Makes sure the buffer holds at least Count elements of InStride bytes.
     *
     * @param Count        The number of elements about to be uploaded.
     * @param InStride     The size of one element, in bytes.
     * @param bInTransient True for a buffer that takes fresh upload memory on every update instead of being copied into.
     * @param OutRetired   Where a buffer that had to be replaced goes to wait out the frames in flight.
     * @param FrameCounter The frame the replaced buffer is retired on.
     * @return False when a new buffer was needed and could not be created.
     */
    NODISCARD bool Reserve(int32 Count, int32 InStride, bool bInTransient, TArray<FUIRetiredBuffer>& OutRetired, uint64 FrameCounter);

    /** @brief Retires the buffer, so the next upload creates a new one. */
    void Retire(TArray<FUIRetiredBuffer>& OutRetired, uint64 FrameCounter);

    NODISCARD FORCEINLINE FRHIBuffer* GetBuffer() const
    {
        return Buffer.Get();
    }

    NODISCARD FORCEINLINE const FRHIBufferRef& GetBufferRef() const
    {
        return Buffer;
    }

    NODISCARD FORCEINLINE int32 GetStride() const
    {
        return Stride;
    }

    NODISCARD FORCEINLINE bool IsTransient() const
    {
        return bIsTransient;
    }

    /** @return The state the buffer is drawn from, which it returns to after every copy. */
    NODISCARD FORCEINLINE ERHIResourceState GetDrawState() const
    {
        return Kind == EUIGeometryStreamKind::Index ? ERHIResourceState::IndexBuffer : ERHIResourceState::VertexBuffer;
    }

private:
    FRHIBufferRef         Buffer;
    const CHAR*           DebugName;
    int32                 Stride;
    int32                 Capacity;
    EUIGeometryStreamKind Kind;
    bool                  bIsTransient;
};

class FUIGeometryUploader
{
public:
    /**
     * @brief Queues an upload. The data is read when Flush records the copy, so it has to stay alive until then.
     *
     * @param Stream The stream, already reserved for Count elements.
     * @param Data   The elements to upload.
     * @param Count  How many elements to upload.
     */
    void Add(FUIGeometryStream& Stream, const void* Data, int32 Count);

    /** @brief Records every queued upload and forgets them. */
    void Flush(FRHICommandList& InCommandList);

private:
    struct FPendingUpload
    {
        FUIGeometryStream* Stream;
        const void*        Data;
        int32              Size;
    };

    TArray<FPendingUpload, TInlineArrayAllocator<FPendingUpload, 4>> Pending;
};
