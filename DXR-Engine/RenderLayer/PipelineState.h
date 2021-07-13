#pragma once
#include "ResourceBase.h"
#include "Shader.h"

class PipelineState : public Resource
{
public:
    virtual class GraphicsPipelineState* AsGraphics()
    {
        return nullptr;
    }
    virtual class ComputePipelineState* AsCompute()
    {
        return nullptr;
    }
    virtual class RayTracingPipelineState* AsRayTracing()
    {
        return nullptr;
    }
};

enum class EDepthWriteMask
{
    Zero = 0,
    All = 1
};

inline const char* ToString( EDepthWriteMask DepthWriteMask )
{
    switch ( DepthWriteMask )
    {
    case EDepthWriteMask::Zero: return "Zero";
    case EDepthWriteMask::All:  return "All";
    default: return "Unknown";
    }
}

enum class EStencilOp
{
    Keep = 1,
    Zero = 2,
    Replace = 3,
    IncrSat = 4,
    DecrSat = 5,
    Invert = 6,
    Incr = 7,
    Decr = 8
};

inline const char* ToString( EStencilOp StencilOp )
{
    switch ( StencilOp )
    {
    case EStencilOp::Keep:    return "Keep";
    case EStencilOp::Zero:    return "Zero";
    case EStencilOp::Replace: return "Replace";
    case EStencilOp::IncrSat: return "IncrSat";
    case EStencilOp::DecrSat: return "DecrSat";
    case EStencilOp::Invert:  return "Invert";
    case EStencilOp::Incr:    return "Incr";
    case EStencilOp::Decr:    return "Decr";
    default: return "Unknown";
    }
}

struct DepthStencilOp
{
    EStencilOp      StencilFailOp = EStencilOp::Keep;
    EStencilOp      StencilDepthFailOp = EStencilOp::Keep;
    EStencilOp      StencilPassOp = EStencilOp::Keep;
    EComparisonFunc StencilFunc = EComparisonFunc::Always;
};

struct DepthStencilStateCreateInfo
{
    EDepthWriteMask DepthWriteMask = EDepthWriteMask::All;
    EComparisonFunc DepthFunc = EComparisonFunc::Less;
    bool            DepthEnable = true;
    uint8           StencilReadMask = 0xff;
    uint8           StencilWriteMask = 0xff;
    bool            StencilEnable = false;
    DepthStencilOp  FrontFace = DepthStencilOp();
    DepthStencilOp  BackFace = DepthStencilOp();
};

class DepthStencilState : public Resource
{
};

enum class ECullMode
{
    None = 1,
    Front = 2,
    Back = 3
};

inline const char* ToString( ECullMode CullMode )
{
    switch ( CullMode )
    {
    case ECullMode::None:  return "None";
    case ECullMode::Front: return "Front";
    case ECullMode::Back:  return "Back";
    default: return "Unknown";
    }
}

enum class EFillMode
{
    WireFrame = 1,
    Solid = 2
};

inline const char* ToString( EFillMode FillMode )
{
    switch ( FillMode )
    {
    case EFillMode::WireFrame: return "WireFrame";
    case EFillMode::Solid:     return "Solid";
    default: return "Unknown";
    }
}

struct RasterizerStateCreateInfo
{
    EFillMode FillMode = EFillMode::Solid;
    ECullMode CullMode = ECullMode::Back;
    bool   FrontCounterClockwise = false;
    int32  DepthBias = 0;
    float  DepthBiasClamp = 0.0f;
    float  SlopeScaledDepthBias = 0.0f;
    bool   DepthClipEnable = true;
    bool   MultisampleEnable = false;
    bool   AntialiasedLineEnable = false;
    uint32 ForcedSampleCount = 0;
    bool   EnableConservativeRaster = false;
};

class RasterizerState : public Resource
{
};

enum class EBlend
{
    Zero = 1,
    One = 2,
    SrcColor = 3,
    InvSrcColor = 4,
    SrcAlpha = 5,
    InvSrcAlpha = 6,
    DestAlpha = 7,
    InvDestAlpha = 8,
    DestColor = 9,
    InvDestColor = 10,
    SrcAlphaSat = 11,
    BlendFactor = 12,
    InvBlendFactor = 13,
    Src1Color = 14,
    InvSrc1Color = 15,
    Src1Alpha = 16,
    InvSrc1Alpha = 17
};

inline const char* ToString( EBlend Blend )
{
    switch ( Blend )
    {
    case EBlend::Zero:           return "Zero";
    case EBlend::One:            return "One";
    case EBlend::SrcColor:       return "SrcColor";
    case EBlend::InvSrcColor:    return "InvSrcColor";
    case EBlend::SrcAlpha:       return "SrcAlpha";
    case EBlend::InvSrcAlpha:    return "InvSrcAlpha";
    case EBlend::DestAlpha:      return "DestAlpha";
    case EBlend::InvDestAlpha:   return "InvDestAlpha";
    case EBlend::DestColor:      return "DestColor";
    case EBlend::InvDestColor:   return "InvDestColor";
    case EBlend::SrcAlphaSat:    return "SrcAlphaSat";
    case EBlend::BlendFactor:    return "BlendFactor";
    case EBlend::InvBlendFactor: return "InvBlendFactor";
    case EBlend::Src1Color:      return "Src1Color";
    case EBlend::InvSrc1Color:   return "InvSrc1Color";
    case EBlend::Src1Alpha:      return "Src1Alpha";
    case EBlend::InvSrc1Alpha:   return "InvSrc1Alpha";
    default: return "Unknown";
    }
}

enum class EBlendOp
{
    Add = 1,
    Subtract = 2,
    RevSubtract = 3,
    Min = 4,
    Max = 5
};

inline const char* ToString( EBlendOp BlendOp )
{
    switch ( BlendOp )
    {
    case EBlendOp::Add:         return "Add";
    case EBlendOp::Subtract:    return "Subtract";
    case EBlendOp::RevSubtract: return "RevSubtract";
    case EBlendOp::Min:         return "Min";
    case EBlendOp::Max:         return "Max";
    default: return "Unknown";
    }
}

enum class ELogicOp
{
    Clear = 0,
    Set = 1,
    Copy = 2,
    CopyInverted = 3,
    Noop = 4,
    Invert = 5,
    And = 6,
    Nand = 7,
    Or = 8,
    Nor = 9,
    Xor = 10,
    Equiv = 11,
    AndReverse = 12,
    AndInverted = 13,
    OrReverse = 14,
    OrInverted = 15
};

inline const char* ToString( ELogicOp LogicOp )
{
    switch ( LogicOp )
    {
    case ELogicOp::Clear:        return "Clear";
    case ELogicOp::Set:          return "Set";
    case ELogicOp::Copy:         return "Copy";
    case ELogicOp::CopyInverted: return "CopyInverted";
    case ELogicOp::Noop:         return "Noop";
    case ELogicOp::Invert:       return "Invert";
    case ELogicOp::And:          return "And";
    case ELogicOp::Nand:         return "Nand";
    case ELogicOp::Or:           return "Or";
    case ELogicOp::Nor:          return "Nor";
    case ELogicOp::Xor:          return "Xor";
    case ELogicOp::Equiv:        return "Equiv";
    case ELogicOp::AndReverse:   return "AndReverse";
    case ELogicOp::AndInverted:  return "AndInverted";
    case ELogicOp::OrReverse:    return "OrReverse";
    case ELogicOp::OrInverted:   return "OrInverted";
    default: return "Unknown";
    }
}

enum EColorWriteFlag : uint8
{
    ColorWriteFlag_None = 0,
    ColorWriteFlag_Red = 1,
    ColorWriteFlag_Green = 2,
    ColorWriteFlag_Blue = 4,
    ColorWriteFlag_Alpha = 8,
    ColorWriteFlag_All = (((ColorWriteFlag_Red | ColorWriteFlag_Green) | ColorWriteFlag_Blue) | ColorWriteFlag_Alpha)
};

struct RenderTargetWriteState
{
    RenderTargetWriteState() = default;

    RenderTargetWriteState( uint8 InMask )
        : Mask( InMask )
    {
    }

    bool WriteNone() const
    {
        return Mask == ColorWriteFlag_None;
    }
    bool WriteRed() const
    {
        return (Mask & ColorWriteFlag_Red);
    }
    bool WriteGreen() const
    {
        return (Mask & ColorWriteFlag_Green);
    }
    bool WriteBlue() const
    {
        return (Mask & ColorWriteFlag_Blue);
    }
    bool WriteAlpha() const
    {
        return (Mask & ColorWriteFlag_Alpha);
    }
    bool WriteAll() const
    {
        return Mask == ColorWriteFlag_All;
    }

    uint8 Mask = ColorWriteFlag_All;
};

struct RenderTargetBlendState
{
    EBlend   SrcBlend = EBlend::One;
    EBlend   DestBlend = EBlend::Zero;
    EBlendOp BlendOp = EBlendOp::Add;
    EBlend   SrcBlendAlpha = EBlend::One;
    EBlend   DestBlendAlpha = EBlend::Zero;
    EBlendOp BlendOpAlpha = EBlendOp::Add;;
    ELogicOp LogicOp = ELogicOp::Noop;
    bool     BlendEnable = false;
    bool     LogicOpEnable = false;
    RenderTargetWriteState RenderTargetWriteMask;
};

struct BlendStateCreateInfo
{
    bool AlphaToCoverageEnable = false;
    bool IndependentBlendEnable = false;
    RenderTargetBlendState RenderTarget[8];
};

class BlendState : public Resource
{
};

enum class EInputClassification
{
    Vertex = 0,
    Instance = 1,
};

inline const char* ToString( EInputClassification BlendOp )
{
    switch ( BlendOp )
    {
    case EInputClassification::Vertex:   return "Vertex";
    case EInputClassification::Instance: return "Instance";
    default: return "Unknown";
    }
}

struct InputElement
{
    std::string          Semantic = "";
    uint32               SemanticIndex = 0;
    EFormat              Format = EFormat::Unknown;
    uint32               InputSlot = 0;
    uint32               ByteOffset = 0;
    EInputClassification InputClassification = EInputClassification::Vertex;
    uint32               InstanceStepRate = 0;
};

struct InputLayoutStateCreateInfo
{
    InputLayoutStateCreateInfo() = default;

    InputLayoutStateCreateInfo( const TArray<InputElement>& InElements )
        : Elements( InElements )
    {
    }

    InputLayoutStateCreateInfo( std::initializer_list<InputElement> InList )
        : Elements( InList )
    {
    }

    TArray<InputElement> Elements;
};

class InputLayoutState : public Resource
{
};

enum class EIndexBufferStripCutValue
{
    Disabled = 0,
    _0xffff = 1,
    _0xffffffff = 2
};

inline const char* ToString( EIndexBufferStripCutValue IndexBufferStripCutValue )
{
    switch ( IndexBufferStripCutValue )
    {
    case EIndexBufferStripCutValue::Disabled:    return "Disabled";
    case EIndexBufferStripCutValue::_0xffff:     return "0xffff";
    case EIndexBufferStripCutValue::_0xffffffff: return "0xffffffff";
    default: return "";
    }
}

struct PipelineRenderTargetFormats
{
    EFormat RenderTargetFormats[8];
    uint32  NumRenderTargets = 0;
    EFormat DepthStencilFormat = EFormat::Unknown;
};

struct GraphicsPipelineShaderState
{
    GraphicsPipelineShaderState() = default;

    GraphicsPipelineShaderState( VertexShader* InVertexShader, PixelShader* InPixelShader )
        : VertexShader( InVertexShader )
        , PixelShader( InPixelShader )
    {
    }

    VertexShader* VertexShader = nullptr;
    PixelShader* PixelShader = nullptr;
};

struct GraphicsPipelineStateCreateInfo
{
    InputLayoutState* InputLayoutState = nullptr;
    DepthStencilState* DepthStencilState = nullptr;
    RasterizerState* RasterizerState = nullptr;
    BlendState* BlendState = nullptr;
    uint32                      SampleCount = 1;
    uint32                      SampleQuality = 0;
    uint32                      SampleMask = 0xffffffff;
    EIndexBufferStripCutValue   IBStripCutValue = EIndexBufferStripCutValue::Disabled;
    EPrimitiveTopologyType      PrimitiveTopologyType = EPrimitiveTopologyType::Triangle;
    GraphicsPipelineShaderState ShaderState;
    PipelineRenderTargetFormats PipelineFormats;
};

class GraphicsPipelineState : public PipelineState
{
public:
    virtual GraphicsPipelineState* AsGraphics() override
    {
        return this;
    }
};

struct ComputePipelineStateCreateInfo
{
    ComputePipelineStateCreateInfo() = default;

    ComputePipelineStateCreateInfo( ComputeShader* InShader )
        : Shader( InShader )
    {
    }

    ComputeShader* Shader = nullptr;
};

class ComputePipelineState : public PipelineState
{
public:
    virtual ComputePipelineState* AsCompute() override
    {
        return this;
    }
};

struct RayTracingHitGroup
{
    RayTracingHitGroup() = default;

    RayTracingHitGroup( const std::string& InName, RayAnyHitShader* InAnyHit, RayClosestHitShader* InClosestHit )
        : Name( InName )
        , AnyHit( InAnyHit )
        , ClosestHit( InClosestHit )
    {
    }

    std::string          Name;
    RayAnyHitShader* AnyHit;
    RayClosestHitShader* ClosestHit;
};

struct RayTracingPipelineStateCreateInfo
{
    RayGenShader* RayGen = nullptr;
    TArray<RayAnyHitShader*>     AnyHitShaders;
    TArray<RayClosestHitShader*> ClosestHitShaders;
    TArray<RayMissShader*>       MissShaders;
    TArray<RayTracingHitGroup>   HitGroups;
    uint32 MaxAttributeSizeInBytes = 0;
    uint32 MaxPayloadSizeInBytes = 0;
    uint32 MaxRecursionDepth = 1;
};

class RayTracingPipelineState : public PipelineState
{
public:
    virtual RayTracingPipelineState* AsRayTracing() override
    {
        return this;
    }
};