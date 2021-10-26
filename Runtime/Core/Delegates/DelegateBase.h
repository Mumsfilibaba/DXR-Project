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
        , Size()
    {
        CopyFrom( Other );
    }

    /* Move constructor */
    FORCEINLINE CDelegateBase( CDelegateBase&& Other )
        : Storage()
        , Size( Other.Size )
    {
        Storage.MoveFrom( Move( Other.Storage ) );
        Other.Size = 0;
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
        AllocatorType TempStorage;
        TempStorage.MoveFrom( Move( Storage ) );
        Storage.MoveFrom( Move( Other.Storage ) );
        Other.Storage.MoveFrom( Move( TempStorage ) );

        ::Swap<int32>( Size, Other.Size );
    }

    /* Cheacks weather or not there exist any delegate bound */
    FORCEINLINE bool IsBound() const
    {
        return (Size > 0);
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

    /* Retrieve the bound object, returns nullptr for non-member delegates */
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

    /* Retrieve the delegate handle for this object */
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
    using AllocatorType = TInlineArrayAllocator<int8, InlineBytes>;

    /* Constructor for a empty delegate */
    FORCEINLINE explicit CDelegateBase()
        : Storage()
        , Size( 0 )
    {
    }

    /* Release the delegate */
    FORCEINLINE void Release()
    {
        if ( IsBound() )
        {
            GetDelegate()->~IDelegateInstance();
        }
    }

    /* Copy from another function */
    FORCEINLINE void CopyFrom( const CDelegateBase& Other ) noexcept
    {
        if ( Other.IsBound() )
        {
            int32 CurrentSize = Size;
            Storage.Realloc( CurrentSize, Other.Size );
            Other.GetDelegate()->Clone( Storage.GetAllocation() );

            Size = Other.Size;
        }
        else
        {
            Size = 0;
            Storage.Free();
        }
    }

    /* Allocate from storage, set size, and return the memory */
    FORCEINLINE void* AllocateStorage( int32 NewSize )
    {
        int32 PreviousSize = Size;
        Size = NewSize;
        return Storage.Realloc( PreviousSize, Size );
    }

    /* Internal function that is used to retrive the functor pointer */
    FORCEINLINE IDelegateInstance* GetDelegate() noexcept
    {
        return reinterpret_cast<IDelegateInstance*>(Storage.GetAllocation());
    }

    /* Internal function that is used to retrive the functor pointer */
    FORCEINLINE const IDelegateInstance* GetDelegate() const noexcept
    {
        return reinterpret_cast<const IDelegateInstance*>(Storage.GetAllocation());
    }

    /* Retrieve the storage */
    FORCEINLINE AllocatorType& GetStorage()
    {
        return Storage;
    }

    /* Retrieve the storage */
    FORCEINLINE const AllocatorType& GetStorage() const
    {
        return Storage;
    }

    /* Storage for the delegate instance */
    AllocatorType Storage;
    int32 Size;
};