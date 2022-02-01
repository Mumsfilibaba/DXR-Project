#pragma once
#include "DelegateInstance.h"

#include "Core/Containers/Allocators.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// DelegateBase - Base class containing variables that does not require templates

class CDelegateBase
{
    enum
    {
        // TODO: Look into padding so we can use larger structs?
        InlineBytes = 32
    };

public:

    /**
     * Copy constructor 
     * 
     * @param Other: Delegate to copy from
     */
    FORCEINLINE CDelegateBase(const CDelegateBase& Other)
        : Storage()
        , Size()
    {
        CopyFrom(Other);
    }

    /**
     * Move constructor
     *
     * @param Other: Delegate to move from
     */
    FORCEINLINE CDelegateBase(CDelegateBase&& Other) noexcept
        : Storage()
        , Size(Other.Size)
    {
        Storage.MoveFrom(Move(Other.Storage));
        Other.Size = 0;
    }

    /**
     * Destructor 
     */
    FORCEINLINE ~CDelegateBase()
    {
        Unbind();
    }

    /**
     * Unbinds any bound delegate 
     */
    FORCEINLINE void Unbind()
    {
        Release();
    }

    /**
     * Swaps two delegates 
     * 
     * @param Other: Delegate to swap with
     */
    FORCEINLINE void Swap(CDelegateBase& Other)
    {
        AllocatorType TempStorage;
        TempStorage.MoveFrom(Move(Storage));
        Storage.MoveFrom(Move(Other.Storage));
        Other.Storage.MoveFrom(Move(TempStorage));

        ::Swap<int32>(Size, Other.Size);
    }

    /**
     * Checks weather or not there exist any delegate bound 
     * 
     * @return: Returns true if there is a delegate bound
     */
    FORCEINLINE bool IsBound() const
    {
        return (Size > 0);
    }

    /**
     * Check if an object is bound to this delegate
     * 
     * @param Object: Pointer to object to check for
     * @return: Returns true if this Object is bound to the delegate
     */
    FORCEINLINE bool IsObjectBound(const void* Object) const
    {
        if (Object != nullptr && IsBound())
        {
            return GetDelegate()->IsObjectBound(Object);
        }
        else
        {
            return nullptr;
        }
    }

    /**
     * Check if object is bound to this delegate
     *
     * @param Object: Pointer to object to check for
     * @return: Returns true if the Object was unbound from the delegate
     */
    FORCEINLINE bool UnbindIfBound(const void* Object)
    {
        if (IsObjectBound(Object))
        {
            Unbind();
            return true;
        }
        else
        {
            return false;
        }
    }

    /**
     * Retrieve the bound object, returns nullptr for non-member delegates 
     * 
     * @return: Returns the pointer to the object bound to the delegate
     */
    FORCEINLINE const void* GetBoundObject() const
    {
        if (IsBound())
        {
            return GetDelegate()->GetBoundObject();
        }
        else
        {
            return nullptr;
        }
    }

    /**
     * Retrieve the delegate handle for this delegate
     * 
     * @return: Returns the delegate handle to this delegate
     */
    FORCEINLINE CDelegateHandle GetHandle() const
    {
        if (IsBound())
        {
            return GetDelegate()->GetHandle();
        }
        else
        {
            return CDelegateHandle();
        }
    }

    /**
     * Move-assignment operator
     * 
     * @param Rhs: Instance to move from
     * @return: A reference to this instance
     */
    FORCEINLINE CDelegateBase& operator=(CDelegateBase&& Rhs) noexcept
    {
        CDelegateBase(Move(Rhs)).Swap(*this);
        return *this;
    }

    /**
     * Copy-assignment operator
     *
     * @param Rhs: Instance to copy from
     * @return: A reference to this instance
     */
    FORCEINLINE CDelegateBase& operator=(const CDelegateBase& Rhs)
    {
        CDelegateBase(Rhs).Swap(*this);
        return *this;
    }

protected:

    // TODO: Should allocator use the element type at all? 
    using AllocatorType = TInlineArrayAllocator<int8, InlineBytes>;

    FORCEINLINE explicit CDelegateBase()
        : Storage()
        , Size(0)
    {
    }

    FORCEINLINE void Release()
    {
        if (IsBound())
        {
            GetDelegate()->~IDelegateInstance();
        }
    }

    FORCEINLINE void CopyFrom(const CDelegateBase& Other) noexcept
    {
        if (Other.IsBound())
        {
            int32 CurrentSize = Size;
            Storage.Realloc(CurrentSize, Other.Size);
            Other.GetDelegate()->Clone(Storage.GetAllocation());

            Size = Other.Size;
        }
        else
        {
            Size = 0;
            Storage.Free();
        }
    }

    FORCEINLINE void* AllocateStorage(int32 NewSize)
    {
        int32 PreviousSize = Size;
        Size = NewSize;
        return Storage.Realloc(PreviousSize, Size);
    }

    FORCEINLINE IDelegateInstance* GetDelegate() noexcept
    {
        return reinterpret_cast<IDelegateInstance*>(Storage.GetAllocation());
    }

    FORCEINLINE const IDelegateInstance* GetDelegate() const noexcept
    {
        return reinterpret_cast<const IDelegateInstance*>(Storage.GetAllocation());
    }

    FORCEINLINE AllocatorType& GetStorage()
    {
        return Storage;
    }

    FORCEINLINE const AllocatorType& GetStorage() const
    {
        return Storage;
    }

    AllocatorType Storage;
    int32 Size;
};