#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TBitReference

template<typename StorageType>
class TBitReference
{
public:
    FORCEINLINE TBitReference(StorageType& InStorage, StorageType InMask) noexcept
        : Storage(InStorage)
        , Mask(InMask)
    {
    }

public:

    /**
     * Perform bitwise ANS with BitReference and boolean
     *
     * @param bRhs: Value to AND with
     * @return: Returns a reference to this BitReference
     */
    FORCEINLINE TBitReference& operator&=(const bool bRhs) noexcept
    {
        const StorageType Value = bRhs ? (StorageType(~0) & Mask) : StorageType(0);
        Storage |= (Storage & Value);
        return *this;
    }

    /**
     * Perform bitwise ANS with BitReference and boolean
     *
     * @param bRhs: Value to AND with
     * @return: Returns a reference to this BitReference
     */
    FORCEINLINE TBitReference& operator|=(const bool bRhs) noexcept
    {
        const StorageType Value = bRhs ? (StorageType(~0) & Mask) : StorageType(0);
        Storage |= Value;
        return *this;
    }

    /**
     * Assign a new value to underlying bit
     *
     * @param bRhs: Value to assign
     * @return: Returns a reference to this BitReference
     */
    FORCEINLINE TBitReference& operator=(const bool bRhs) noexcept
    {
        const StorageType Value = bRhs ? (StorageType(~0) & Mask) : StorageType(0);
        Storage = (Storage & ~Mask) | Value;
        return *this;
    }

    /**
     * Assign another BitReference to this instance
     *
     * @param Rhs: BitReference to assign
     * @return: Returns a reference to this BitReference
     */
    FORCEINLINE TBitReference& operator=(const TBitReference& Rhs) noexcept
    {
        Storage = Rhs.Storage;
        Mask    = Rhs.Mask;
        return *this;
    }

    /**
     * Returns the value of the bit as a bool
     *
     * @return: Returns true if the Bit is assigned, otherwise false
     */
    FORCEINLINE operator bool() const noexcept
    {
        return (Storage & Mask);
    }

private:
    StorageType& Storage;
    StorageType  Mask;
};