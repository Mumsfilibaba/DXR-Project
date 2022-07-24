#pragma once
#include "RHIShader.h"
#include "RHIResourceBase.h"

#include "Core/Templates/UnderlyingType.h"
#include "Core/Containers/StaticArray.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class FRHIRasterizerState>         FRHIRasterizerStateRef;
typedef TSharedRef<class FRHIBlendState>              FRHIBlendStateRef;
typedef TSharedRef<class FRHIDepthStencilState>       FRHIDepthStencilStateRef;
typedef TSharedRef<class FRHIVertexInputLayout>       FRHIVertexInputLayoutRef;
typedef TSharedRef<class FRHIGraphicsPipelineState>   FRHIGraphicsPipelineStateRef;
typedef TSharedRef<class FRHIComputePipelineState>    FRHIComputePipelineStateRef;
typedef TSharedRef<class FRHIRayTracingPipelineState> FRHIRayTracingPipelineStateRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EDepthWriteMask

enum class EDepthWriteMask : uint8
{
    Zero = 0,
    All  = 1
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
    Keep    = 1,
    Zero    = 2,
    Replace = 3,
    IncrSat = 4,
    DecrSat = 5,
    Invert  = 6,
    Incr    = 7,
    Decr    = 8
};

inline const char* ToString(EStencilOp StencilOp)
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FDepthStencilStateFace

struct FDepthStencilStateFace
{
    FDepthStencilStateFace()
        : StencilFailOp(EStencilOp::Keep)
        , StencilDepthFailOp(EStencilOp::Keep)
        , StencilDepthPassOp(EStencilOp::Keep)
        , StencilFunc(EComparisonFunc::Always)
    { }

    FDepthStencilStateFace(
        EStencilOp InStencilFailOp,
        EStencilOp InStencilDepthFailOp,
        EStencilOp InStencilPassOp,
        EComparisonFunc InStencilFunc)
        : StencilFailOp(InStencilFailOp)
        , StencilDepthFailOp(InStencilDepthFailOp)
        , StencilDepthPassOp(InStencilPassOp)
        , StencilFunc(InStencilFunc)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = ToUnderlying(StencilFailOp);
        HashCombine(Hash, ToUnderlying(StencilDepthFailOp));
        HashCombine(Hash, ToUnderlying(StencilDepthPassOp));
        HashCombine(Hash, ToUnderlying(StencilFunc));
        return Hash;
    }

    bool operator==(const FDepthStencilStateFace& RHS) const
    {
        return (StencilFailOp      == RHS.StencilFailOp) 
            && (StencilDepthFailOp == RHS.StencilDepthFailOp)
            && (StencilDepthPassOp      == RHS.StencilDepthPassOp)
            && (StencilFunc        == RHS.StencilFunc);
    }

    bool operator!=(const FDepthStencilStateFace& RHS) const
    {
        return !(*this == RHS);
    }

    EStencilOp      StencilFailOp;
    EStencilOp      StencilDepthFailOp;
    EStencilOp      StencilDepthPassOp;
    EComparisonFunc StencilFunc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIDepthStencilStateInitializer

class FRHIDepthStencilStateInitializer
{
public:

    FRHIDepthStencilStateInitializer()
        : DepthWriteMask(EDepthWriteMask::All)
        , DepthFunc(EComparisonFunc::Less)
        , bDepthEnable(true)
        , StencilReadMask(0xff)
        , StencilWriteMask(0xff)
        , bStencilEnable(false)
        , FrontFace()
        , BackFace()
    { }

    FRHIDepthStencilStateInitializer(
        EComparisonFunc InDepthFunc,
        bool bInDepthEnable,
        EDepthWriteMask InDepthWriteMask = EDepthWriteMask::All,
        bool bInStencilEnable = false,
        uint8 InStencilReadMask = 0xff,
        uint8 InStencilWriteMask = 0xff,
        const FDepthStencilStateFace& InFrontFace = FDepthStencilStateFace(),
        const FDepthStencilStateFace& InBackFace = FDepthStencilStateFace())
        : DepthWriteMask(InDepthWriteMask)
        , DepthFunc(InDepthFunc)
        , bDepthEnable(bInDepthEnable)
        , StencilReadMask(InStencilReadMask)
        , StencilWriteMask(InStencilWriteMask)
        , bStencilEnable(bInStencilEnable)
        , FrontFace(InFrontFace)
        , BackFace(InBackFace)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = ToUnderlying(DepthWriteMask);
        HashCombine(Hash, ToUnderlying(DepthFunc));
        HashCombine(Hash, bDepthEnable);
        HashCombine(Hash, StencilReadMask);
        HashCombine(Hash, StencilWriteMask);
        HashCombine(Hash, bStencilEnable);
        HashCombine(Hash, FrontFace.GetHash());
        HashCombine(Hash, BackFace.GetHash());
        return Hash;
    }

    bool operator==(const FRHIDepthStencilStateInitializer& RHS) const
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

    bool operator!=(const FRHIDepthStencilStateInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    EDepthWriteMask        DepthWriteMask;
    EComparisonFunc        DepthFunc;
    bool                   bDepthEnable;
    uint8                  StencilReadMask;
    uint8                  StencilWriteMask;
    bool                   bStencilEnable;
    FDepthStencilStateFace FrontFace;
    FDepthStencilStateFace BackFace;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIDepthStencilState

class FRHIDepthStencilState : public FRHIResource
{
protected:

    FRHIDepthStencilState()  = default;
    ~FRHIDepthStencilState() = default;
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
// FRHIRasterizerStateInitializer

class FRHIRasterizerStateInitializer
{
public:

    FRHIRasterizerStateInitializer()
        : FillMode(EFillMode::Solid)
        , CullMode(ECullMode::Back)
        , bFrontCounterClockwise(false)
	    , bDepthClipEnable(true)
	    , bMultisampleEnable(false)
	    , bAntialiasedLineEnable(false)
	    , bEnableConservativeRaster(false)
	    , ForcedSampleCount(0)
	    , DepthBias(0)
	    , DepthBiasClamp(0.0f)
	    , SlopeScaledDepthBias(0.0f)
    { }

    FRHIRasterizerStateInitializer(
        EFillMode InFillMode,
        ECullMode InCullMode,
        bool bInFrontCounterClockwise = false,
        int32 InDepthBias = 0,
        float InDepthBiasClamp = 0.0f,
        float InSlopeScaledDepthBias = 0.0f,
        bool bInDepthClipEnable = true,
        bool bInMultisampleEnable = false,
        bool bInAntialiasedLineEnable = false,
        uint32 InForcedSampleCount = 1,
        bool bInEnableConservativeRaster = false)
        : FillMode(InFillMode)
        , CullMode(InCullMode)
        , bFrontCounterClockwise(bInFrontCounterClockwise)
	    , bDepthClipEnable(bInDepthClipEnable)
	    , bMultisampleEnable(bInMultisampleEnable)
	    , bAntialiasedLineEnable(bInAntialiasedLineEnable)
	    , bEnableConservativeRaster(bInEnableConservativeRaster)
	    , ForcedSampleCount(InForcedSampleCount)
	    , DepthBias(InDepthBias)
	    , DepthBiasClamp(InDepthBiasClamp)
	    , SlopeScaledDepthBias(InSlopeScaledDepthBias)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = ToUnderlying(FillMode);
        HashCombine(Hash, ToUnderlying(CullMode));
        HashCombine(Hash, bFrontCounterClockwise);
        HashCombine(Hash, bDepthClipEnable);
        HashCombine(Hash, bMultisampleEnable);
        HashCombine(Hash, bAntialiasedLineEnable);
        HashCombine(Hash, bEnableConservativeRaster);
        HashCombine(Hash, ForcedSampleCount);
        HashCombine(Hash, DepthBias);
        HashCombine(Hash, DepthBiasClamp);
        HashCombine(Hash, SlopeScaledDepthBias);
        return Hash;
    }

    bool operator==(const FRHIRasterizerStateInitializer& RHS) const
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

    bool operator!=(const FRHIRasterizerStateInitializer& RHS) const
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
// FRHIRasterizerState

class FRHIRasterizerState : public FRHIResource
{
protected:

    FRHIRasterizerState()  = default;
    ~FRHIRasterizerState() = default;
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

inline const char* ToString(EBlendType  Blend)
{
    switch (Blend)
    {
        case EBlendType ::Zero:           return "Zero";
        case EBlendType ::One:            return "One";
        case EBlendType ::SrcColor:       return "SrcColor";
        case EBlendType ::InvSrcColor:    return "InvSrcColor";
        case EBlendType ::SrcAlpha:       return "SrcAlpha";
        case EBlendType ::InvSrcAlpha:    return "InvSrcAlpha";
        case EBlendType ::DstAlpha:       return "DstAlpha";
        case EBlendType ::InvDstAlpha:    return "InvDstAlpha";
        case EBlendType ::DstColor:       return "DstColor";
        case EBlendType ::InvDstColor:    return "InvDstColor";
        case EBlendType ::SrcAlphaSat:    return "SrcAlphaSat";
        case EBlendType ::BlendFactor:    return "BlendFactor";
        case EBlendType ::InvBlendFactor: return "InvBlendFactor";
        case EBlendType ::Src1Color:      return "Src1Color";
        case EBlendType ::InvSrc1Color:   return "InvSrc1Color";
        case EBlendType ::Src1Alpha:      return "Src1Alpha";
        case EBlendType ::InvSrc1Alpha:   return "InvSrc1Alpha";
        default:                          return "Unknown";
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

enum class EColorWriteFlag : uint8
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
// FRenderTargetWriteState

struct FRenderTargetWriteState
{
    FRenderTargetWriteState()
        : Mask(EColorWriteFlag::All)
    { }

    FRenderTargetWriteState(EColorWriteFlag InMask)
        : Mask(InMask)
    { }

    FORCEINLINE bool WriteNone() const
    {
        return (Mask == EColorWriteFlag::None);
    }

    FORCEINLINE bool WriteRed() const
    {
        return ((Mask & EColorWriteFlag::Red) != EColorWriteFlag::None);
    }

    FORCEINLINE bool WriteGreen() const
    {
        return ((Mask & EColorWriteFlag::Green) != EColorWriteFlag::None);
    }

    FORCEINLINE bool WriteBlue() const
    {
        return ((Mask & EColorWriteFlag::Blue) != EColorWriteFlag::None);
    }

    FORCEINLINE bool WriteAlpha() const
    {
        return ((Mask & EColorWriteFlag::Alpha) != EColorWriteFlag::None);
    }

    FORCEINLINE bool WriteAll() const
    {
        return (Mask == EColorWriteFlag::All);
    }

    FORCEINLINE bool operator==(FRenderTargetWriteState RHS) const
    {
        return (Mask == RHS.Mask);
    }

    FORCEINLINE bool operator!=(FRenderTargetWriteState RHS) const
    {
        return (Mask != RHS.Mask);
    }

    EColorWriteFlag Mask;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRenderTargetBlendDesc

struct FRenderTargetBlendDesc
{
    FRenderTargetBlendDesc()
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

    FRenderTargetBlendDesc(
        bool bInBlendEnable,
        EBlendType InSrcBlend,
        EBlendType InDstBlend,
        EBlendOp InBlendOp = EBlendOp::Add,
        EBlendType InSrcBlendAlpha = EBlendType::One,
        EBlendType InDstBlendAlpha = EBlendType::Zero,
        EBlendOp InBlendOpAlpha = EBlendOp::Add,
        ELogicOp InLogicOp = ELogicOp::Noop,
        bool bInLogicOpEnable = false,
        FRenderTargetWriteState InRenderTargetWriteMask = FRenderTargetWriteState())
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

    uint64 GetHash() const
    {
        if (bBlendEnable && bLogicOpEnable)
        {
            return 0;
        }

        uint64 Hash = ToUnderlying(SrcBlend);
        HashCombine(Hash, ToUnderlying(DstBlend));
        HashCombine(Hash, ToUnderlying(BlendOp));
        HashCombine(Hash, ToUnderlying(SrcBlendAlpha));
        HashCombine(Hash, ToUnderlying(DstBlendAlpha));
        HashCombine(Hash, ToUnderlying(BlendOpAlpha));
        HashCombine(Hash, ToUnderlying(LogicOp));
        HashCombine(Hash, bBlendEnable);
        HashCombine(Hash, bLogicOpEnable);
        HashCombine(Hash, RenderTargetWriteMask.Mask);
        return Hash;
    }

    bool operator==(const FRenderTargetBlendDesc& RHS) const
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

    bool operator!=(const FRenderTargetBlendDesc& RHS) const
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
    FRenderTargetWriteState RenderTargetWriteMask;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIBlendStateInitializer

class FRHIBlendStateInitializer
{
public:

    FRHIBlendStateInitializer()
        : RenderTargets()
        , bAlphaToCoverageEnable(false)
        , bIndependentBlendEnable(false)
    { }

    FRHIBlendStateInitializer(
        const TStaticArray<FRenderTargetBlendDesc, kRHIMaxRenderTargetCount>& InRenderTargets,
        bool bInAlphaToCoverageEnable,
        bool bInIndependentBlendEnable)
        : RenderTargets(InRenderTargets)
        , bAlphaToCoverageEnable(bInAlphaToCoverageEnable)
        , bIndependentBlendEnable(bInIndependentBlendEnable)
    { }

    uint64 GetHash() const
    {
        uint64 Hash = 0;

        const uint32 NumRenderTargets = bIndependentBlendEnable ? kRHIMaxRenderTargetCount : 1u;
        for (uint32 Index = 0; Index < NumRenderTargets; ++Index)
        {
            HashCombine(Hash, RenderTargets[Index].GetHash());
        }

        HashCombine(Hash, bAlphaToCoverageEnable);
        HashCombine(Hash, bIndependentBlendEnable);
        return Hash;
    }

    bool operator==(const FRHIBlendStateInitializer& RHS) const
    {
        return (RenderTargets           == RHS.RenderTargets)
            && (bAlphaToCoverageEnable  == RHS.bAlphaToCoverageEnable)
            && (bIndependentBlendEnable == RHS.bIndependentBlendEnable);
    }

    bool operator!=(const FRHIBlendStateInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    TStaticArray<FRenderTargetBlendDesc, kRHIMaxRenderTargetCount> RenderTargets;
    bool bAlphaToCoverageEnable;
    bool bIndependentBlendEnable;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIBlendState

class FRHIBlendState : public FRHIResource
{
protected:

    FRHIBlendState()  = default;
    ~FRHIBlendState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EVertexInputClass

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
    default:                          return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FVertexInputElement

struct FVertexInputElement
{
    FVertexInputElement()
        : Semantic("")
        , SemanticIndex(0)
        , Format(EFormat::Unknown)
        , VertexStride(0)
        , InputSlot(0)
        , ByteOffset(0)
        , InputClass(EVertexInputClass::Vertex)
        , InstanceStepRate(0)
    { }

    FVertexInputElement(
        const FString& InSemantic,
        uint32 InSemanticIndex,
        EFormat InFormat,
        uint16 InVertexStride,
        uint32 InInputSlot,
        uint32 InByteOffset,
        EVertexInputClass InInputClass,
        uint32 InInstanceStepRate)
        : Semantic(InSemantic)
        , SemanticIndex(InSemanticIndex)
        , Format(InFormat)
        , VertexStride(InVertexStride)
        , InputSlot(InInputSlot)
        , ByteOffset(InByteOffset)
        , InputClass(InInputClass)
        , InstanceStepRate(InInstanceStepRate)
    { }

    bool operator==(const FVertexInputElement& RHS) const
    {
        return (Semantic         == RHS.Semantic)
            && (SemanticIndex    == RHS.SemanticIndex)
            && (Format           == RHS.Format)
            && (VertexStride     == RHS.VertexStride)
            && (InputSlot        == RHS.InputSlot)
            && (ByteOffset       == RHS.ByteOffset)
            && (InputClass       == RHS.InputClass)
            && (InstanceStepRate == RHS.InstanceStepRate);
    }

    bool operator!=(const FVertexInputElement& RHS) const
    {
        return !(*this == RHS);
    }

    FString            Semantic;
    uint32            SemanticIndex;
    EFormat           Format;
    uint16            VertexStride;
    uint32            InputSlot;
    uint32            ByteOffset;
    EVertexInputClass InputClass;
    uint32            InstanceStepRate;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIVertexInputLayoutInitializer

class FRHIVertexInputLayoutInitializer
{
public:

    FRHIVertexInputLayoutInitializer()
        : Elements()
    { }

    FRHIVertexInputLayoutInitializer(const TArray<FVertexInputElement>& InElements)
        : Elements(InElements)
    { }

    FRHIVertexInputLayoutInitializer(std::initializer_list<FVertexInputElement> InList)
        : Elements(InList)
    { }

    bool operator==(const FRHIVertexInputLayoutInitializer& RHS) const
    {
        return (Elements == RHS.Elements);
    }

    bool operator!=(const FRHIVertexInputLayoutInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    TArray<FVertexInputElement> Elements;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIVertexInputLayout

class FRHIVertexInputLayout : public FRHIResource
{
protected:

    FRHIVertexInputLayout()  = default;
    ~FRHIVertexInputLayout() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EIndexBufferStripCutValue

enum EIndexBufferStripCutValue : uint8
{
    IndexBufferStripCutValue_Disabled   = 0,
    IndexBufferStripCutValue_0xffff     = 1,
    IndexBufferStripCutValue_0xffffffff = 2
};

inline const char* ToString(EIndexBufferStripCutValue IndexBufferStripCutValue)
{
    switch (IndexBufferStripCutValue)
    {
        case IndexBufferStripCutValue_Disabled:   return "IndexBufferStripCutValue_Disabled";
        case IndexBufferStripCutValue_0xffff:     return "IndexBufferStripCutValue_0xffff";
        case IndexBufferStripCutValue_0xffffffff: return "IndexBufferStripCutValue_0xffffffff";
        default:                                  return "";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIPipelineState

class FRHIPipelineState : public FRHIResource
{
protected:

    FRHIPipelineState()  = default;
    ~FRHIPipelineState() = default;

public:

    /** @brief: Set the name of the PipelineState */
    virtual void SetName(const FString& InName) { }

    /** @return: Returns the name of the PipelineState */
    virtual FString GetName() const { return ""; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FGraphicsPipelineFormats

struct FGraphicsPipelineFormats
{
    FGraphicsPipelineFormats()
        : RenderTargetFormats()
        , NumRenderTargets(0)
        , DepthStencilFormat(EFormat::Unknown)
    {
        RenderTargetFormats.Fill(EFormat::Unknown);
    }

    FGraphicsPipelineFormats(
        const TStaticArray<EFormat, kRHIMaxRenderTargetCount>& InRenderTargetFormats,
        uint32 InNumRenderTargets,
        EFormat InDepthStencilFormat = EFormat::Unknown)
        : RenderTargetFormats(InRenderTargetFormats)
        , NumRenderTargets(InNumRenderTargets)
        , DepthStencilFormat(InDepthStencilFormat)
    { }

    bool operator==(const FGraphicsPipelineFormats& RHS) const
    {
        return (RenderTargetFormats == RHS.RenderTargetFormats)
            && (NumRenderTargets    == RHS.NumRenderTargets)
            && (DepthStencilFormat  == RHS.DepthStencilFormat);
    }

    bool operator!=(const FGraphicsPipelineFormats& RHS) const
    {
        return !(*this == RHS);
    }

    TStaticArray<EFormat, kRHIMaxRenderTargetCount> RenderTargetFormats;
    uint32 NumRenderTargets;

    EFormat DepthStencilFormat;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FGraphicsPipelineShaders

struct FGraphicsPipelineShaders
{
    FGraphicsPipelineShaders()
        : VertexShader(nullptr)
        , PixelShader(nullptr)
    { }

    FGraphicsPipelineShaders(FRHIVertexShader* InVertexShader, FRHIPixelShader* InPixelShader)
        : VertexShader(InVertexShader)
        , PixelShader(InPixelShader)
    { }

    bool operator==(const FGraphicsPipelineShaders& RHS) const
    {
        return (VertexShader == RHS.VertexShader)
            && (PixelShader  == RHS.PixelShader);
    }

    bool operator!=(const FGraphicsPipelineShaders& RHS) const
    {
        return !(*this == RHS);
    }

    FRHIVertexShader* VertexShader = nullptr;
    FRHIPixelShader*  PixelShader  = nullptr;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIGraphicsPipelineStateInitializer

class FRHIGraphicsPipelineStateInitializer
{
public:

    FRHIGraphicsPipelineStateInitializer()
        : VertexInputLayout(nullptr)
        , DepthStencilState(nullptr)
        , RasterizerState(nullptr)
        , BlendState(nullptr)
        , SampleCount(1)
        , SampleQuality(0)
        , SampleMask(0xffffffff)
        , IBStripCutValue(IndexBufferStripCutValue_Disabled)
        , PrimitiveTopologyType(EPrimitiveTopologyType::Triangle)
        , ShaderState()
        , PipelineFormats()
    { }

    FRHIGraphicsPipelineStateInitializer(
        FRHIVertexInputLayout* InVertexInputLayout,
        FRHIDepthStencilState* InDepthStencilState,
        FRHIRasterizerState* InRasterizerState,
        FRHIBlendState* InBlendState,
        const FGraphicsPipelineShaders& InShaderState,
        const FGraphicsPipelineFormats& InPipelineFormats,
        EPrimitiveTopologyType InPrimitiveTopologyType = EPrimitiveTopologyType::Triangle,
        uint32 InSampleCount = 1,
        uint32 InSampleQuality = 0,
        uint32 InSampleMask = 0xffffffff,
        EIndexBufferStripCutValue InIBStripCutValue = IndexBufferStripCutValue_Disabled)
        : VertexInputLayout(InVertexInputLayout)
        , DepthStencilState(InDepthStencilState)
        , RasterizerState(InRasterizerState)
        , BlendState(InBlendState)
        , SampleCount(InSampleCount)
        , SampleQuality(InSampleQuality)
        , SampleMask(InSampleMask)
        , IBStripCutValue(InIBStripCutValue)
        , PrimitiveTopologyType(InPrimitiveTopologyType)
        , ShaderState(InShaderState)
        , PipelineFormats(InPipelineFormats)
    { }

    bool operator==(const FRHIGraphicsPipelineStateInitializer& RHS) const
    {
        return (VertexInputLayout     == RHS.VertexInputLayout)
            && (DepthStencilState     == RHS.DepthStencilState)
            && (RasterizerState       == RHS.RasterizerState)
            && (BlendState            == RHS.BlendState)
            && (SampleCount           == RHS.SampleCount)
            && (SampleQuality         == RHS.SampleQuality)
            && (SampleMask            == RHS.SampleMask)
            && (IBStripCutValue       == RHS.IBStripCutValue)
            && (PrimitiveTopologyType == RHS.PrimitiveTopologyType)
            && (ShaderState           == RHS.ShaderState)
            && (PipelineFormats       == RHS.PipelineFormats);
    }

    bool operator!=(const FRHIGraphicsPipelineStateInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    FRHIVertexInputLayout*    VertexInputLayout;
    FRHIDepthStencilState*    DepthStencilState;
    FRHIRasterizerState*      RasterizerState;
    FRHIBlendState*           BlendState;

    uint32                    SampleCount;
    uint32                    SampleQuality;
    uint32                    SampleMask;

    EIndexBufferStripCutValue IBStripCutValue;
    EPrimitiveTopologyType    PrimitiveTopologyType;
    FGraphicsPipelineShaders  ShaderState;
    FGraphicsPipelineFormats  PipelineFormats;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIGraphicsPipelineState

class FRHIGraphicsPipelineState : public FRHIPipelineState
{
protected:

    FRHIGraphicsPipelineState()  = default;
    ~FRHIGraphicsPipelineState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIComputePipelineStateInitializer

class FRHIComputePipelineStateInitializer
{
public:

    FRHIComputePipelineStateInitializer()
        : Shader(nullptr)
    { }

    FRHIComputePipelineStateInitializer(FRHIComputeShader* InShader)
        : Shader(InShader)
    { }

    bool operator==(const FRHIComputePipelineStateInitializer& RHS) const
    {
        return (Shader == RHS.Shader);
    }

    bool operator!=(const FRHIComputePipelineStateInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    FRHIComputeShader* Shader;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIComputePipelineState

class FRHIComputePipelineState : public FRHIPipelineState
{
protected:

    FRHIComputePipelineState()  = default;
    ~FRHIComputePipelineState() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERayTracingHitGroupType

enum class ERayTracingHitGroupType : uint8
{
    Unknown    = 0,
    Triangles  = 1,
    Procedural = 2
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIRayTracingHitGroupInitializer

class FRHIRayTracingHitGroupInitializer
{
public:

    FRHIRayTracingHitGroupInitializer()
        : Name()
        , Type(ERayTracingHitGroupType::Unknown)
        , Shaders()
    { }

    FRHIRayTracingHitGroupInitializer(
        const FString& InName,
        ERayTracingHitGroupType InType,
        const TArrayView<FRHIRayTracingShader*>& InRayTracingShaders)
        : Name(InName)
        , Type(InType)
        , Shaders(InRayTracingShaders)
    { }

    bool operator==(const FRHIRayTracingHitGroupInitializer& RHS) const
    {
        return (Name == RHS.Name) && (Shaders == RHS.Shaders) && (Type == RHS.Type);
    }

    bool operator!=(const FRHIRayTracingHitGroupInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    FString                        Name;
    ERayTracingHitGroupType       Type;
    TArray<FRHIRayTracingShader*> Shaders;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIRayTracingPipelineStateInitializer

class FRHIRayTracingPipelineStateInitializer
{
public:

    FRHIRayTracingPipelineStateInitializer()
        : RayGenShaders()
        , CallableShaders()
	    , MissShaders()
	    , HitGroups()
        , MaxAttributeSizeInBytes(0)
        , MaxPayloadSizeInBytes(0)
        , MaxRecursionDepth(1)
    { }

    FRHIRayTracingPipelineStateInitializer(
        const TArrayView<FRHIRayGenShader*>& InRayGenShaders,
        const TArrayView<FRHIRayCallableShader*>& InCallableShaders,
        const TArrayView<FRHIRayTracingHitGroupInitializer>& InHitGroups,
        const TArrayView<FRHIRayMissShader*>& InMissShaders,
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
    { }

    bool operator==(const FRHIRayTracingPipelineStateInitializer& RHS) const
    {
        return (RayGenShaders           == RHS.RayGenShaders)
            && (CallableShaders         == RHS.CallableShaders)
            && (HitGroups               == RHS.HitGroups)
            && (MissShaders             == RHS.MissShaders)
            && (MaxAttributeSizeInBytes == RHS.MaxAttributeSizeInBytes)
            && (MaxPayloadSizeInBytes   == RHS.MaxPayloadSizeInBytes)
            && (MaxRecursionDepth       == RHS.MaxRecursionDepth);
    }

    bool operator!=(const FRHIRayTracingPipelineStateInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    TArray<FRHIRayGenShader*>                 RayGenShaders;
    TArray<FRHIRayCallableShader*>            CallableShaders;
    TArray<FRHIRayMissShader*>                MissShaders;
    TArray<FRHIRayTracingHitGroupInitializer> HitGroups;
    
    uint32 MaxAttributeSizeInBytes;
    uint32 MaxPayloadSizeInBytes;
    uint32 MaxRecursionDepth;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIRayTracingPipelineState

class FRHIRayTracingPipelineState : public FRHIPipelineState
{
protected:

    FRHIRayTracingPipelineState()  = default;
    ~FRHIRayTracingPipelineState() = default;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
