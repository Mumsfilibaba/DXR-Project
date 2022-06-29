#pragma once
#include "RHI/RHIResources.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIVertexBuffer

class CNullRHIVertexBuffer : public CRHIVertexBuffer
{
public:

    CNullRHIVertexBuffer(const FRHIVertexBufferInitializer& Initializer)
        : CRHIVertexBuffer(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIIndexBuffer

class CNullRHIIndexBuffer : public FRHIIndexBuffer
{
public:
    
    CNullRHIIndexBuffer(const FRHIIndexBufferInitializer& Initializer)
        : FRHIIndexBuffer(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIConstantBuffer

class CNullRHIConstantBuffer : public FRHIConstantBuffer
{
public:
    
    CNullRHIConstantBuffer(const FRHIConstantBufferInitializer& Initializer)
        : FRHIConstantBuffer(Initializer)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIConstantBuffer Interface
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIGenericBuffer

class CNullRHIGenericBuffer : public FRHIGenericBuffer
{
public:
    
    CNullRHIGenericBuffer(const FRHIGenericBufferInitializer& Initializer)
        : FRHIGenericBuffer(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TNullRHIBuffer

template<typename BaseBufferType>
class TNullRHIBuffer : public BaseBufferType
{
public:

    template<typename... ArgTypes>
    TNullRHIBuffer(ArgTypes&&... Args)
        : BaseBufferType(Forward<ArgTypes>(Args)...)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIBuffer Interface

    virtual void* GetRHIBaseResource() const override final { return nullptr; }

    virtual void* GetRHIBaseBuffer() override final { return this; }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif