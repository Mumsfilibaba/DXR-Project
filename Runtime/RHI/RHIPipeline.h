#pragma once
#include "RHIShader.h"
#include "RHIResourceBase.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIDepthWriteMask

enum class ERHIDepthWriteMask
{
    Zero = 0,
    All  = 1
};

inline const char* ToString(ERHIDepthWriteMask DepthWriteMask)
{
    switch (DepthWriteMask)
    {
    case ERHIDepthWriteMask::Zero: return "Zero";
    case ERHIDepthWriteMask::All:  return "All";
    default:                       return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIStencilOp

enum class ERHIStencilOp
{
    Keep                 = 1,
    Zero                 = 2,
    Replace              = 3,
    IncrementAndSaturate = 4,
    DecrementAndSaturate = 5,
    Invert               = 6,
    Increment            = 7,
    Decrement            = 8
};

inline const char* ToString(ERHIStencilOp StencilOp)
{
    switch (StencilOp)
    {
    case ERHIStencilOp::Keep:                 return "Keep";
    case ERHIStencilOp::Zero:                 return "Zero";
    case ERHIStencilOp::Replace:              return "Replace";
    case ERHIStencilOp::IncrementAndSaturate: return "IncrementAndSaturate";
    case ERHIStencilOp::DecrementAndSaturate: return "DecrementAndSaturate";
    case ERHIStencilOp::Invert:               return "Invert";
    case ERHIStencilOp::Increment:            return "Increment";
    case ERHIStencilOp::Decrement:            return "Decrement";
    default:                                  return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIDepthStencilOp

struct SRHIDepthStencilOp
{
    ERHIStencilOp      StencilFailOp      = ERHIStencilOp::Keep;
    ERHIStencilOp      StencilDepthFailOp = ERHIStencilOp::Keep;
    ERHIStencilOp      StencilPassOp      = ERHIStencilOp::Keep;
    ERHIComparisonFunc StencilFunc        = ERHIComparisonFunc::Always;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIDepthStencilStateDesc

struct SRHIDepthStencilStateDesc
{
    ERHIDepthWriteMask DepthWriteMask   = ERHIDepthWriteMask::All;
    ERHIComparisonFunc DepthFunc        = ERHIComparisonFunc::Less;
    bool               bDepthEnable     = true;
    uint8              StencilReadMask  = 0xff;
    uint8              StencilWriteMask = 0xff;
    bool               bStencilEnable   = false;
    SRHIDepthStencilOp FrontFace        = SRHIDepthStencilOp();
    SRHIDepthStencilOp BackFace         = SRHIDepthStencilOp();
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIDepthStencilState

class CRHIDepthStencilState : public CRHIResource
{
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHICullMode

enum class ERHICullMode
{
    None  = 1,
    Front = 2,
    Back  = 3
};

inline const char* ToString(ERHICullMode CullMode)
{
    switch (CullMode)
    {
    case ERHICullMode::None:  return "None";
    case ERHICullMode::Front: return "Front";
    case ERHICullMode::Back:  return "Back";
    default:                  return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIFillMode

enum class ERHIFillMode
{
    WireFrame = 1,
    Solid     = 2
};

inline const char* ToString(ERHIFillMode FillMode)
{
    switch (FillMode)
    {
    case ERHIFillMode::WireFrame: return "WireFrame";
    case ERHIFillMode::Solid:     return "Solid";
    default:                      return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIRasterizerStateDesc

struct SRHIRasterizerStateDesc
{
    ERHIFillMode FillMode                  = ERHIFillMode::Solid;
    ERHICullMode CullMode                  = ERHICullMode::Back;
    bool         bFrontCounterClockwise    = false;
    int32        DepthBias                 = 0;
    float        DepthBiasClamp            = 0.0f;
    float        SlopeScaledDepthBias      = 0.0f;
    bool         bDepthClipEnable          = true;
    bool         bMultisampleEnable        = false;
    bool         bAntialiasedLineEnable    = false;
    uint32       ForcedSampleCount         = 0;
    bool         bEnableConservativeRaster = false;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRasterizerState

class CRHIRasterizerState : public CRHIResource
{
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIBlend

enum class ERHIBlend
{
    Zero           = 1,
    One            = 2,
    SrcColor       = 3,
    InvSrcColor    = 4,
    SrcAlpha       = 5,
    InvSrcAlpha    = 6,
    DestAlpha      = 7,
    InvDestAlpha   = 8,
    DestColor      = 9,
    InvDestColor   = 10,
    SrcAlphaSat    = 11,
    BlendFactor    = 12,
    InvBlendFactor = 13,
    Src1Color      = 14,
    InvSrc1Color   = 15,
    Src1Alpha      = 16,
    InvSrc1Alpha   = 17
};

inline const char* ToString(ERHIBlend Blend)
{
    switch (Blend)
    {
    case ERHIBlend::Zero:           return "Zero";
    case ERHIBlend::One:            return "One";
    case ERHIBlend::SrcColor:       return "SrcColor";
    case ERHIBlend::InvSrcColor:    return "InvSrcColor";
    case ERHIBlend::SrcAlpha:       return "SrcAlpha";
    case ERHIBlend::InvSrcAlpha:    return "InvSrcAlpha";
    case ERHIBlend::DestAlpha:      return "DestAlpha";
    case ERHIBlend::InvDestAlpha:   return "InvDestAlpha";
    case ERHIBlend::DestColor:      return "DestColor";
    case ERHIBlend::InvDestColor:   return "InvDestColor";
    case ERHIBlend::SrcAlphaSat:    return "SrcAlphaSat";
    case ERHIBlend::BlendFactor:    return "BlendFactor";
    case ERHIBlend::InvBlendFactor: return "InvBlendFactor";
    case ERHIBlend::Src1Color:      return "Src1Color";
    case ERHIBlend::InvSrc1Color:   return "InvSrc1Color";
    case ERHIBlend::Src1Alpha:      return "Src1Alpha";
    case ERHIBlend::InvSrc1Alpha:   return "InvSrc1Alpha";
    default:                        return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIBlendOp

enum class ERHIBlendOp
{
    Add         = 1,
    Subtract    = 2,
    RevSubtract = 3,
    Min         = 4,
    Max         = 5
};

inline const char* ToString(ERHIBlendOp BlendOp)
{
    switch (BlendOp)
    {
    case ERHIBlendOp::Add:         return "Add";
    case ERHIBlendOp::Subtract:    return "Subtract";
    case ERHIBlendOp::RevSubtract: return "RevSubtract";
    case ERHIBlendOp::Min:         return "Min";
    case ERHIBlendOp::Max:         return "Max";
    default:                       return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHILogicOp

enum class ERHILogicOp
{
    Clear        = 0,
    Set          = 1,
    Copy         = 2,
    CopyInverted = 3,
    Noop         = 4,
    Invert       = 5,
    And          = 6,
    Nand         = 7,
    Or           = 8,
    Nor          = 9,
    Xor          = 10,
    Equiv        = 11,
    AndReverse   = 12,
    AndInverted  = 13,
    OrReverse    = 14,
    OrInverted   = 15
};

inline const char* ToString(ERHILogicOp LogicOp)
{
    switch (LogicOp)
    {
    case ERHILogicOp::Clear:        return "Clear";
    case ERHILogicOp::Set:          return "Set";
    case ERHILogicOp::Copy:         return "Copy";
    case ERHILogicOp::CopyInverted: return "CopyInverted";
    case ERHILogicOp::Noop:         return "Noop";
    case ERHILogicOp::Invert:       return "Invert";
    case ERHILogicOp::And:          return "And";
    case ERHILogicOp::Nand:         return "Nand";
    case ERHILogicOp::Or:           return "Or";
    case ERHILogicOp::Nor:          return "Nor";
    case ERHILogicOp::Xor:          return "Xor";
    case ERHILogicOp::Equiv:        return "Equiv";
    case ERHILogicOp::AndReverse:   return "AndReverse";
    case ERHILogicOp::AndInverted:  return "AndInverted";
    case ERHILogicOp::OrReverse:    return "OrReverse";
    case ERHILogicOp::OrInverted:   return "OrInverted";
    default:                        return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIColorWriteFlag

enum ERHIColorWriteFlag : uint8
{
    ColorWriteFlag_None  = 0,
    ColorWriteFlag_Red   = 1,
    ColorWriteFlag_Green = 2,
    ColorWriteFlag_Blue  = 4,
    ColorWriteFlag_Alpha = 8,
    ColorWriteFlag_All   = (((ColorWriteFlag_Red | ColorWriteFlag_Green) | ColorWriteFlag_Blue) | ColorWriteFlag_Alpha)
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIRenderTargetWriteState

struct SRHIRenderTargetWriteState
{
    SRHIRenderTargetWriteState() = default;

    SRHIRenderTargetWriteState(uint8 InMask)
        : Mask(InMask)
    { }

    FORCEINLINE bool WriteNone() const
    {
        return Mask == ColorWriteFlag_None;
    }

    FORCEINLINE bool WriteRed() const
    {
        return (Mask & ColorWriteFlag_Red);
    }

    FORCEINLINE bool WriteGreen() const
    {
        return (Mask & ColorWriteFlag_Green);
    }

    FORCEINLINE bool WriteBlue() const
    {
        return (Mask & ColorWriteFlag_Blue);
    }

    FORCEINLINE bool WriteAlpha() const
    {
        return (Mask & ColorWriteFlag_Alpha);
    }

    FORCEINLINE bool WriteAll() const
    {
        return Mask == ColorWriteFlag_All;
    }

    uint8 Mask = ColorWriteFlag_All;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRenderTargetBlendState

struct SRenderTargetBlendState
{
    ERHIBlend   SrcBlend       = ERHIBlend::One;
    ERHIBlend   DestBlend      = ERHIBlend::Zero;
    ERHIBlendOp BlendOp        = ERHIBlendOp::Add;
    ERHIBlend   SrcBlendAlpha  = ERHIBlend::One;
    ERHIBlend   DestBlendAlpha = ERHIBlend::Zero;
    ERHIBlendOp BlendOpAlpha   = ERHIBlendOp::Add;
    ERHILogicOp LogicOp        = ERHILogicOp::Noop;

    bool bBlendEnable   = false;
    bool bLogicOpEnable = false;

    SRHIRenderTargetWriteState RenderTargetWriteMask;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIBlendStateDesc

struct SRHIBlendStateDesc
{
    bool bAlphaToCoverageEnable  = false;
    bool bIndependentBlendEnable = false;

    SRenderTargetBlendState RenderTarget[8];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIBlendState

class CRHIBlendState : public CRHIResource
{
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIInputClassification

enum class ERHIInputClassification
{
    Vertex   = 0,
    Instance = 1,
};

inline const char* ToString(ERHIInputClassification BlendOp)
{
    switch (BlendOp)
    {
    case ERHIInputClassification::Vertex:   return "Vertex";
    case ERHIInputClassification::Instance: return "Instance";
    default:                                return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIInputElement

struct SRHIInputElement
{
    String                  Semantic            = "";
    uint32                  SemanticIndex       = 0;
    ERHIFormat              Format              = ERHIFormat::Unknown;
    uint32                  InputSlot           = 0;
    uint32                  ByteOffset          = 0;
    ERHIInputClassification InputClassification = ERHIInputClassification::Vertex;
    uint32                  InstanceStepRate    = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIInputLayoutStateDesc

struct SRHIInputLayoutStateDesc
{
    SRHIInputLayoutStateDesc() = default;

    SRHIInputLayoutStateDesc(const TArray<SRHIInputElement>& InElements)
        : Elements(InElements)
    { }

    SRHIInputLayoutStateDesc(std::initializer_list<SRHIInputElement> InList)
        : Elements(InList)
    { }

    TArray<SRHIInputElement> Elements;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIInputLayoutState

class CRHIInputLayoutState : public CRHIResource
{
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIIndexBufferStripCutValue

enum class ERHIIndexBufferStripCutValue
{
    Disabled    = 0,
    _0xffff     = 1,
    _0xffffffff = 2
};

inline const char* ToString(ERHIIndexBufferStripCutValue IndexBufferStripCutValue)
{
    switch (IndexBufferStripCutValue)
    {
    case ERHIIndexBufferStripCutValue::Disabled:    return "Disabled";
    case ERHIIndexBufferStripCutValue::_0xffff:     return "0xffff";
    case ERHIIndexBufferStripCutValue::_0xffffffff: return "0xffffffff";
    default:                                        return "";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIPipelineRenderTargetFormats

struct SRHIPipelineRenderTargetFormats
{
    ERHIFormat RenderTargetFormats[8];
    uint32     NumRenderTargets = 0;

    ERHIFormat DepthStencilFormat = ERHIFormat::Unknown;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIPipelineState

class CRHIPipelineState : public CRHIResource
{
public:

    /**
     * @brief: Cast the PipelineState to a Graphics PipelineState
     *
     * @return: Returns a pointer to a Graphics PipelineState if the object implements it
     */
    virtual class CRHIGraphicsPipelineState* AsGraphics() { return nullptr; }
    
    /**
     * @brief: Cast the PipelineState to a Compute PipelineState
     *
     * @return: Returns a pointer to a Compute PipelineState if the object implements it
     */
    virtual class CRHIComputePipelineState* AsCompute() { return nullptr; }

    /**
     * @brief: Cast the PipelineState to a Ray tracing PipelineState
     *
     * @return: Returns a pointer to a Ray tracing PipelineState if the object implements it
     */
    virtual class CRHIRayTracingPipelineState* AsRayTracing() { return nullptr; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIGraphicsPipelineShaderState

struct SRHIGraphicsPipelineShaderState
{
    SRHIGraphicsPipelineShaderState() = default;

    SRHIGraphicsPipelineShaderState(CRHIVertexShader* InVertexShader, CRHIPixelShader* InPixelShader)
        : VertexShader(InVertexShader)
        , PixelShader(InPixelShader)
    { }

    CRHIVertexShader* VertexShader = nullptr;
    CRHIPixelShader*  PixelShader  = nullptr;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIGraphicsPipelineStateDesc

struct SRHIGraphicsPipelineStateDesc
{
    CRHIInputLayoutState*  InputLayoutState  = nullptr;
    CRHIDepthStencilState* DepthStencilState = nullptr;
    CRHIRasterizerState*   RasterizerState   = nullptr;
    CRHIBlendState*        BlendState        = nullptr;

    uint32 SampleCount   = 1;
    uint32 SampleQuality = 0;
    uint32 SampleMask    = 0xffffffff;

    ERHIIndexBufferStripCutValue    IBStripCutValue       = ERHIIndexBufferStripCutValue::Disabled;
    EPrimitiveTopologyType       PrimitiveTopologyType = EPrimitiveTopologyType::Triangle;
    SRHIGraphicsPipelineShaderState ShaderState;
    SRHIPipelineRenderTargetFormats PipelineFormats;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIGraphicsPipelineState

class CRHIGraphicsPipelineState : public CRHIPipelineState
{
public:

    /**
     * @brief: Cast the PipelineState to a Graphics PipelineState
     *
     * @return: Returns a pointer to a Graphics PipelineState if the object implements it
     */
    virtual CRHIGraphicsPipelineState* AsGraphics() override { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIComputePipelineStateDesc

struct SRHIComputePipelineStateDesc
{
    SRHIComputePipelineStateDesc() = default;

    SRHIComputePipelineStateDesc(CRHIComputeShader* InShader)
        : Shader(InShader)
    { }

    CRHIComputeShader* Shader = nullptr;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIComputePipelineState

class CRHIComputePipelineState : public CRHIPipelineState
{
public:

    /**
     * @brief: Cast the PipelineState to a Compute PipelineState
     *
     * @return: Returns a pointer to a Compute PipelineState if the object implements it
     */
    virtual CRHIComputePipelineState* AsCompute() override { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRayTracingHitGroup

struct SRHIRayTracingHitGroup
{
    SRHIRayTracingHitGroup() = default;

    SRHIRayTracingHitGroup(const String& InName, CRHIRayAnyHitShader* InAnyHit, CRHIRayClosestHitShader* InClosestHit)
        : Name(InName)
        , AnyHit(InAnyHit)
        , ClosestHit(InClosestHit)
    { }

    bool operator==(const SRHIRayTracingHitGroup& Rhs) const
    {
        return (Name == Rhs.Name) && (AnyHit == Rhs.AnyHit) && (ClosestHit == Rhs.ClosestHit);
    }

    bool operator!=(const SRHIRayTracingHitGroup& Rhs) const
    {
        return !(*this == Rhs);
    }

    String                   Name;
    CRHIRayAnyHitShader*     AnyHit;
    CRHIRayClosestHitShader* ClosestHit;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIRayTracingPipelineStateDesc

struct SRHIRayTracingPipelineStateDesc
{
    CRHIRayGenShader*                RayGen = nullptr;

    TArray<CRHIRayAnyHitShader*>     AnyHitShaders;
    TArray<CRHIRayClosestHitShader*> ClosestHitShaders;
    TArray<CRHIRayMissShader*>       MissShaders;
    TArray<SRHIRayTracingHitGroup>   HitGroups;

    uint32 MaxAttributeSizeInBytes = 0;
    uint32 MaxPayloadSizeInBytes   = 0;
    uint32 MaxRecursionDepth       = 1;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingPipelineState

class CRHIRayTracingPipelineState : public CRHIPipelineState
{
public:

    /**
     * @brief: Cast the PipelineState to a Ray tracing PipelineState
     * 
     * @return: Returns a pointer to a Ray tracing PipelineState if the object implements it
     */
    virtual CRHIRayTracingPipelineState* AsRayTracing() override { return this; }
};