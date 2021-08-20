#pragma once
#include "DelegateInstance.h"

#include "Core/Containers/Allocators.h"

/* Base class containing variables that does not require templates */
class CDelegateBase
{
    enum
    {
        // TODO: Look into padding so we can use larger structs?
        InlineBytes = 32
    };

public:

    /* Copy constructor */
    FORCEINLINE CDelegateBase( const CDelegateBase& Other )
        : Storage()
    {
        CopyFrom( Other );
    }

    /* Move constructor */
    FORCEINLINE CDelegateBase( CDelegateBase&& Other )
        : Storage( Move( Other.Storage ) )
    {
    }

    /* Destructor unbinding the delegate */
    FORCEINLINE ~CDelegateBase()
    {
        Unbind();
    }

    /* Unbinds the delegate */
    FORCEINLINE void Unbind()
    {
        Release();
    }

    /* Swaps two delegates */
    FORCEINLINE void Swap( CDelegateBase& Other )
    {
        AllocatorType TempStorage( Move( Storage ) );
        Storage = Move( Other.Storage );
        Other.Storage = Move( TempStorage );
    }

    /* Cheacks weather or not there exist any delegate bound */
    FORCEINLINE bool IsBound() const
    {
        return Storage.HasAllocation();
    }

    /* Check if object is bound to this delegate */
    FORCEINLINE bool IsObjectBound( const void* Object ) const
    {
        if ( Object != nullptr && IsBound() )
        {
            return GetDelegate()->IsObjectBound( Object );
        }
        else
        {
            return nullptr;
        }
    }

    /* Check if object is bound to this delegate */
    FORCEINLINE bool UnbindIfBound( const void* Object )
    {
        if ( IsObjectBound( Object ) )
        {
            Unbind();
            return true;
        }
        else
        {
            return false;
        }
    }

    /* Retrive the bound object, returns nullptr for non-member delegates */
    FORCEINLINE const void* GetBoundObject() const
    {
        if ( IsBound() )
        {
            return GetDelegate()->GetBoundObject();
        }
        else
        {
            return nullptr;
        }
    }

    /* Retrive the delegate handle for this object */
    FORCEINLINE CDelegateHandle GetHandle() const
    {
        if ( IsBound() )
        {
            return GetDelegate()->GetHandle();
        }
        else
        {
            return CDelegateHandle();
        }
    }

    /* Move assignment */
    FORCEINLINE CDelegateBase& operator=( CDelegateBase&& RHS )
    {
        CDelegateBase( Move( RHS ) ).Swap( *this );
        return *this;
    }

    /* Copy assignment */
    FORCEINLINE CDelegateBase& operator=( const CDelegateBase& RHS )
    {
        CDelegateBase( RHS ).Swap( *this );
        return *this;
    }

protected:

    // TODO: Should allocator use the element type at all? 
    using AllocatorType = TInlineAllocator<int8, InlineBytes>;

    /* Constructor for a empty delegate */
    FORCEINLINE explicit CDelegateBase()
        : Storage()
    {
    }

    /* Release the delegate */
    FORCEINLINE void Release()
    {
        if ( Storage.HasAllocation() )
        {
            GetDelegate()->~IDelegateInstance();
            Storage.Free();
        }
    }

    /* Copy from another function */
    FORCEINLINE void CopyFrom( const CDelegateBase& Other ) noexcept
    {
        if ( Other.IsBound() )
        {
            Storage.Allocate( Other.Storage.GetSize() );
            Other.GetDelegate()->Clone( Storage.Raw() );
        }
    }

    /* Internal function that is used to retrive the functor pointer */
    FORCEINLINE IDelegateInstance* GetDelegate() noexcept
    {
        return reinterpret_cast<IDelegateInstance*>(GetStorage().Raw());
    }

    /* Internal function that is used to retrive the functor pointer */
    FORCEINLINE const IDelegateInstance* GetDelegate() const noexcept
    {
        return reinterpret_cast<const IDelegateInstance*>(GetStorage().Raw());
    }

    /* Retrive the storage */
    FORCEINLINE AllocatorType& GetStorage()
    {
        return Storage;
    }

    /* Retrive the storage */
    FORCEINLINE const AllocatorType& GetStorage() const
    {
        return Storage;
    }

    /* Storage for the delegate instance */
    AllocatorType Storage;
};