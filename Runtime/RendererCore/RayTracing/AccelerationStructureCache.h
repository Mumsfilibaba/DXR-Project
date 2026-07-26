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
    virtual bool Load(const StringView& Key, TArray<uint8>& OutBytes) = 0;

    /** Persists the raw serialized bytes under Key. */
    virtual void Store(const StringView& Key, const TArray<uint8>& Bytes) = 0;
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

    NODISCARD bool TryLoad(const StringView& Key, TArray<uint8>& OutBytes) const
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

        if (!RHI::IsAccelerationStructureSerializationHeaderValid(Header))
        {
            return false;
        }

        OutBytes = ::Move(Bytes);
        return true;
    }

    NODISCARD static constexpr uint64 GetPayloadOffset()
    {
        return sizeof(FRHIAccelerationStructureSerializationHeader);
    }

    void Store(const StringView& Key, const TArray<uint8>& Bytes)
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
