#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "RHI/RHI.h"
#include "RHI/RayTracing/RHIRayTracingTypes.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct IAccelerationStructureCache
{
    virtual ~IAccelerationStructureCache() = default;

    /** @return true and fills OutBytes if Key is present; false on miss. */
    virtual bool Load(const FStringView& Key, TArray<uint8>& OutBytes) = 0;

    /** Persists the raw serialized bytes under Key. */
    virtual void Store(const FStringView& Key, const TArray<uint8>& Bytes) = 0;
};

class FAccelerationStructureCache
{
public:
    FAccelerationStructureCache() = default;

    explicit FAccelerationStructureCache(IAccelerationStructureCache* InBackend)
        : Backend(InBackend)
    {
    }

    void SetBackend(IAccelerationStructureCache* InBackend)
    {
        Backend = InBackend;
    }

    NODISCARD IAccelerationStructureCache* GetBackend() const
    {
        return Backend;
    }

    /**
     * @brief Load the entry under Key: the header and the serialized structure in OutBytes, and what was stored after the structure in OutAttachment
     * @return Returns false on a miss, or when the entry was written by another build, RHI or driver
     */
    NODISCARD bool TryLoad(const FStringView& Key, TArray<uint8>& OutBytes, TArray<uint8>& OutAttachment) const
    {
        if (!Backend)
        {
            return false;
        }

        TArray<uint8> Bytes;
        if (!Backend->Load(Key, Bytes))
        {
            return false;
        }

        if (Bytes.Size() < int32(sizeof(FRHIAccelerationStructureSerializationHeader)))
        {
            return false;
        }

        FRHIAccelerationStructureSerializationHeader Header;
        Memory::Memcpy(&Header, Bytes.Data(), sizeof(FRHIAccelerationStructureSerializationHeader));

        if (!Header.HasValidMagic())
        {
            return false;
        }

        if (Header.SerializedSizeInBytes > uint64(Bytes.Size()) - GetPayloadOffset())
        {
            return false;
        }

        if (!RHI::IsAccelerationStructureSerializationHeaderValid(Header))
        {
            return false;
        }

        const int32 AttachmentOffset = static_cast<int32>(GetPayloadOffset() + Header.SerializedSizeInBytes);
        OutAttachment.Clear();
        OutAttachment.Append(Bytes.Data() + AttachmentOffset, Bytes.Size() - AttachmentOffset);

        Bytes.Resize(AttachmentOffset);
        OutBytes = ::Move(Bytes);
        return true;
    }

    NODISCARD static constexpr uint64 GetPayloadOffset()
    {
        return sizeof(FRHIAccelerationStructureSerializationHeader);
    }

    void Store(const FStringView& Key, const TArray<uint8>& Bytes)
    {
        if (Backend)
        {
            Backend->Store(Key, Bytes);
        }
    }

    NODISCARD bool HasBackend() const
    {
        return Backend != nullptr;
    }

private:
    IAccelerationStructureCache* Backend = nullptr;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
