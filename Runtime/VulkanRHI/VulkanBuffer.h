#pragma once
#include "RHI/RHIResources.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanVertexBuffer

class CVulkanVertexBuffer : public CRHIVertexBuffer
{
public:
    CVulkanVertexBuffer(uint32 InNumVertices, uint32 InStride, uint32 InFlags)
        : CRHIVertexBuffer(InNumVertices, InStride, InFlags)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanIndexBuffer

class CVulkanIndexBuffer : public CRHIIndexBuffer
{
public:
    CVulkanIndexBuffer(ERHIIndexFormat InIndexFormat, uint32 InNumIndices, uint32 InFlags)
        : CRHIIndexBuffer(InIndexFormat, InNumIndices, InFlags)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanConstantBuffer

class CVulkanConstantBuffer : public CRHIConstantBuffer
{
public:
    CVulkanConstantBuffer(uint32 InSizeInBytes, uint32 InFlags)
        : CRHIConstantBuffer(InSizeInBytes, InFlags)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanStructuredBuffer

class CVulkanStructuredBuffer : public CRHIStructuredBuffer
{
public:
    CVulkanStructuredBuffer(uint32 InNumElements, uint32 InStride, uint32 InFlags)
        : CRHIStructuredBuffer(InNumElements, InStride, InFlags)
    {
    }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TVulkanBuffer

template<typename BaseBufferType>
class TVulkanBuffer : public BaseBufferType
{
public:

    template<typename... ArgTypes>
    TVulkanBuffer(ArgTypes&&... Args)
        : BaseBufferType(Forward<ArgTypes>(Args)...)
    {
    }

    virtual void* Map(uint32 Offset, uint32 InSize) override
    {
        return nullptr;
    }

    virtual void Unmap(uint32 Offset, uint32 InSize) override final
    {
    }

    virtual void SetName(const String& InName) override final
    {
        CRHIResource::SetName(InName);
    }

    virtual bool IsValid() const override
    {
        return true;
    }
};
