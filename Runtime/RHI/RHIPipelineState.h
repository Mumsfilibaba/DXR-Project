#pragma once
#include "RHI/RHIResource.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

enum class EStencilOp : uint8
{
    Keep    = 1,
    Zero    = 2,
    Replace = 3,
    IncrSat = 4,
    DecrSat = 5,
    Invert  = 6,
    Incr    = 7,
    Decr    = 8
};

constexpr const CHAR* ToString(EStencilOp StencilOp)
{
    switch (StencilOp)
    {
        case EStencilOp::Keep:    return "Keep";
        case EStencilOp::Zero:    return "Zero";
        case EStencilOp::Replace: return "Replace";
        case EStencilOp::IncrSat: return "IncrSat";
        case EStencilOp::DecrSat: return "DecrSat";
        case EStencilOp::Invert:  return "Invert";
        case EStencilOp::Incr:    return "Incr";
        case EStencilOp::Decr:    return "Decr";
        default:                  return "Unknown";
    }
}

struct FStencilState
{
    FStencilState()
        : StencilFailOp(EStencilOp::Keep)
        , StencilDepthFailOp(EStencilOp::Keep)
        , StencilDepthPassOp(EStencilOp::Keep)
        , StencilFunc(EComparisonFunc::Always)
    {
    }

    FStencilState(EStencilOp InStencilFailOp, EStencilOp InStencilDepthFailOp, EStencilOp InStencilPassOp, EComparisonFunc InStencilFunc)
        : StencilFailOp(InStencilFailOp)
        , StencilDepthFailOp(InStencilDepthFailOp)
        , StencilDepthPassOp(InStencilPassOp)
        , StencilFunc(InStencilFunc)
    {
    }

    bool operator==(const FStencilState& Other) const
    {
        return StencilFailOp == Other.StencilFailOp && StencilDepthFailOp == Other.StencilDepthFailOp && StencilDepthPassOp == Other.StencilDepthPassOp && StencilFunc == Other.StencilFunc;
    }

    bool operator!=(const FStencilState& Other) const
    {
        return !(*this == Other);
    }

    friend uint64 GetHashForType(const FStencilState& Value)
    {
        uint64 Hash = UnderlyingTypeValue(Value.StencilFailOp);
        HashCombine(Hash, UnderlyingTypeValue(Value.StencilDepthFailOp));
        HashCombine(Hash, UnderlyingTypeValue(Value.StencilDepthPassOp));
        HashCombine(Hash, UnderlyingTypeValue(Value.StencilFunc));
        return Hash;
    }

    EStencilOp      StencilFailOp;
    EStencilOp      StencilDepthFailOp;
    EStencilOp      StencilDepthPassOp;
    EComparisonFunc StencilFunc;
};

struct FRHIDepthStencilStateInitializer
{
    inline static constexpr uint32 DefaultStencilMask = 0xffffffff;

    FRHIDepthStencilStateInitializer()
        : DepthFunc(EComparisonFunc::Less)
        , bDepthWriteEnable(true)
        , bDepthEnable(true)
        , StencilReadMask(DefaultStencilMask)
        , StencilWriteMask(DefaultStencilMask)
        , bStencilEnable(false)
        , FrontFace()
        , BackFace()
    {
    }

    FRHIDepthStencilStateInitializer(
        EComparisonFunc      InDepthFunc,
        bool                 bInDepthEnable,
        bool                 bInDepthWriteEnable = true,
        bool                 bInStencilEnable    = false,
        uint32               InStencilReadMask   = DefaultStencilMask,
        uint32               InStencilWriteMask  = DefaultStencilMask,
        const FStencilState& InFrontFace         = FStencilState(),
        const FStencilState& InBackFace          = FStencilState())
        : DepthFunc(InDepthFunc)
        , bDepthWriteEnable(bInDepthWriteEnable)
        , bDepthEnable(bInDepthEnable)
        , StencilReadMask(InStencilReadMask)
        , StencilWriteMask(InStencilWriteMask)
        , bStencilEnable(bInStencilEnable)
        , FrontFace(InFrontFace)
        , BackFace(InBackFace)
    {
    }

    bool operator==(const FRHIDepthStencilStateInitializer& Other) const
    {
        return DepthFunc         == Other.DepthFunc
            && bDepthWriteEnable == Other.bDepthWriteEnable
            && bDepthEnable      == Other.bDepthEnable
            && StencilReadMask   == Other.StencilReadMask
            && StencilWriteMask  == Other.StencilWriteMask
            && bStencilEnable    == Other.bStencilEnable
            && FrontFace         == Other.FrontFace
            && BackFace          == Other.BackFace;
    }

    bool operator!=(const FRHIDepthStencilStateInitializer& Other) const
    {
        return !(*this == Other);
    }

    friend uint64 GetHashForType(const FRHIDepthStencilStateInitializer& Value)
    {
        uint64 Hash = static_cast<uint64>(Value.bDepthWriteEnable);
        HashCombine(Hash, UnderlyingTypeValue(Value.DepthFunc));
        HashCombine(Hash, Value.bDepthEnable);
        HashCombine(Hash, Value.StencilReadMask);
        HashCombine(Hash, Value.StencilWriteMask);
        HashCombine(Hash, Value.bStencilEnable);
        HashCombine(Hash, GetHashForType(Value.FrontFace));
        HashCombine(Hash, GetHashForType(Value.BackFace));
        return Hash;
    }

    EComparisonFunc DepthFunc;
    bool            bDepthWriteEnable;
    bool            bDepthEnable;
    uint32          StencilReadMask;
    uint32          StencilWriteMask;
    bool            bStencilEnable;
    FStencilState   FrontFace;
    FStencilState   BackFace;
};

class FRHIDepthStencilState : public FRHIResource
{
protected:
    FRHIDepthStencilState() = default;
    virtual ~FRHIDepthStencilState() = default;

public:
    virtual FRHIDepthStencilStateInitializer GetInitializer() const = 0;
};

enum class ECullMode : uint8
{
    None  = 1,
    Front = 2,
    Back  = 3
};

constexpr const CHAR* ToString(ECullMode CullMode)
{
    switch (CullMode)
    {
        case ECullMode::None:  return "None";
        case ECullMode::Front: return "Front";
        case ECullMode::Back:  return "Back";
        default:               return "Unknown";
    }
}

enum class EFillMode : uint8
{
    WireFrame = 1,
    Solid     = 2
};

constexpr const CHAR* ToString(EFillMode FillMode)
{
    switch (FillMode)
    {
        case EFillMode::WireFrame: return "WireFrame";
        case EFillMode::Solid:     return "Solid";
        default:                   return "Unknown";
    }
}

struct FRHIRasterizerStateInitializer
{
    FRHIRasterizerStateInitializer()
        : FillMode(EFillMode::Solid)
        , CullMode(ECullMode::Back)
        , bFrontCounterClockwise(false)
        , bDepthClipEnable(true)
        , bMultisampleEnable(false)
        , bAntialiasedLineEnable(false)
        , bEnableConservativeRaster(false)
        , bEnableDepthBias(true)
        , ForcedSampleCount(0)
        , DepthBias(0.0f)
        , DepthBiasClamp(0.0f)
        , SlopeScaledDepthBias(0.0f)
    {
    }

    FRHIRasterizerStateInitializer(
        EFillMode InFillMode,
        ECullMode InCullMode,
        bool      bInFrontCounterClockwise    = false,
        float     InDepthBias                 = 0.0f,
        float     InDepthBiasClamp            = 0.0f,
        float     InSlopeScaledDepthBias      = 0.0f,
        bool      bInDepthClipEnable          = true,
        bool      bInMultisampleEnable        = false,
        bool      bInAntialiasedLineEnable    = false,
        uint32    InForcedSampleCount         = 1,
        bool      bInEnableConservativeRaster = false,
        bool      bInEnableDepthBias          = true)
        : FillMode(InFillMode)
        , CullMode(InCullMode)
        , bFrontCounterClockwise(bInFrontCounterClockwise)
        , bDepthClipEnable(bInDepthClipEnable)
        , bMultisampleEnable(bInMultisampleEnable)
        , bAntialiasedLineEnable(bInAntialiasedLineEnable)
        , bEnableConservativeRaster(bInEnableConservativeRaster)
        , bEnableDepthBias(bInEnableDepthBias)
        , ForcedSampleCount(InForcedSampleCount)
        , DepthBias(InDepthBias)
        , DepthBiasClamp(InDepthBiasClamp)
        , SlopeScaledDepthBias(InSlopeScaledDepthBias)
    {
    }

    bool operator==(const FRHIRasterizerStateInitializer& Other) const
    {
        return FillMode                  == Other.FillMode
            && CullMode                  == Other.CullMode
            && bFrontCounterClockwise    == Other.bFrontCounterClockwise
            && DepthBias                 == Other.DepthBias
            && DepthBiasClamp            == Other.DepthBiasClamp
            && SlopeScaledDepthBias      == Other.SlopeScaledDepthBias
            && bDepthClipEnable          == Other.bDepthClipEnable
            && bMultisampleEnable        == Other.bMultisampleEnable
            && bAntialiasedLineEnable    == Other.bAntialiasedLineEnable
            && ForcedSampleCount         == Other.ForcedSampleCount
            && bEnableConservativeRaster == Other.bEnableConservativeRaster;
    }

    bool operator!=(const FRHIRasterizerStateInitializer& Other) const
    {
        return !(*this == Other);
    }

    friend uint64 GetHashForType(const FRHIRasterizerStateInitializer& Value)
    {
        uint64 Hash = UnderlyingTypeValue(Value.FillMode);
        HashCombine(Hash, UnderlyingTypeValue(Value.CullMode));
        HashCombine(Hash, Value.bFrontCounterClockwise);
        HashCombine(Hash, Value.bDepthClipEnable);
        HashCombine(Hash, Value.bMultisampleEnable);
        HashCombine(Hash, Value.bAntialiasedLineEnable);
        HashCombine(Hash, Value.bEnableConservativeRaster);
        HashCombine(Hash, Value.ForcedSampleCount);
        HashCombine(Hash, Value.DepthBias);
        HashCombine(Hash, Value.DepthBiasClamp);
        HashCombine(Hash, Value.SlopeScaledDepthBias);
        return Hash;
    }

    EFillMode FillMode;
    ECullMode CullMode;
    bool      bFrontCounterClockwise;
    bool      bDepthClipEnable;
    bool      bMultisampleEnable;
    bool      bAntialiasedLineEnable;
    bool      bEnableConservativeRaster;
    bool      bEnableDepthBias;
    uint32    ForcedSampleCount;
    float     DepthBias;
    float     DepthBiasClamp;
    float     SlopeScaledDepthBias;
};

class FRHIRasterizerState : public FRHIResource
{
protected:
    FRHIRasterizerState() = default;
    virtual ~FRHIRasterizerState() = default;

public:
    virtual FRHIRasterizerStateInitializer GetInitializer() const = 0;
};

enum class EBlendType : uint8
{
    Zero           = 1,
    One            = 2,
    SrcColor       = 3,
    InvSrcColor    = 4,
    SrcAlpha       = 5,
    InvSrcAlpha    = 6,
    DstAlpha       = 7,
    InvDstAlpha    = 8,
    DstColor       = 9,
    InvDstColor    = 10,
    SrcAlphaSat    = 11,
    BlendFactor    = 12,
    InvBlendFactor = 13,
    Src1Color      = 14,
    InvSrc1Color   = 15,
    Src1Alpha      = 16,
    InvSrc1Alpha   = 17
};

constexpr const CHAR* ToString(EBlendType  Blend)
{
    switch (Blend)
    {
        case EBlendType::Zero:           return "Zero";
        case EBlendType::One:            return "One";
        case EBlendType::SrcColor:       return "SrcColor";
        case EBlendType::InvSrcColor:    return "InvSrcColor";
        case EBlendType::SrcAlpha:       return "SrcAlpha";
        case EBlendType::InvSrcAlpha:    return "InvSrcAlpha";
        case EBlendType::DstAlpha:       return "DstAlpha";
        case EBlendType::InvDstAlpha:    return "InvDstAlpha";
        case EBlendType::DstColor:       return "DstColor";
        case EBlendType::InvDstColor:    return "InvDstColor";
        case EBlendType::SrcAlphaSat:    return "SrcAlphaSat";
        case EBlendType::BlendFactor:    return "BlendFactor";
        case EBlendType::InvBlendFactor: return "InvBlendFactor";
        case EBlendType::Src1Color:      return "Src1Color";
        case EBlendType::InvSrc1Color:   return "InvSrc1Color";
        case EBlendType::Src1Alpha:      return "Src1Alpha";
        case EBlendType::InvSrc1Alpha:   return "InvSrc1Alpha";
        default:                          return "Unknown";
    }
}

enum class EBlendOp : uint8
{
    Add         = 1,
    Subtract    = 2,
    RevSubtract = 3,
    Min         = 4,
    Max         = 5
};

constexpr const CHAR* ToString(EBlendOp BlendOp)
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

enum class ELogicOp : uint8
{
    Clear        = 0,
    Set          = 1,
    Copy         = 2,
    CopyInverted = 3,
    NoOp         = 4,
    Invert       = 5,
    And          = 6,
    Nand         = 7,
    Or           = 8,
    Nor          = 9,
    Xor          = 10,
    Equivalent   = 11,
    AndReverse   = 12,
    AndInverted  = 13,
    OrReverse    = 14,
    OrInverted   = 15
};

constexpr const CHAR* ToString(ELogicOp LogicOp)
{
    switch (LogicOp)
    {
        case ELogicOp::Clear:        return "Clear";
        case ELogicOp::Set:          return "Set";
        case ELogicOp::Copy:         return "Copy";
        case ELogicOp::CopyInverted: return "CopyInverted";
        case ELogicOp::NoOp:         return "NoOp";
        case ELogicOp::Invert:       return "Invert";
        case ELogicOp::And:          return "And";
        case ELogicOp::Nand:         return "Nand";
        case ELogicOp::Or:           return "Or";
        case ELogicOp::Nor:          return "Nor";
        case ELogicOp::Xor:          return "Xor";
        case ELogicOp::Equivalent:   return "Equivalent";
        case ELogicOp::AndReverse:   return "AndReverse";
        case ELogicOp::AndInverted:  return "AndInverted";
        case ELogicOp::OrReverse:    return "OrReverse";
        case ELogicOp::OrInverted:   return "OrInverted";
        default:                     return "Unknown";
    }
}

enum class EColorWriteFlags : uint8
{
    None  = 0,
    Red   = FLAG(0),
    Green = FLAG(1),
    Blue  = FLAG(2),
    Alpha = FLAG(3),
    All   = Red | Green | Blue | Alpha
};

ENUM_CLASS_OPERATORS(EColorWriteFlags);

struct FRenderTargetBlendInfo
{
    FRenderTargetBlendInfo()
        : SrcBlend(EBlendType::One)
        , DstBlend(EBlendType::Zero)
        , BlendOp(EBlendOp::Add)
        , SrcBlendAlpha(EBlendType::One)
        , DstBlendAlpha(EBlendType::Zero)
        , BlendOpAlpha(EBlendOp::Add)
        , bBlendEnable(false)
        , ColorWriteMask(EColorWriteFlags::All)
    {
    }

    FRenderTargetBlendInfo(
        bool             bInBlendEnable,
        EBlendType       InSrcBlend,
        EBlendType       InDstBlend,
        EBlendOp         InBlendOp        = EBlendOp::Add,
        EBlendType       InSrcBlendAlpha  = EBlendType::One,
        EBlendType       InDstBlendAlpha  = EBlendType::Zero,
        EBlendOp         InBlendOpAlpha   = EBlendOp::Add,
        EColorWriteFlags InColorWriteMask = EColorWriteFlags::All)
        : SrcBlend(InSrcBlend)
        , DstBlend(InDstBlend)
        , BlendOp(InBlendOp)
        , SrcBlendAlpha(InSrcBlendAlpha)
        , DstBlendAlpha(InDstBlendAlpha)
        , BlendOpAlpha(InBlendOpAlpha)
        , bBlendEnable(bInBlendEnable)
        , ColorWriteMask(InColorWriteMask)
    {
    }

    bool operator==(const FRenderTargetBlendInfo& Other) const
    {
        return SrcBlend       == Other.SrcBlend
            && DstBlend       == Other.DstBlend
            && BlendOp        == Other.BlendOp
            && SrcBlendAlpha  == Other.SrcBlendAlpha
            && DstBlendAlpha  == Other.DstBlendAlpha
            && BlendOpAlpha   == Other.BlendOpAlpha
            && bBlendEnable   == Other.bBlendEnable
            && ColorWriteMask == Other.ColorWriteMask;
    }

    bool operator!=(const FRenderTargetBlendInfo& Other) const
    {
        return !(*this == Other);
    }

    friend uint64 GetHashForType(const FRenderTargetBlendInfo& Value)
    {
        uint64 Hash = UnderlyingTypeValue(Value.SrcBlend);
        HashCombine(Hash, UnderlyingTypeValue(Value.DstBlend));
        HashCombine(Hash, UnderlyingTypeValue(Value.BlendOp));
        HashCombine(Hash, UnderlyingTypeValue(Value.SrcBlendAlpha));
        HashCombine(Hash, UnderlyingTypeValue(Value.DstBlendAlpha));
        HashCombine(Hash, UnderlyingTypeValue(Value.BlendOpAlpha));
        HashCombine(Hash, Value.bBlendEnable);
        HashCombine(Hash, UnderlyingTypeValue(Value.ColorWriteMask));
        return Hash;
    }

    EBlendType       SrcBlend;
    EBlendType       DstBlend;
    EBlendOp         BlendOp;
    EBlendType       SrcBlendAlpha;
    EBlendType       DstBlendAlpha;
    EBlendOp         BlendOpAlpha;
    bool             bBlendEnable;
    EColorWriteFlags ColorWriteMask;
};

static_assert(TAlignmentOf<FRenderTargetBlendInfo>::Value == sizeof(uint8), "FRenderTargetBlendInfo is assumed to aligned to a uint8");

struct FRHIBlendStateInitializer
{
    FRHIBlendStateInitializer()
        : RenderTargets()
        , NumRenderTargets(0)
        , LogicOp(ELogicOp::NoOp)
        , bLogicOpEnable(false)
        , bAlphaToCoverageEnable(false)
        , bIndependentBlendEnable(false)
    {
    }

    bool operator==(const FRHIBlendStateInitializer& Other) const
    {
        return FMemory::Memcmp(RenderTargets, Other.RenderTargets, sizeof(RenderTargets)) == 0
            && NumRenderTargets        == Other.NumRenderTargets
            && LogicOp                 == Other.LogicOp
            && bLogicOpEnable          == Other.bLogicOpEnable
            && bAlphaToCoverageEnable  == Other.bAlphaToCoverageEnable
            && bIndependentBlendEnable == Other.bIndependentBlendEnable;
    }

    bool operator!=(const FRHIBlendStateInitializer& Other) const
    {
        return !(*this == Other);
    }

    friend uint64 GetHashForType(const FRHIBlendStateInitializer& Value)
    {
        uint64 Hash = 0;
        for (uint32 Index = 0; Index < Value.NumRenderTargets; ++Index)
            HashCombine(Hash, GetHashForType(Value.RenderTargets[Index]));

        HashCombine(Hash, UnderlyingTypeValue(Value.LogicOp));
        HashCombine(Hash, Value.bLogicOpEnable);
        HashCombine(Hash, Value.bAlphaToCoverageEnable);
        HashCombine(Hash, Value.bIndependentBlendEnable);
        return Hash;
    }

    FRenderTargetBlendInfo RenderTargets[RHI_MAX_RENDER_TARGETS];
    uint8                  NumRenderTargets;
    ELogicOp               LogicOp;

    bool bLogicOpEnable          : 1;
    bool bAlphaToCoverageEnable  : 1;
    bool bIndependentBlendEnable : 1;
};

class FRHIBlendState : public FRHIResource
{
protected:
    FRHIBlendState() = default;
    virtual ~FRHIBlendState() = default;

public:
    virtual FRHIBlendStateInitializer GetInitializer() const = 0;
};

enum class EVertexInputClass : uint8
{
    Vertex   = 0,
    Instance = 1,
};

constexpr const CHAR* ToString(EVertexInputClass BlendOp)
{
    switch (BlendOp)
    {
        case EVertexInputClass::Vertex:   return "Vertex";
        case EVertexInputClass::Instance: return "Instance";
        default:                          return "Unknown";
    }
}

struct FVertexElement
{
    FVertexElement()
        : Semantic("")
        , SemanticIndex(0)
        , Format(EFormat::Unknown)
        , VertexStride(0)
        , InputSlot(0)
        , ByteOffset(0)
        , ShaderElementIndex(0)
        , InputClass(EVertexInputClass::Vertex)
        , InstanceStepRate(0)
    {
    }

    FVertexElement(
        const FString&    InSemantic,
        uint32            InSemanticIndex,
        EFormat           InFormat,
        uint16            InVertexStride,
        uint32            InInputSlot,
        uint32            InByteOffset,
        uint32            InShaderElementIndex,
        EVertexInputClass InInputClass,
        uint32            InInstanceStepRate)
        : Semantic(InSemantic)
        , SemanticIndex(InSemanticIndex)
        , Format(InFormat)
        , VertexStride(InVertexStride)
        , InputSlot(InInputSlot)
        , ByteOffset(InByteOffset)
        , ShaderElementIndex(InShaderElementIndex)
        , InputClass(InInputClass)
        , InstanceStepRate(InInstanceStepRate)
    {
    }

    bool operator==(const FVertexElement& Other) const
    {
        return Semantic           == Other.Semantic
            && SemanticIndex      == Other.SemanticIndex
            && Format             == Other.Format
            && VertexStride       == Other.VertexStride
            && InputSlot          == Other.InputSlot
            && ByteOffset         == Other.ByteOffset
            && ShaderElementIndex == Other.ShaderElementIndex
            && InputClass         == Other.InputClass
            && InstanceStepRate   == Other.InstanceStepRate;
    }

    bool operator!=(const FVertexElement& Other) const
    {
        return !(*this == Other);
    }

    // Semantic in the shader to match
    FString Semantic;

    // Index of the semantic in the shader
    uint32 SemanticIndex;

    // Format of this vertex-element
    EFormat Format;

    // Stride for each vertex in the vertex-stream that this element is a part of
    uint16 VertexStride;

    // Index of the vertex-stream that this element is a part of
    uint32 InputSlot;

    // Offset within the vertex-structure that this element is a part of
    uint32 ByteOffset;

    // Index of the element in the shader that this element matching
    uint32 ShaderElementIndex;

    // How often this element should be updated
    EVertexInputClass InputClass;

    // How many elements to increment per instance
    uint32 InstanceStepRate;
};

typedef TArray<FVertexElement> FRHIVertexLayoutInitializerList;

class FRHIVertexLayout : public FRHIResource
{
protected:
    FRHIVertexLayout() = default;
    virtual ~FRHIVertexLayout() = default;
    
public:
    virtual FRHIVertexLayoutInitializerList GetInitializerList() const = 0;
};

class FRHIPipelineState : public FRHIResource
{
protected:
    FRHIPipelineState() = default;
    virtual ~FRHIPipelineState() = default;

public:
    virtual void SetDebugName(const FString& InName) { }
    virtual FString GetDebugName() const { return ""; }
};

struct FGraphicsPipelineFormats
{
    FGraphicsPipelineFormats()
        : RenderTargetFormats()
        , NumRenderTargets(0)
        , DepthStencilFormat(EFormat::Unknown)
    {
    }

    bool operator==(const FGraphicsPipelineFormats& Other) const
    {
        if (DepthStencilFormat == Other.DepthStencilFormat && NumRenderTargets == Other.NumRenderTargets)
            return FMemory::Memcmp(RenderTargetFormats, Other.RenderTargetFormats, sizeof(RenderTargetFormats)) == 0;
            
        return false;
    }

    bool operator!=(const FGraphicsPipelineFormats& Other) const
    {
        return !(*this == Other);
    }

    EFormat RenderTargetFormats[RHI_MAX_RENDER_TARGETS];
    uint8   NumRenderTargets;
    EFormat DepthStencilFormat;
};

struct FGraphicsPipelineShaders
{
    FGraphicsPipelineShaders()
        : VertexShader(nullptr)
        , HullShader(nullptr)
        , DomainShader(nullptr)
        , GeometryShader(nullptr)
        , PixelShader(nullptr)
    {
    }

    FGraphicsPipelineShaders(
        FRHIVertexShader*   InVertexShader,
        FRHIHullShader*     InHullShader,
        FRHIDomainShader*   InDomainShader,
        FRHIGeometryShader* InGeometryShader,
        FRHIPixelShader*    InPixelShader)
        : VertexShader(InVertexShader)
        , HullShader(InHullShader)
        , DomainShader(InDomainShader)
        , GeometryShader(InGeometryShader)
        , PixelShader(InPixelShader)
    {
    }

    bool operator==(const FGraphicsPipelineShaders& Other) const
    {
        return VertexShader   == Other.VertexShader
            && HullShader     == Other.HullShader
            && DomainShader   == Other.DomainShader
            && GeometryShader == Other.GeometryShader
            && PixelShader    == Other.PixelShader;
    }

    bool operator!=(const FGraphicsPipelineShaders& Other) const
    {
        return !(*this == Other);
    }

    FRHIVertexShader*   VertexShader;
    FRHIHullShader*     HullShader;
    FRHIDomainShader*   DomainShader;
    FRHIGeometryShader* GeometryShader;
    FRHIPixelShader*    PixelShader;
};

struct FViewInstancingInfo
{
    FViewInstancingInfo()
        : NumArraySlices(0)
        , StartRenderTargetArrayIndex(0)
        , bEnableViewInstancing(false)
    {
    }

    bool operator==(const FViewInstancingInfo& Other) const 
    {
        return NumArraySlices == Other.NumArraySlices
            && StartRenderTargetArrayIndex == Other.StartRenderTargetArrayIndex
            && bEnableViewInstancing == Other.bEnableViewInstancing;
    }

    bool operator!=(const FViewInstancingInfo& Other) const
    {
        return !(*this == Other);
    }

    uint8 NumArraySlices;
    uint8 StartRenderTargetArrayIndex : 7;
    uint8 bEnableViewInstancing : 1;
};

struct FRHIGraphicsPipelineStateInitializer
{
    static constexpr uint32 DefaultSampleMask = 0xffffffff;

    FRHIGraphicsPipelineStateInitializer()
        : VertexInputLayout(nullptr)
        , DepthStencilState(nullptr)
        , RasterizerState(nullptr)
        , BlendState(nullptr)
        , SampleCount(1)
        , SampleQuality(0)
        , SampleMask(DefaultSampleMask)
        , PrimitiveTopology(EPrimitiveTopology::TriangleList)
        , bPrimitiveRestartEnable(false)
        , ShaderState()
        , PipelineFormats()
        , ViewInstancingInfo()
    {
    }

    FRHIGraphicsPipelineStateInitializer(
        FRHIVertexLayout*          InVertexInputLayout,
        FRHIDepthStencilState*          InDepthStencilState,
        FRHIRasterizerState*            InRasterizerState,
        FRHIBlendState*                 InBlendState,
        const FGraphicsPipelineShaders& InShaderState,
        const FGraphicsPipelineFormats& InPipelineFormats,
        EPrimitiveTopology              InPrimitiveTopology       = EPrimitiveTopology::TriangleList,
        uint32                          InSampleCount             = 1,
        uint32                          InSampleQuality           = 0,
        uint32                          InSampleMask              = DefaultSampleMask,
        bool                            bInPrimitiveRestartEnable = false)
        : VertexInputLayout(InVertexInputLayout)
        , DepthStencilState(InDepthStencilState)
        , RasterizerState(InRasterizerState)
        , BlendState(InBlendState)
        , SampleCount(InSampleCount)
        , SampleQuality(InSampleQuality)
        , SampleMask(InSampleMask)
        , PrimitiveTopology(InPrimitiveTopology)
        , bPrimitiveRestartEnable(bInPrimitiveRestartEnable)
        , ShaderState(InShaderState)
        , PipelineFormats(InPipelineFormats)
        , ViewInstancingInfo()
    {
    }

    bool operator==(const FRHIGraphicsPipelineStateInitializer& Other) const
    {
        return VertexInputLayout       == Other.VertexInputLayout
            && DepthStencilState       == Other.DepthStencilState
            && RasterizerState         == Other.RasterizerState
            && BlendState              == Other.BlendState
            && SampleCount             == Other.SampleCount
            && SampleQuality           == Other.SampleQuality
            && SampleMask              == Other.SampleMask
            && bPrimitiveRestartEnable == Other.bPrimitiveRestartEnable
            && PrimitiveTopology       == Other.PrimitiveTopology
            && ShaderState             == Other.ShaderState
            && PipelineFormats         == Other.PipelineFormats
            && ViewInstancingInfo      == Other.ViewInstancingInfo;
    }

    bool operator!=(const FRHIGraphicsPipelineStateInitializer& Other) const
    {
        return !(*this == Other);
    }

    // Weak reference to the VertexInputLayout being used
    FRHIVertexLayout* VertexInputLayout;

    // Weak reference to the DepthStencilState being used
    FRHIDepthStencilState* DepthStencilState;

    // Weak reference to the RasterizerState being used
    FRHIRasterizerState* RasterizerState;

    // Weak reference to the BlendState being used
    FRHIBlendState* BlendState;

    uint32 SampleCount;
    uint32 SampleQuality;
    uint32 SampleMask;

    EPrimitiveTopology PrimitiveTopology;
    bool               bPrimitiveRestartEnable;

    FGraphicsPipelineShaders ShaderState;
    FGraphicsPipelineFormats PipelineFormats;
    FViewInstancingInfo      ViewInstancingInfo;
};

class FRHIGraphicsPipelineState : public FRHIPipelineState
{
protected:
    FRHIGraphicsPipelineState()  = default;
    ~FRHIGraphicsPipelineState() = default;
};

struct FRHIComputePipelineStateInitializer
{
    FRHIComputePipelineStateInitializer()
        : Shader(nullptr)
    {
    }

    FRHIComputePipelineStateInitializer(FRHIComputeShader* InShader)
        : Shader(InShader)
    {
    }

    bool operator==(const FRHIComputePipelineStateInitializer& Other) const
    {
        return Shader == Other.Shader;
    }

    bool operator!=(const FRHIComputePipelineStateInitializer& Other) const
    {
        return !(*this == Other);
    }

    FRHIComputeShader* Shader;
};

class FRHIComputePipelineState : public FRHIPipelineState
{
protected:
    FRHIComputePipelineState() = default;
    virtual ~FRHIComputePipelineState() = default;
};

enum class ERayTracingHitGroupType : uint8
{
    Unknown    = 0,
    Triangles  = 1,
    Procedural = 2
};

struct FRHIRayTracingHitGroupInfo
{
    FRHIRayTracingHitGroupInfo() = default;

    FRHIRayTracingHitGroupInfo(const FString& InName, ERayTracingHitGroupType InType, TArrayView<FRHIRayTracingShader*> InRayTracingShaders)
        : Name(InName)
        , Type(InType)
        , Shaders(InRayTracingShaders)
    {
    }

    bool operator==(const FRHIRayTracingHitGroupInfo& Other) const
    {
        return Name == Other.Name && Shaders == Other.Shaders && Type == Other.Type;
    }

    bool operator!=(const FRHIRayTracingHitGroupInfo& Other) const
    {
        return !(*this == Other);
    }

    FString                       Name;
    ERayTracingHitGroupType       Type = ERayTracingHitGroupType::Unknown;
    TArray<FRHIRayTracingShader*> Shaders;
};

struct FRHIRayTracingPipelineStateInitializer
{
    FRHIRayTracingPipelineStateInitializer() = default;

    FRHIRayTracingPipelineStateInitializer(
        const TArrayView<FRHIRayGenShader*>&          InRayGenShaders,
        const TArrayView<FRHIRayCallableShader*>&     InCallableShaders,
        const TArrayView<FRHIRayTracingHitGroupInfo>& InHitGroups,
        const TArrayView<FRHIRayMissShader*>&         InMissShaders,
        uint32 InMaxAttributeSizeInBytes,
        uint32 InMaxPayloadSizeInBytes,
        uint32 InMaxRecursionDepth)
        : RayGenShaders(InRayGenShaders)
        , CallableShaders(InCallableShaders)
        , MissShaders(InMissShaders)
        , HitGroups(InHitGroups)
        , MaxAttributeSizeInBytes(InMaxAttributeSizeInBytes)
        , MaxPayloadSizeInBytes(InMaxPayloadSizeInBytes)
        , MaxRecursionDepth(InMaxRecursionDepth)
    {
    }

    bool operator==(const FRHIRayTracingPipelineStateInitializer& Other) const
    {
        return RayGenShaders           == Other.RayGenShaders
            && CallableShaders         == Other.CallableShaders
            && HitGroups               == Other.HitGroups
            && MissShaders             == Other.MissShaders
            && MaxAttributeSizeInBytes == Other.MaxAttributeSizeInBytes
            && MaxPayloadSizeInBytes   == Other.MaxPayloadSizeInBytes
            && MaxRecursionDepth       == Other.MaxRecursionDepth;
    }

    bool operator!=(const FRHIRayTracingPipelineStateInitializer& Other) const
    {
        return !(*this == Other);
    }

    TArray<FRHIRayGenShader*>          RayGenShaders;
    TArray<FRHIRayCallableShader*>     CallableShaders;
    TArray<FRHIRayMissShader*>         MissShaders;
    TArray<FRHIRayTracingHitGroupInfo> HitGroups;

    uint32 MaxAttributeSizeInBytes = 0;
    uint32 MaxPayloadSizeInBytes   = 0;
    uint32 MaxRecursionDepth       = 1;
};

class FRHIRayTracingPipelineState : public FRHIPipelineState
{
protected:
    FRHIRayTracingPipelineState() = default;
    virtual ~FRHIRayTracingPipelineState() = default;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
