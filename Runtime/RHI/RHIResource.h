#pragma once
#include "Core/IRefCounted.h"
#include "Core/Threading/Atomic.h"
#include "Core/Templates/Utility/NonCopyable.h"
#include "RHI/RHICore.h"
#include "RHI/RHITypes.h"

enum class ERHIResourceType : uint8
{
    Unknown = 0,
    Buffer,
    Texture,
    SwapChain,
    SamplerState,
    Fence,
    Query,
    Shader,
    InputLayout,
    PipelineState,
    DepthStencilState,
    RasterizerState,
    BlendState,
    GeometryAccelerationStructure,
    SceneAccelerationStructure,
    ClusterAccelerationStructure,
    ClusterTemplate,
    OpacityMicromap,
    ShaderBindingTable,
    ShaderResourceView,
    UnorderedAccessView,
    RenderTargetView,
    DepthStencilView,
};

NODISCARD constexpr const CHAR_T* ToString(ERHIResourceType Type)
{
    switch (Type)
    {
        case ERHIResourceType::Buffer:                        return TEXT("Buffer");
        case ERHIResourceType::Texture:                       return TEXT("Texture");
        case ERHIResourceType::SwapChain:                     return TEXT("SwapChain");
        case ERHIResourceType::SamplerState:                  return TEXT("SamplerState");
        case ERHIResourceType::Fence:                         return TEXT("Fence");
        case ERHIResourceType::Query:                         return TEXT("Query");
        case ERHIResourceType::Shader:                        return TEXT("Shader");
        case ERHIResourceType::InputLayout:                   return TEXT("InputLayout");
        case ERHIResourceType::PipelineState:                 return TEXT("PipelineState");
        case ERHIResourceType::DepthStencilState:             return TEXT("DepthStencilState");
        case ERHIResourceType::RasterizerState:               return TEXT("RasterizerState");
        case ERHIResourceType::BlendState:                    return TEXT("BlendState");
        case ERHIResourceType::GeometryAccelerationStructure: return TEXT("GeometryAccelerationStructure");
        case ERHIResourceType::SceneAccelerationStructure:    return TEXT("SceneAccelerationStructure");
        case ERHIResourceType::ClusterAccelerationStructure:  return TEXT("ClusterAccelerationStructure");
        case ERHIResourceType::ClusterTemplate:               return TEXT("ClusterTemplate");
        case ERHIResourceType::OpacityMicromap:               return TEXT("OpacityMicromap");
        case ERHIResourceType::ShaderBindingTable:            return TEXT("ShaderBindingTable");
        case ERHIResourceType::ShaderResourceView:            return TEXT("ShaderResourceView");
        case ERHIResourceType::UnorderedAccessView:           return TEXT("UnorderedAccessView");
        case ERHIResourceType::RenderTargetView:              return TEXT("RenderTargetView");
        case ERHIResourceType::DepthStencilView:              return TEXT("DepthStencilView");
        default:                                              return TEXT("Unknown");
    }
}

class RHI_API FRHIResource : public IRefCounted, public FNonCopyable
{
    enum class EState : int32
    {
        Unknown = 0,
        Alive,
        Deleted,
    };

public:
    FRHIResource(ERHIResourceType InResourceType);
    virtual ~FRHIResource();

    // IRefCounted Interface
    virtual int32 AddRef()  const override;
    virtual int32 Release() const override;
    
    virtual int32 GetRefCount() const override;

    NODISCARD ERHIResourceType GetResourceType() const
    {
        return ResourceType;
    }

private:
    ERHIResourceType            ResourceType;
    mutable FAtomicInt32        StrongReferences;
    mutable TAtomicEnum<EState> State;
};
