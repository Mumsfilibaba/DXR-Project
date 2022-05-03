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

    CNullRHIVertexBuffer(const CRHIVertexBufferInitializer& Initializer)
        : CRHIVertexBuffer(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIIndexBuffer

class CNullRHIIndexBuffer : public CRHIIndexBuffer
{
public:
    
    CNullRHIIndexBuffer(const CRHIIndexBufferInitializer& Initializer)
        : CRHIIndexBuffer(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIConstantBuffer

class CNullRHIConstantBuffer : public CRHIConstantBuffer
{
public:
    
    CNullRHIConstantBuffer(const CRHIConstantBufferInitializer& Initializer)
        : CRHIConstantBuffer(Initializer)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIConstantBuffer Interface
    virtual CRHIDescriptorHandle GetBindlessHandle() const override final { return CRHIDescriptorHandle(); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIGenericBuffer

class CNullRHIGenericBuffer : public CRHIGenericBuffer
{
public:
    
    CNullRHIGenericBuffer(const CRHIGenericBufferInitializer& Initializer)
        : CRHIGenericBuffer(Initializer)
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
    // CRHIBuffer Interface

    virtual void* GetRHIBaseResource() const override final { return nullptr; }

    virtual void* GetRHIBaseBuffer() override final { return this; }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif