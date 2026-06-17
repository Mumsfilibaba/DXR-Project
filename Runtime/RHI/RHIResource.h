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
    ShaderResourceView,
    UnorderedAccessView,
    RenderTargetView,
    DepthStencilView,
};

NODISCARD constexpr const CHAR* ToString(ERHIResourceType Type)
{
    switch (Type)
    {
        case ERHIResourceType::Buffer:                        return "Buffer";
        case ERHIResourceType::Texture:                       return "Texture";
        case ERHIResourceType::SwapChain:                     return "SwapChain";
        case ERHIResourceType::SamplerState:                  return "SamplerState";
        case ERHIResourceType::Fence:                         return "Fence";
        case ERHIResourceType::Query:                         return "Query";
        case ERHIResourceType::Shader:                        return "Shader";
        case ERHIResourceType::InputLayout:                   return "InputLayout";
        case ERHIResourceType::PipelineState:                 return "PipelineState";
        case ERHIResourceType::DepthStencilState:             return "DepthStencilState";
        case ERHIResourceType::RasterizerState:               return "RasterizerState";
        case ERHIResourceType::BlendState:                    return "BlendState";
        case ERHIResourceType::GeometryAccelerationStructure: return "GeometryAccelerationStructure";
        case ERHIResourceType::SceneAccelerationStructure:    return "SceneAccelerationStructure";
        case ERHIResourceType::ShaderResourceView:            return "ShaderResourceView";
        case ERHIResourceType::UnorderedAccessView:           return "UnorderedAccessView";
        case ERHIResourceType::RenderTargetView:              return "RenderTargetView";
        case ERHIResourceType::DepthStencilView:              return "DepthStencilView";
        default:                                              return "Unknown";
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
    mutable AtomicInt32         StrongReferences;
    mutable TAtomicEnum<EState> State;
};
