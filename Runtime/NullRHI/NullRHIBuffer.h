#pragma once
#include "RHI/RHIResources.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

template<typename T>
struct TNullRHIBuffer;

typedef TNullRHIBuffer<class FRHIVertexBuffer>            FNullRHIVertexBuffer;
typedef TNullRHIBuffer<class FRHIIndexBuffer>             FNullRHIIndexBuffer;
typedef TNullRHIBuffer<class FRHIGenericBuffer>           FNullRHIGenericBuffer;
typedef TNullRHIBuffer<struct FNullRHIConstantBufferBase> FNullRHIConstantBuffer;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNullRHIConstantBufferBase

struct FNullRHIConstantBufferBase 
    : public FRHIConstantBuffer
{
    explicit FNullRHIConstantBufferBase(const FRHIConstantBufferInitializer& Initializer)
        : FRHIConstantBuffer(Initializer)
    { }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIConstantBuffer Interface
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TNullRHIBuffer

template<typename BaseBufferType>
struct TNullRHIBuffer final 
    : public BaseBufferType
{
    template<typename... ArgTypes>
    explicit TNullRHIBuffer(ArgTypes&&... Args)
        : BaseBufferType(Forward<ArgTypes>(Args)...)
    { }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIBuffer Interface

    virtual void* GetRHIBaseResource() const override final { return nullptr; }
    virtual void* GetRHIBaseBuffer()   override final       { return this; }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif