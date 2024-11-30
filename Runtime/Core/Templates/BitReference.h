#pragma once

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
     * @brief Perform bitwise ANS with BitReference and boolean
     * @param bRHS - Value to AND with
     * @return Returns a reference to this BitReference
     */
    FORCEINLINE TBitReference& operator&=(const bool bRHS) noexcept
    {
        const StorageType Value = bRHS ? (StorageType(~0) & Mask) : StorageType(0);
        Storage |= (Storage & Value);
        return *this;
    }

    /**
     * @brief Perform bitwise ANS with BitReference and boolean
     * @param bRHS - Value to AND with
     * @return Returns a reference to this BitReference
     */
    FORCEINLINE TBitReference& operator|=(const bool bRHS) noexcept
    {
        const StorageType Value = bRHS ? (StorageType(~0) & Mask) : StorageType(0);
        Storage |= Value;
        return *this;
    }

    /**
     * @brief Assign a new value to underlying bit
     * @param bRHS - Value to assign
     * @return Returns a reference to this BitReference
     */
    FORCEINLINE TBitReference& operator=(const bool bRHS) noexcept
    {
        const StorageType Value = bRHS ? (StorageType(~0) & Mask) : StorageType(0);
        Storage = (Storage & ~Mask) | Value;
        return *this;
    }

    /**
     * @brief Assign another BitReference to this instance
     * @param Rhs - BitReference to assign
     * @return Returns a reference to this BitReference
     */
    FORCEINLINE TBitReference& operator=(const TBitReference& Rhs) noexcept
    {
        Storage = Rhs.Storage;
        Mask    = Rhs.Mask;
        return *this;
    }

    /**
     * @brief Returns the value of the bit as a bool
     * @return Returns true if the Bit is assigned, otherwise false
     */
    FORCEINLINE operator bool() const noexcept
    {
        return Storage & Mask;
    }

private:
    StorageType& Storage;
    StorageType  Mask;
};