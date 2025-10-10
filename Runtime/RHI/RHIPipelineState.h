#pragma once
#include "RHI/RHIResource.h"

#define RHI_DEFAULT_STENCIl_MASK (0xffffffff)
#define RHI_DEFAULT_SAMPLE_MASK (0xffffffff)

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

NODISCARD constexpr const CHAR* ToString(EStencilOp StencilOp)
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
        
        default: return "Unknown";
    }
}

struct FRHIDepthStencilStateInfo
{
    struct FStencilState
    {
        constexpr bool operator==(const FStencilState& Other) const noexcept = default;

        NODISCARD friend uint64 GetHashForType(const FStencilState& Value)
        {
            uint64 Hash = UnderlyingTypeValue(Value.StencilFailOp);
            HashCombine(Hash, UnderlyingTypeValue(Value.StencilDepthFailOp));
            HashCombine(Hash, UnderlyingTypeValue(Value.StencilDepthPassOp));
            HashCombine(Hash, UnderlyingTypeValue(Value.StencilFunc));
            return Hash;
        }

        EStencilOp StencilFailOp      = EStencilOp::Keep;
        EStencilOp StencilDepthFailOp = EStencilOp::Keep;
        EStencilOp StencilDepthPassOp = EStencilOp::Keep;
        EComparisonFunc StencilFunc   = EComparisonFunc::Always;
    };

    constexpr bool operator==(const FRHIDepthStencilStateInfo& Other) const noexcept = default;

    NODISCARD friend uint64 GetHashForType(const FRHIDepthStencilStateInfo& Value)
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

    EComparisonFunc DepthFunc         = EComparisonFunc::Less;
    bool            bDepthWriteEnable = true;
    bool            bDepthEnable      = true;
    uint32          StencilReadMask   = RHI_DEFAULT_STENCIl_MASK;
    uint32          StencilWriteMask  = RHI_DEFAULT_STENCIl_MASK;
    bool            bStencilEnable    = false;
    FStencilState   FrontFace         = { };
    FStencilState   BackFace          = { };
};

class FRHIDepthStencilState : public FRHIResource
{
protected:
    FRHIDepthStencilState() = default;
    virtual ~FRHIDepthStencilState() = default;

public:
    virtual FRHIDepthStencilStateInfo GetInfo() const = 0;
};

enum class ECullMode : uint8
{
    None  = 1,
    Front = 2,
    Back  = 3
};

NODISCARD constexpr const CHAR* ToString(ECullMode CullMode)
{
    switch (CullMode)
    {
        case ECullMode::None:  return "None";
        case ECullMode::Front: return "Front";
        case ECullMode::Back:  return "Back";

        default: return "Unknown";
    }
}

enum class EFillMode : uint8
{
    WireFrame = 1,
    Solid     = 2
};

NODISCARD constexpr const CHAR* ToString(EFillMode FillMode)
{
    switch (FillMode)
    {
        case EFillMode::WireFrame: return "WireFrame";
        case EFillMode::Solid:     return "Solid";
        
        default: return "Unknown";
    }
}

struct FRHIRasterizerStateInfo
{
    constexpr bool operator==(const FRHIRasterizerStateInfo& Other) const noexcept = default;

    NODISCARD friend uint64 GetHashForType(const FRHIRasterizerStateInfo& Value)
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

    EFillMode FillMode                  = EFillMode::Solid;
    ECullMode CullMode                  = ECullMode::Back;
    bool      bFrontCounterClockwise    = false;
    bool      bDepthClipEnable          = true;
    bool      bMultisampleEnable        = false;
    bool      bAntialiasedLineEnable    = false;
    bool      bEnableConservativeRaster = false;
    bool      bEnableDepthBias          = true;
    uint32    ForcedSampleCount         = 0;
    float     DepthBias                 = 0.0f;
    float     DepthBiasClamp            = 0.0f;
    float     SlopeScaledDepthBias      = 0.0f;
};

class FRHIRasterizerState : public FRHIResource
{
protected:
    FRHIRasterizerState() = default;
    virtual ~FRHIRasterizerState() = default;

public:
    virtual FRHIRasterizerStateInfo GetInfo() const = 0;
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

NODISCARD constexpr const CHAR* ToString(EBlendType  Blend)
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
        
        default: return "Unknown";
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

NODISCARD constexpr const CHAR* ToString(EBlendOp BlendOp)
{
    switch (BlendOp)
    {
        case EBlendOp::Add:         return "Add";
        case EBlendOp::Subtract:    return "Subtract";
        case EBlendOp::RevSubtract: return "RevSubtract";
        case EBlendOp::Min:         return "Min";
        case EBlendOp::Max:         return "Max";
        
        default: return "Unknown";
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

NODISCARD constexpr const CHAR* ToString(ELogicOp LogicOp)
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
        
        default: return "Unknown";
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
    constexpr bool operator==(const FRenderTargetBlendInfo& Other) const noexcept = default;

    NODISCARD friend uint64 GetHashForType(const FRenderTargetBlendInfo& Value)
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

    EBlendType       SrcBlend       = EBlendType::One;
    EBlendType       DstBlend       = EBlendType::Zero;
    EBlendOp         BlendOp        = EBlendOp::Add;
    EBlendType       SrcBlendAlpha  = EBlendType::One;
    EBlendType       DstBlendAlpha  = EBlendType::Zero;
    EBlendOp         BlendOpAlpha   = EBlendOp::Add;
    bool             bBlendEnable   = false;
    EColorWriteFlags ColorWriteMask = EColorWriteFlags::All;
};

static_assert(TAlignmentOf<FRenderTargetBlendInfo>::Value == sizeof(uint8), "FRenderTargetBlendInfo is assumed to aligned to a uint8");

struct FRHIBlendStateInfo
{
    constexpr bool operator==(const FRHIBlendStateInfo& Other) const noexcept = default;

    NODISCARD friend uint64 GetHashForType(const FRHIBlendStateInfo& Value)
    {
        uint64 Hash = 0;
        for (uint32 Index = 0; Index < Value.NumRenderTargets; ++Index)
        {
            HashCombine(Hash, GetHashForType(Value.RenderTargets[Index]));
        }

        HashCombine(Hash, UnderlyingTypeValue(Value.LogicOp));
        HashCombine(Hash, Value.bLogicOpEnable);
        HashCombine(Hash, Value.bAlphaToCoverageEnable);
        HashCombine(Hash, Value.bIndependentBlendEnable);
        return Hash;
    }

    FRenderTargetBlendInfo RenderTargets[RHI_MAX_RENDER_TARGETS];
    uint8 NumRenderTargets = 0;
    
    ELogicOp LogicOp = ELogicOp::NoOp;
    bool bLogicOpEnable = false;
    bool bAlphaToCoverageEnable = false;
    bool bIndependentBlendEnable = false;
};

class FRHIBlendState : public FRHIResource
{
protected:
    FRHIBlendState() = default;
    virtual ~FRHIBlendState() = default;

public:
    virtual FRHIBlendStateInfo GetInfo() const = 0;
};

enum class EVertexInputClass : uint8
{
    Vertex   = 0,
    Instance = 1,
};

NODISCARD constexpr const CHAR* ToString(EVertexInputClass BlendOp)
{
    switch (BlendOp)
    {
        case EVertexInputClass::Vertex:   return "Vertex";
        case EVertexInputClass::Instance: return "Instance";
        
        default: return "Unknown";
    }
}

struct FRHIInputElementInfo
{
    /** @brief Semantic in the shader to match */
    FString Semantic;

    /** @brief Index of the semantic in the shader */
    uint32 SemanticIndex = 0;

    /** @brief Format of this vertex-element */
    EFormat Format = EFormat::Unknown;

    /** @brief Stride for each vertex in the vertex-stream that this element is a part of */
    uint16 VertexStride = 0;

    /** @brief Index of the vertex-stream that this element is a part of */
    uint32 InputSlot = 0;

    /** @brief Offset within the vertex-structure that this element is a part of */
    uint32 ByteOffset = 0;

    /** @brief Index of the element in the shader that this element matching */
    uint32 ShaderElementIndex = 0;

    /** @brief How often this element should be updated */
    EVertexInputClass InputClass = EVertexInputClass::Vertex;

    /** @brief How many elements to increment per instance */
    uint32 InstanceStepRate = 0;
};

class FRHIInputLayout : public FRHIResource
{
protected:
    FRHIInputLayout() = default;
    virtual ~FRHIInputLayout() = default;
    
public:
    virtual const FRHIInputElementInfo* GetInputElementInfo(uint32 Index) const = 0;
    virtual uint32 GetNumInputElementInfos() const = 0; 
};

class FRHIPipelineState : public FRHIResource
{
protected:
    FRHIPipelineState() = default;
    virtual ~FRHIPipelineState() = default;

public:
    virtual void* GetRHINativeHandle() const { return nullptr; }

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
        {
            return FMemory::Memcmp(RenderTargetFormats, Other.RenderTargetFormats, sizeof(RenderTargetFormats)) == 0;
        }
            
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
    constexpr FGraphicsPipelineShaders() noexcept = default;

    constexpr FGraphicsPipelineShaders(FRHIVertexShader* InVertexShader, FRHIHullShader* InHullShader, FRHIDomainShader* InDomainShader, 
        FRHIGeometryShader* InGeometryShader, FRHIPixelShader* InPixelShader) noexcept
        : VertexShader(InVertexShader)
        , HullShader(InHullShader)
        , DomainShader(InDomainShader)
        , GeometryShader(InGeometryShader)
        , PixelShader(InPixelShader)
    {
    }

    constexpr bool operator==(const FGraphicsPipelineShaders& Other) const noexcept = default;

    FRHIVertexShader*   VertexShader   = nullptr;
    FRHIHullShader*     HullShader     = nullptr;
    FRHIDomainShader*   DomainShader   = nullptr;
    FRHIGeometryShader* GeometryShader = nullptr;
    FRHIPixelShader*    PixelShader    = nullptr;
};

struct FViewInstancingInfo
{
    constexpr FViewInstancingInfo()
        : NumArraySlices(0)
        , StartRenderTargetArrayIndex(0)
        , bEnableViewInstancing(false)
    {
    }

    constexpr bool operator==(const FViewInstancingInfo& Other) const noexcept = default;

    uint8 NumArraySlices;
    uint8 StartRenderTargetArrayIndex : 7;
    uint8 bEnableViewInstancing : 1;
};

struct FRHIGraphicsPipelineStateInitializer
{
    FRHIGraphicsPipelineStateInitializer() noexcept = default;

    FRHIGraphicsPipelineStateInitializer(FRHIInputLayout* InVertexInputLayout, FRHIDepthStencilState* InDepthStencilState, FRHIRasterizerState* InRasterizerState,
        FRHIBlendState* InBlendState, const FGraphicsPipelineShaders& InShaderState, const FGraphicsPipelineFormats& InPipelineFormats, EPrimitiveTopology InPrimitiveTopology = EPrimitiveTopology::TriangleList,
        uint32 InSampleCount = 1, uint32 InSampleQuality = 0, uint32 InSampleMask = RHI_DEFAULT_SAMPLE_MASK, bool bInPrimitiveRestartEnable = false) noexcept
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

    bool operator==(const FRHIGraphicsPipelineStateInitializer& Other) const noexcept = default;

    FRHIInputLayout*      VertexInputLayout = nullptr;
    FRHIDepthStencilState* DepthStencilState = nullptr;
    FRHIRasterizerState*   RasterizerState   = nullptr;
    FRHIBlendState*        BlendState        = nullptr;

    uint32 SampleCount   = 1;
    uint32 SampleQuality = 0;
    uint32 SampleMask    = RHI_DEFAULT_SAMPLE_MASK;

    EPrimitiveTopology PrimitiveTopology       = EPrimitiveTopology::TriangleList;
    bool               bPrimitiveRestartEnable = false;

    FGraphicsPipelineShaders ShaderState        = { };
    FGraphicsPipelineFormats PipelineFormats    = { };
    FViewInstancingInfo      ViewInstancingInfo = { };
};

class FRHIGraphicsPipelineState : public FRHIPipelineState
{
protected:
    FRHIGraphicsPipelineState()  = default;
    ~FRHIGraphicsPipelineState() = default;
};

struct FRHIComputePipelineStateInitializer
{
    constexpr FRHIComputePipelineStateInitializer() noexcept = default;

    constexpr FRHIComputePipelineStateInitializer(FRHIComputeShader* InShader) noexcept
        : Shader(InShader)
    {
    }

    constexpr bool operator==(const FRHIComputePipelineStateInitializer& Other) const noexcept = default;

    FRHIComputeShader* Shader = nullptr;
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
    FRHIRayTracingHitGroupInfo() noexcept = default;

    FRHIRayTracingHitGroupInfo(const FString& InName, ERayTracingHitGroupType InType, TArrayView<FRHIRayTracingShader*> InRayTracingShaders) noexcept
        : Name(InName)
        , Type(InType)
        , Shaders(InRayTracingShaders)
    {
    }

    bool operator==(const FRHIRayTracingHitGroupInfo& Other) const noexcept = default;

    FString                       Name;
    ERayTracingHitGroupType       Type = ERayTracingHitGroupType::Unknown;
    TArray<FRHIRayTracingShader*> Shaders;
};

struct FRHIRayTracingPipelineStateInitializer
{
    FRHIRayTracingPipelineStateInitializer() noexcept  = default;

    FRHIRayTracingPipelineStateInitializer(const TArrayView<FRHIRayGenShader*>& InRayGenShaders, const TArrayView<FRHIRayCallableShader*>& InCallableShaders,
        const TArrayView<FRHIRayTracingHitGroupInfo>& InHitGroups, const TArrayView<FRHIRayMissShader*>& InMissShaders, uint32 InMaxAttributeSizeInBytes,
        uint32 InMaxPayloadSizeInBytes, uint32 InMaxRecursionDepth) noexcept
        : RayGenShaders(InRayGenShaders)
        , CallableShaders(InCallableShaders)
        , MissShaders(InMissShaders)
        , HitGroups(InHitGroups)
        , MaxAttributeSizeInBytes(InMaxAttributeSizeInBytes)
        , MaxPayloadSizeInBytes(InMaxPayloadSizeInBytes)
        , MaxRecursionDepth(InMaxRecursionDepth)
    {
    }

    bool operator==(const FRHIRayTracingPipelineStateInitializer& Other) const noexcept = default;

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
