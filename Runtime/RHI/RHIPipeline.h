#pragma once
#include "RHIShader.h"
#include "RHIResources.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EDepthWriteMask

enum class EDepthWriteMask : uint8
{
    Zero = 0x00,
    All  = 0xff
};

inline const char* ToString(EDepthWriteMask DepthWriteMask)
{
    switch (DepthWriteMask)
    {
    case EDepthWriteMask::Zero: return "Zero";
    case EDepthWriteMask::All:  return "All";
    default:                    return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EStencilOp

enum class EStencilOp : uint8
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

inline const char* ToString(EStencilOp StencilOp)
{
    switch (StencilOp)
    {
    case EStencilOp::Keep:                 return "Keep";
    case EStencilOp::Zero:                 return "Zero";
    case EStencilOp::Replace:              return "Replace";
    case EStencilOp::IncrementAndSaturate: return "IncrementAndSaturate";
    case EStencilOp::DecrementAndSaturate: return "DecrementAndSaturate";
    case EStencilOp::Invert:               return "Invert";
    case EStencilOp::Increment:            return "Increment";
    case EStencilOp::Decrement:            return "Decrement";
    default:                               return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SDepthStencilStateFaceDesc

struct SDepthStencilStateFaceDesc
{
    SDepthStencilStateFaceDesc()
        : StencilFailOp(EStencilOp::Keep)
        , StencilDepthFailOp(EStencilOp::Keep)
        , StencilPassOp(EStencilOp::Keep)
        , StencilFunc(EComparisonFunc::Always)
    { }

    SDepthStencilStateFaceDesc( EStencilOp InStencilFailOp
                              , EStencilOp InStencilDepthFailOp
                              , EStencilOp InStencilPassOp
                              , EComparisonFunc InStencilFunc)
        : StencilFailOp(InStencilFailOp)
        , StencilDepthFailOp(InStencilDepthFailOp)
        , StencilPassOp(InStencilPassOp)
        , StencilFunc(InStencilFunc)
    { }

    bool operator==(const SDepthStencilStateFaceDesc& RHS) const
    {
        return (StencilFailOp      == RHS.StencilFailOp) 
            && (StencilDepthFailOp == RHS.StencilDepthFailOp)
            && (StencilPassOp      == RHS.StencilPassOp)
            && (StencilFunc        == RHS.StencilFunc);
    }

    bool operator!=(const SDepthStencilStateFaceDesc& RHS) const
    {
        return !(*this == RHS);
    }

    EStencilOp      StencilFailOp;
    EStencilOp      StencilDepthFailOp;
    EStencilOp      StencilPassOp;
    EComparisonFunc StencilFunc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SDepthStencilStateDesc

struct SDepthStencilStateDesc
{
    SDepthStencilStateDesc()
        : DepthWriteMask(EDepthWriteMask::All)
        , DepthFunc(EComparisonFunc::Less)
        , bDepthEnable(true)
        , StencilReadMask(0xff)
        , StencilWriteMask(0xff)
        , bStencilEnable(false)
        , FrontFace()
        , BackFace()
    { }

    SDepthStencilStateDesc( EDepthWriteMask InDepthWriteMask
                          , EComparisonFunc InDepthFunc
                          , bool bInDepthEnable
                          , uint8 InStencilReadMask
                          , uint8 InStencilWriteMask
                          , bool bInStencilEnable
                          , const SDepthStencilStateFaceDesc& InFrontFace
                          , const SDepthStencilStateFaceDesc& InBackFace)
        : DepthWriteMask(InDepthWriteMask)
        , DepthFunc(InDepthFunc)
        , bDepthEnable(bInDepthEnable)
        , StencilReadMask(InStencilReadMask)
        , StencilWriteMask(InStencilWriteMask)
        , bStencilEnable(bInStencilEnable)
        , FrontFace(InFrontFace)
        , BackFace(InBackFace)
    { }

    bool operator==(const SDepthStencilStateDesc& RHS) const
    {
        return (DepthWriteMask   == RHS.DepthWriteMask)
            && (DepthFunc        == RHS.DepthFunc)
            && (bDepthEnable     == RHS.bDepthEnable)
            && (StencilReadMask  == RHS.StencilReadMask)
            && (StencilWriteMask == RHS.StencilWriteMask)
            && (bStencilEnable   == RHS.bStencilEnable)
            && (FrontFace        == RHS.FrontFace)
            && (BackFace         == RHS.BackFace);
    }

    bool operator!=(const SDepthStencilStateDesc& RHS) const
    {
        return !(*this == RHS);
    }

    EDepthWriteMask            DepthWriteMask;
    EComparisonFunc            DepthFunc;
    bool                       bDepthEnable;
    uint8                      StencilReadMask;
    uint8                      StencilWriteMask;
    bool                       bStencilEnable;
    SDepthStencilStateFaceDesc FrontFace;
    SDepthStencilStateFaceDesc BackFace;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIDepthStencilState

class CRHIDepthStencilState : public CRHIResource
{
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ECullMode

enum class ECullMode : uint8
{
    None  = 1,
    Front = 2,
    Back  = 3
};

inline const char* ToString(ECullMode CullMode)
{
    switch (CullMode)
    {
    case ECullMode::None:  return "None";
    case ECullMode::Front: return "Front";
    case ECullMode::Back:  return "Back";
    default:               return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EFillMode

enum class EFillMode : uint8
{
    WireFrame = 1,
    Solid     = 2
};

inline const char* ToString(EFillMode FillMode)
{
    switch (FillMode)
    {
    case EFillMode::WireFrame: return "WireFrame";
    case EFillMode::Solid:     return "Solid";
    default:                   return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRasterizerStateDesc

struct SRasterizerStateDesc
{
    SRasterizerStateDesc()
        : FillMode(EFillMode::Solid)
        , CullMode(ECullMode::Back)
        , bFrontCounterClockwise(false)
        , DepthBias(0)
        , DepthBiasClamp(0.0f)
        , SlopeScaledDepthBias(0.0f)
        , bDepthClipEnable(true)
        , bMultisampleEnable(false)
        , bAntialiasedLineEnable(false)
        , ForcedSampleCount(0)
        , bEnableConservativeRaster(false)
    { }

    SRasterizerStateDesc( EFillMode InFillMode
                        , ECullMode InCullMode
                        , bool bInFrontCounterClockwise
                        , int32 InDepthBias
                        , float InDepthBiasClamp
                        , float InSlopeScaledDepthBias
                        , bool bInDepthClipEnable
                        , bool bInMultisampleEnable
                        , bool bInAntialiasedLineEnable
                        , uint32 InForcedSampleCount
                        , bool bInEnableConservativeRaster)
        : FillMode(InFillMode)
        , CullMode(InCullMode)
        , bFrontCounterClockwise(bInFrontCounterClockwise)
        , DepthBias(InDepthBias)
        , DepthBiasClamp(InDepthBiasClamp)
        , SlopeScaledDepthBias(InSlopeScaledDepthBias)
        , bDepthClipEnable(bInDepthClipEnable)
        , bMultisampleEnable(bInMultisampleEnable)
        , bAntialiasedLineEnable(bInAntialiasedLineEnable)
        , ForcedSampleCount(InForcedSampleCount)
        , bEnableConservativeRaster(bInEnableConservativeRaster)
    { }

    bool operator==(const SRasterizerStateDesc& RHS) const
    {
        return (FillMode                  == RHS.FillMode)
            && (CullMode                  == RHS.CullMode)
            && (bFrontCounterClockwise    == RHS.bFrontCounterClockwise)
            && (DepthBias                 == RHS.DepthBias)
            && (DepthBiasClamp            == RHS.DepthBiasClamp)
            && (SlopeScaledDepthBias      == RHS.SlopeScaledDepthBias)
            && (bDepthClipEnable          == RHS.bDepthClipEnable)
            && (bMultisampleEnable        == RHS.bMultisampleEnable)
            && (bAntialiasedLineEnable    == RHS.bAntialiasedLineEnable)
            && (ForcedSampleCount         == RHS.ForcedSampleCount)
            && (bEnableConservativeRaster == RHS.bEnableConservativeRaster);
    }

    bool operator!=(const SRasterizerStateDesc& RHS) const
    {
        return !(*this == RHS);
    }

    EFillMode FillMode;
    ECullMode CullMode;
    bool      bFrontCounterClockwise;
    bool      bDepthClipEnable;
    bool      bMultisampleEnable;
    bool      bAntialiasedLineEnable;
    bool      bEnableConservativeRaster;
    uint32    ForcedSampleCount;
    int32     DepthBias;
    float     DepthBiasClamp;
    float     SlopeScaledDepthBias;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRasterizerState

class CRHIRasterizerState : public CRHIResource
{
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EBlendType

enum class EBlendType : uint8
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

inline const char* ToString(EBlendType Blend)
{
    switch (Blend)
    {
    case EBlendType::Zero:           return "Zero";
    case EBlendType::One:            return "One";
    case EBlendType::SrcColor:       return "SrcColor";
    case EBlendType::InvSrcColor:    return "InvSrcColor";
    case EBlendType::SrcAlpha:       return "SrcAlpha";
    case EBlendType::InvSrcAlpha:    return "InvSrcAlpha";
    case EBlendType::DestAlpha:      return "DestAlpha";
    case EBlendType::InvDestAlpha:   return "InvDestAlpha";
    case EBlendType::DestColor:      return "DestColor";
    case EBlendType::InvDestColor:   return "InvDestColor";
    case EBlendType::SrcAlphaSat:    return "SrcAlphaSat";
    case EBlendType::BlendFactor:    return "BlendFactor";
    case EBlendType::InvBlendFactor: return "InvBlendFactor";
    case EBlendType::Src1Color:      return "Src1Color";
    case EBlendType::InvSrc1Color:   return "InvSrc1Color";
    case EBlendType::Src1Alpha:      return "Src1Alpha";
    case EBlendType::InvSrc1Alpha:   return "InvSrc1Alpha";
    default:                         return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EBlendOp

enum class EBlendOp : uint8
{
    Add         = 1,
    Subtract    = 2,
    RevSubtract = 3,
    Min         = 4,
    Max         = 5
};

inline const char* ToString(EBlendOp BlendOp)
{
    switch (BlendOp)
    {
    case EBlendOp::Add:         return "Add";
    case EBlendOp::Subtract:    return "Subtract";
    case EBlendOp::RevSubtract: return "RevSubtract";
    case EBlendOp::Min:         return "Min";
    case EBlendOp::Max:         return "Max";
    default:                    return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ELogicOp

enum class ELogicOp : uint8
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

inline const char* ToString(ELogicOp LogicOp)
{
    switch (LogicOp)
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
    default:                     return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EColorWriteFlag 

enum EColorWriteFlag : uint8
{
    None  = 0,
    Red   = FLAG(0),
    Green = FLAG(1),
    Blue  = FLAG(2),
    Alpha = FLAG(3),
    All   = (Red | Green | Blue | Alpha)
};

ENUM_CLASS_OPERATORS(EColorWriteFlag);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRenderTargetWriteState

struct SRenderTargetWriteState
{
    SRenderTargetWriteState()
        : Mask(EColorWriteFlag::All)
    { }

    SRenderTargetWriteState(EColorWriteFlag InMask)
        : Mask(InMask)
    { }

    FORCEINLINE bool WriteNone() const { return Mask == EColorWriteFlag::None; }

    FORCEINLINE bool WriteRed() const { return (Mask & EColorWriteFlag::Red); }

    FORCEINLINE bool WriteGreen() const { return (Mask & EColorWriteFlag::Green); }

    FORCEINLINE bool WriteBlue() const { return (Mask & EColorWriteFlag::Blue); }

    FORCEINLINE bool WriteAlpha() const { return (Mask & EColorWriteFlag::Alpha); }

    FORCEINLINE bool WriteAll() const { return (Mask == EColorWriteFlag::All); }

    FORCEINLINE bool operator==(SRenderTargetWriteState RHS) const
    {
        return (Mask == RHS.Mask);
    }

    FORCEINLINE bool operator!=(SRenderTargetWriteState RHS) const
    {
        return (Mask != RHS.Mask);
    }

    EColorWriteFlag Mask;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRenderTargetBlendState

struct SRenderTargetBlendStateDesc
{
    SRenderTargetBlendStateDesc()
        : SrcBlend(EBlendType::One)
        , DstBlend(EBlendType::Zero)
        , BlendOp(EBlendOp::Add)
        , SrcBlendAlpha(EBlendType::One)
        , DstBlendAlpha(EBlendType::Zero)
        , BlendOpAlpha(EBlendOp::Add)
        , LogicOp(ELogicOp::Noop)
        , bBlendEnable(false)
        , bLogicOpEnable(false)
        , RenderTargetWriteMask()
    { }

    SRenderTargetBlendStateDesc( EBlendType InSrcBlend
                               , EBlendType InDstBlend
                               , EBlendOp InBlendOp
                               , EBlendType InSrcBlendAlpha
                               , EBlendType InDstBlendAlpha
                               , EBlendOp InBlendOpAlpha
                               , ELogicOp InLogicOp
                               , bool bInBlendEnable
                               , bool bInLogicOpEnable
                               , SRenderTargetWriteState InRenderTargetWriteMask)
        : SrcBlend(InSrcBlend)
        , DstBlend(InDstBlend)
        , BlendOp(InBlendOp)
        , SrcBlendAlpha(InSrcBlendAlpha)
        , DstBlendAlpha(InDstBlendAlpha)
        , BlendOpAlpha(InBlendOpAlpha)
        , LogicOp(InLogicOp)
        , bBlendEnable(bInBlendEnable)
        , bLogicOpEnable(bInLogicOpEnable)
        , RenderTargetWriteMask(InRenderTargetWriteMask)
    { }

    bool operator==(const SRenderTargetBlendStateDesc& RHS) const
    {
        return (SrcBlend              == RHS.SrcBlend) 
            && (DstBlend              == RHS.DstBlend) 
            && (BlendOp               == RHS.BlendOp) 
            && (SrcBlendAlpha         == RHS.SrcBlendAlpha) 
            && (DstBlendAlpha         == RHS.DstBlendAlpha) 
            && (BlendOpAlpha          == RHS.BlendOpAlpha) 
            && (LogicOp               == RHS.LogicOp) 
            && (bBlendEnable          == RHS.bBlendEnable) 
            && (bLogicOpEnable        == RHS.bLogicOpEnable) 
            && (RenderTargetWriteMask == RHS.RenderTargetWriteMask);
    }

    bool operator!=(const SRenderTargetBlendStateDesc& RHS) const
    {
        return !(*this == RHS);
    }

    EBlendType              SrcBlend;
    EBlendType              DstBlend;
    EBlendOp                BlendOp;
    EBlendType              SrcBlendAlpha;
    EBlendType              DstBlendAlpha;
    EBlendOp                BlendOpAlpha;
    ELogicOp                LogicOp;
    bool                    bBlendEnable;
    bool                    bLogicOpEnable;
    SRenderTargetWriteState RenderTargetWriteMask;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SBlendStateDesc

enum
{
    MaxRenderTargetCount = 8
};

struct SBlendStateDesc
{
    SBlendStateDesc()
        : RenderTargets()
        , bAlphaToCoverageEnable(false)
        , bIndependentBlendEnable(false)
    { }

    SBlendStateDesc(const SRenderTargetBlendStateDesc& InRenderTarget, bool bInAlphaToCoverageEnable, bool bInIndependentBlendEnable)
        : RenderTargets()
        , bAlphaToCoverageEnable(bInAlphaToCoverageEnable)
        , bIndependentBlendEnable(bInIndependentBlendEnable)
    {
        RenderTargets[0] = InRenderTarget;
    }

    SBlendStateDesc( const SRenderTargetBlendStateDesc& InRenderTarget0
                   , const SRenderTargetBlendStateDesc& InRenderTarget1
                   , bool bInAlphaToCoverageEnable
                   , bool bInIndependentBlendEnable)
        : RenderTargets()
        , bAlphaToCoverageEnable(bInAlphaToCoverageEnable)
        , bIndependentBlendEnable(bInIndependentBlendEnable)
    {
        RenderTargets[0] = InRenderTarget0;
        RenderTargets[1] = InRenderTarget1;
    }

    SBlendStateDesc( const SRenderTargetBlendStateDesc& InRenderTarget0
                   , const SRenderTargetBlendStateDesc& InRenderTarget1
                   , const SRenderTargetBlendStateDesc& InRenderTarget2
                   , bool bInAlphaToCoverageEnable
                   , bool bInIndependentBlendEnable)
        : RenderTargets()
        , bAlphaToCoverageEnable(bInAlphaToCoverageEnable)
        , bIndependentBlendEnable(bInIndependentBlendEnable)
    {
        RenderTargets[0] = InRenderTarget0;
        RenderTargets[1] = InRenderTarget1;
        RenderTargets[2] = InRenderTarget2;
    }

    SBlendStateDesc( const SRenderTargetBlendStateDesc& InRenderTarget0
                   , const SRenderTargetBlendStateDesc& InRenderTarget1
                   , const SRenderTargetBlendStateDesc& InRenderTarget2
                   , const SRenderTargetBlendStateDesc& InRenderTarget3
                   , bool bInAlphaToCoverageEnable
                   , bool bInIndependentBlendEnable)
        : RenderTargets()
        , bAlphaToCoverageEnable(bInAlphaToCoverageEnable)
        , bIndependentBlendEnable(bInIndependentBlendEnable)
    {
        RenderTargets[0] = InRenderTarget0;
        RenderTargets[1] = InRenderTarget1;
        RenderTargets[2] = InRenderTarget2;
        RenderTargets[3] = InRenderTarget3;
    }

    bool operator==(const SBlendStateDesc& RHS) const
    {
        return (RenderTargets[0]        == RHS.RenderTargets[0])
            && (RenderTargets[1]        == RHS.RenderTargets[1])
            && (RenderTargets[2]        == RHS.RenderTargets[2])
            && (RenderTargets[3]        == RHS.RenderTargets[3])
            && (RenderTargets[4]        == RHS.RenderTargets[4])
            && (RenderTargets[5]        == RHS.RenderTargets[5])
            && (RenderTargets[6]        == RHS.RenderTargets[6])
            && (RenderTargets[7]        == RHS.RenderTargets[7])
            && (bAlphaToCoverageEnable  == RHS.bAlphaToCoverageEnable)
            && (bIndependentBlendEnable == RHS.bIndependentBlendEnable);
    }

    bool operator!=(const SBlendStateDesc& RHS) const
    {
        return !(*this == RHS);
    }

    SRenderTargetBlendStateDesc RenderTargets[MaxRenderTargetCount];
    bool                        bAlphaToCoverageEnable;
    bool                        bIndependentBlendEnable;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIBlendState

class CRHIBlendState : public CRHIResource
{
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EVertexInputClassification

enum class EVertexInputClass : uint8
{
    Vertex   = 0,
    Instance = 1,
};

inline const char* ToString(EVertexInputClass BlendOp)
{
    switch (BlendOp)
    {
    case EVertexInputClass::Vertex:   return "Vertex";
    case EVertexInputClass::Instance: return "Instance";
    default:                                   return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIInputElement

struct SVertexInputElement
{
    String            Semantic            = "";
    uint32            SemanticIndex       = 0;
    ERHIFormat        Format              = ERHIFormat::Unknown;
    uint32            InputSlot           = 0;
    uint32            ByteOffset          = 0;
    EVertexInputClass InputClassification = EVertexInputClass::Vertex;
    uint32            InstanceStepRate    = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SVertexInputLayoutDesc

struct SVertexInputLayoutDesc
{
    SVertexInputLayoutDesc()
        : Elements()
    { }

    SVertexInputLayoutDesc(const TArray<SVertexInputElement>& InElements)
        : Elements(InElements)
    { }

    SVertexInputLayoutDesc(std::initializer_list<SVertexInputElement> InList)
        : Elements(InList)
    { }

    TArray<SVertexInputElement> Elements;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIVertexInputLayout

class CRHIVertexInputLayout : public CRHIResource
{
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EIndexBufferStripCutValue

enum class EIndexBufferStripCutValue
{
    Disabled          = 0,
    uint16_0xffff     = 1,
    uint32_0xffffffff = 2
};

inline const char* ToString(EIndexBufferStripCutValue IndexBufferStripCutValue)
{
    switch (IndexBufferStripCutValue)
    {
    case EIndexBufferStripCutValue::Disabled:          return "Disabled";
    case EIndexBufferStripCutValue::uint16_0xffff:     return "0xffff";
    case EIndexBufferStripCutValue::uint32_0xffffffff: return "0xffffffff";
    default:                                           return "";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SPipelineRenderTargetDesc

struct SPipelineRenderTargetDesc
{
    ERHIFormat RenderTargetFormats[8];
    uint32     NumRenderTargets = 0;

    ERHIFormat DepthStencilFormat = ERHIFormat::Unknown;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIGraphicsPipelineShaderState

struct SGraphicsPipelineShaders
{
    SGraphicsPipelineShaders() = default;

    SGraphicsPipelineShaders(CRHIVertexShader* InVertexShader, CRHIPixelShader* InPixelShader)
        : VertexShader(InVertexShader)
        , PixelShader(InPixelShader)
    { }

    CRHIVertexShader* VertexShader = nullptr;
    CRHIPixelShader*  PixelShader  = nullptr;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIGraphicsPipelineStateDesc

struct SGraphicsPipelineStateDesc
{
    CRHIVertexInputLayout* InputLayoutState  = nullptr;
    CRHIDepthStencilState* DepthStencilState = nullptr;
    CRHIRasterizerState*   RasterizerState   = nullptr;
    CRHIBlendState*        BlendState        = nullptr;

    uint32 SampleCount   = 1;
    uint32 SampleQuality = 0;
    uint32 SampleMask    = 0xffffffff;

    EIndexBufferStripCutValue IBStripCutValue       = EIndexBufferStripCutValue::Disabled;
    EPrimitiveTopologyType    PrimitiveTopologyType = EPrimitiveTopologyType::Triangle;
    SGraphicsPipelineShaders  ShaderState;
    SPipelineRenderTargetDesc PipelineFormats;
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

    bool operator==(const SRHIRayTracingHitGroup& RHS) const
    {
        return (Name == RHS.Name) && (AnyHit == RHS.AnyHit) && (ClosestHit == RHS.ClosestHit);
    }

    bool operator!=(const SRHIRayTracingHitGroup& RHS) const
    {
        return !(*this == RHS);
    }

    String                   Name;
    CRHIRayAnyHitShader*     AnyHit;
    CRHIRayClosestHitShader* ClosestHit;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIRayTracingPipelineStateDesc

struct SRHIRayTracingPipelineStateDesc
{
    CRHIRayGenShader*                    RayGen = nullptr;

    TArrayView<CRHIRayAnyHitShader*>     AnyHitShaders;
    TArrayView<CRHIRayClosestHitShader*> ClosestHitShaders;
    TArrayView<CRHIRayMissShader*>       MissShaders;
    TArrayView<SRHIRayTracingHitGroup>   HitGroups;

    uint32 MaxAttributeSizeInBytes = 0;
    uint32 MaxPayloadSizeInBytes   = 0;
    uint32 MaxRecursionDepth       = 1;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingPipelineState

class CRHIRayTracingPipelineState : public CRHIResource
{
};