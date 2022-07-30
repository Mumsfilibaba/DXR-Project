#pragma once
#include "ClassUtilities.h"

#include "Core/CoreTypes.h"
#include "Core/CoreDefines.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TAlignmentOf

template<typename T>
struct TAlignmentOf
{
    enum
    {
        Value = alignof(T)
    };
};

template<typename T>
inline constexpr int32 AlignmentOf = TAlignmentOf<T>::Value;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TAlignedStorage

template<int32 kNumBytes, int32 kAlignment> 
class TAlignedStorage
{
public:
    TAlignedStorage() = default;
    TAlignedStorage(const TAlignedStorage&) = default;
    TAlignedStorage(TAlignedStorage&&) = default;
    ~TAlignedStorage() = default;

    TAlignedStorage& operator=(const TAlignedStorage&) = default;
    TAlignedStorage& operator=(TAlignedStorage&&) = default;

    FORCEINLINE void* GetStorage() noexcept
    {
        return Storage;
    }

    FORCEINLINE const void* GetStorage() const noexcept
    {
        return Storage;
    }

    template<typename T>
    FORCEINLINE T* CastStorage() noexcept
    {
        return reinterpret_cast<T*>(GetStorage());
    }

    template<typename T>
    FORCEINLINE const T* CastStorage() const noexcept
    {
        return reinterpret_cast<const T*>(GetStorage());
    }

private:
    ALIGN_AS(kAlignment) uint8 Storage[kNumBytes];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TTypedStorage

template<typename T>
class TTypedStorage
{
public:
    using StorageType = TAlignedStorage<sizeof(T), AlignmentOf<T>>;

    TTypedStorage() = default;
    TTypedStorage(const TTypedStorage&) = default;
    TTypedStorage(TTypedStorage&&) = default;
    ~TTypedStorage() = default;

    TTypedStorage& operator=(const TTypedStorage&) = default;
    TTypedStorage& operator=(TTypedStorage&&) = default;

    FORCEINLINE T* GetStorage() noexcept
    {
        return Storage.template CastStorage<T>();
    }

    FORCEINLINE const T* GetStorage() const noexcept
    {
        return Storage.template CastStorage<T>();
    }

private:
    StorageType Storage;
};
