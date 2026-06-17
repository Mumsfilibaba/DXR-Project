#pragma once
#include "RHI/RHIResource.h"

#define RHI_DEFAULT_STENCIl_MASK (0xffffffff)
#define RHI_DEFAULT_SAMPLE_MASK (0xffffffff)

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FRHIStaticSamplerInfo;

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

struct FRHIDepthStencilStateDesc
{
    struct FStencilState
    {
        constexpr bool operator==(const FStencilState& Other) const noexcept = default;

        EStencilOp      StencilFailOp      = EStencilOp::Keep;
        EStencilOp      StencilDepthFailOp = EStencilOp::Keep;
        EStencilOp      StencilDepthPassOp = EStencilOp::Keep;
        EComparisonFunc StencilFunc        = EComparisonFunc::Always;
    };

    constexpr bool operator==(const FRHIDepthStencilStateDesc& Other) const noexcept = default;

    EComparisonFunc DepthFunc         = EComparisonFunc::Less;
    bool            bDepthWriteEnable = true;
    bool            bDepthEnable      = true;
    uint32          StencilReadMask   = RHI_DEFAULT_STENCIl_MASK;
    uint32          StencilWriteMask  = RHI_DEFAULT_STENCIl_MASK;
    bool            bStencilEnable    = false;
    FStencilState   FrontFace         = { };
    FStencilState   BackFace          = { };
};

template<>
struct THash<FRHIDepthStencilStateDesc::FStencilState>
{
    NODISCARD static uint64 GetHash(const FRHIDepthStencilStateDesc::FStencilState& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.StencilFailOp);
        HashCombine(Result, UnderlyingTypeValue(Value.StencilDepthFailOp));
        HashCombine(Result, UnderlyingTypeValue(Value.StencilDepthPassOp));
        HashCombine(Result, UnderlyingTypeValue(Value.StencilFunc));
        return Result;
    }
};

template<>
struct THash<FRHIDepthStencilStateDesc>
{
    NODISCARD static uint64 GetHash(const FRHIDepthStencilStateDesc& Value)
    {
        uint64 Result = static_cast<uint64>(Value.bDepthWriteEnable);
        HashCombine(Result, UnderlyingTypeValue(Value.DepthFunc));
        HashCombine(Result, Value.bDepthEnable);
        HashCombine(Result, Value.StencilReadMask);
        HashCombine(Result, Value.StencilWriteMask);
        HashCombine(Result, Value.bStencilEnable);
        HashCombine(Result, THash<FRHIDepthStencilStateDesc::FStencilState>::GetHash(Value.FrontFace));
        HashCombine(Result, THash<FRHIDepthStencilStateDesc::FStencilState>::GetHash(Value.BackFace));
        return Result;
    }
};

class FRHIDepthStencilState : public FRHIResource
{
protected:
    FRHIDepthStencilState(const FRHIDepthStencilStateDesc& InDesc)
        : FRHIResource(ERHIResourceType::DepthStencilState)
        , Desc(InDesc)
    {
    }

    virtual ~FRHIDepthStencilState() = default;

public:

    /** @return D3D12: nullptr. Vulkan: nullptr. Metal: nullptr. Null: nullptr. */
    virtual void* GetRHINativeState() const = 0;

    /** @brief Returns the descriptor used to create this state. */
    NODISCARD const FRHIDepthStencilStateDesc& GetDesc() const
    {
        return Desc;
    }

protected:
    FRHIDepthStencilStateDesc Desc;
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

struct FRHIRasterizerStateDesc
{
    constexpr bool operator==(const FRHIRasterizerStateDesc& Other) const noexcept = default;

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

template<>
struct THash<FRHIRasterizerStateDesc>
{
    NODISCARD static uint64 GetHash(const FRHIRasterizerStateDesc& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.FillMode);
        HashCombine(Result, UnderlyingTypeValue(Value.CullMode));
        HashCombine(Result, Value.bFrontCounterClockwise);
        HashCombine(Result, Value.bDepthClipEnable);
        HashCombine(Result, Value.bMultisampleEnable);
        HashCombine(Result, Value.bAntialiasedLineEnable);
        HashCombine(Result, Value.bEnableConservativeRaster);
        HashCombine(Result, Value.ForcedSampleCount);
        HashCombine(Result, Value.DepthBias);
        HashCombine(Result, Value.DepthBiasClamp);
        HashCombine(Result, Value.SlopeScaledDepthBias);
        return Result;
    }
};

class FRHIRasterizerState : public FRHIResource
{
protected:
    FRHIRasterizerState(const FRHIRasterizerStateDesc& InDesc)
        : FRHIResource(ERHIResourceType::RasterizerState)
        , Desc(InDesc)
    {
    }

    virtual ~FRHIRasterizerState() = default;

public:

    /** @return D3D12: nullptr. Vulkan: nullptr. Metal: nullptr. Null: nullptr. */
    virtual void* GetRHINativeState() const = 0;

    /** @brief Returns the descriptor used to create this state. */
    NODISCARD const FRHIRasterizerStateDesc& GetDesc() const
    {
        return Desc;
    }

protected:
    FRHIRasterizerStateDesc Desc;
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

template<>
struct THash<FRenderTargetBlendInfo>
{
    NODISCARD static uint64 GetHash(const FRenderTargetBlendInfo& Value)
    {
        uint64 Result = UnderlyingTypeValue(Value.SrcBlend);
        HashCombine(Result, UnderlyingTypeValue(Value.DstBlend));
        HashCombine(Result, UnderlyingTypeValue(Value.BlendOp));
        HashCombine(Result, UnderlyingTypeValue(Value.SrcBlendAlpha));
        HashCombine(Result, UnderlyingTypeValue(Value.DstBlendAlpha));
        HashCombine(Result, UnderlyingTypeValue(Value.BlendOpAlpha));
        HashCombine(Result, Value.bBlendEnable);
        HashCombine(Result, UnderlyingTypeValue(Value.ColorWriteMask));
        return Result;
    }
};

struct FRHIBlendStateDesc
{
    constexpr bool operator==(const FRHIBlendStateDesc& Other) const noexcept = default;

    FRenderTargetBlendInfo RenderTargets[RHI_MAX_RENDER_TARGETS] = { };
    uint8                  NumRenderTargets                      = 0;
    ELogicOp               LogicOp                               = ELogicOp::NoOp;
    bool                   bLogicOpEnable                        = false;
    bool                   bAlphaToCoverageEnable                = false;
    bool                   bIndependentBlendEnable               = false;
};

template<>
struct THash<FRHIBlendStateDesc>
{
    NODISCARD static uint64 GetHash(const FRHIBlendStateDesc& Value)
    {
        uint64 Result = 0;
        for (uint32 Index = 0; Index < Value.NumRenderTargets; ++Index)
        {
            HashCombine(Result, THash<FRenderTargetBlendInfo>::GetHash(Value.RenderTargets[Index]));
        }

        HashCombine(Result, UnderlyingTypeValue(Value.LogicOp));
        HashCombine(Result, Value.bLogicOpEnable);
        HashCombine(Result, Value.bAlphaToCoverageEnable);
        HashCombine(Result, Value.bIndependentBlendEnable);
        return Result;
    }
};

class FRHIBlendState : public FRHIResource
{
protected:
    FRHIBlendState(const FRHIBlendStateDesc& InDesc)
        : FRHIResource(ERHIResourceType::BlendState)
        , Desc(InDesc)
    {
    }

    virtual ~FRHIBlendState() = default;

public:

    /** @return D3D12: nullptr. Vulkan: nullptr. Metal: nullptr. Null: nullptr. */
    virtual void* GetRHINativeState() const = 0;

    /** @brief Returns the descriptor used to create this state. */
    NODISCARD const FRHIBlendStateDesc& GetDesc() const
    {
        return Desc;
    }

protected:
    FRHIBlendStateDesc Desc;
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

struct FRHIInputElementDesc
{
    /** @brief Semantic in the shader to match */
    String Semantic;

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
    FRHIInputLayout()
        : FRHIResource(ERHIResourceType::InputLayout)
    {
    }

    virtual ~FRHIInputLayout() = default;
    
public:

    /** @return D3D12: nullptr. Vulkan: nullptr. Metal: nullptr. Null: nullptr. */
    virtual void* GetRHINativeState() const = 0;

    virtual const FRHIInputElementDesc* GetInputElementDesc(uint32 Index) const = 0;
    virtual uint32 GetNumInputElementDescs() const = 0; 
};

class FRHIPipelineState : public FRHIResource
{
protected:
    FRHIPipelineState()
        : FRHIResource(ERHIResourceType::PipelineState)
    {
    }

    virtual ~FRHIPipelineState() = default;

public:

    /** @return D3D12: ID3D12PipelineState* (graphics/compute) or ID3D12StateObject* (ray tracing). Vulkan: VkPipeline (graphics/compute) or nullptr (ray tracing). Metal: id<MTLRenderPipelineState> (graphics) or nullptr. Null: nullptr. */
    virtual void* GetRHINativeState() const = 0;

    virtual void SetDebugName(const String& InName) { }
    virtual void GetDebugName(String& OutDebugName) const { OutDebugName.Clear(); }
};

struct FRHIGraphicsPipelineFormats
{
    EFormat RenderTargetFormats[RHI_MAX_RENDER_TARGETS] = { };
    uint8   NumRenderTargets                            = 0;
    EFormat DepthStencilFormat                          = EFormat::Unknown;
};

struct FRHIViewInstancingState
{
    constexpr FRHIViewInstancingState()
        : NumArraySlices(0)
        , StartRenderTargetArrayIndex(0)
        , bEnableViewInstancing(false)
    {
    }

    constexpr bool operator==(const FRHIViewInstancingState&) const noexcept = default;

    uint8 NumArraySlices;
    uint8 StartRenderTargetArrayIndex : 7;
    uint8 bEnableViewInstancing       : 1;
};

struct FRHIMultiSampleState
{
    uint32 SampleCount   = 1;
    uint32 SampleQuality = 0;
    uint32 SampleMask    = RHI_DEFAULT_SAMPLE_MASK;
};

struct FRHIStreamOutputEntry
{
    const CHAR* SemanticName   = nullptr;
    uint32      SemanticIndex  = 0;
    uint8       StartComponent = 0;
    uint8       ComponentCount = 0;
    uint8       OutputSlot     = 0;
};

struct FRHIStreamOutputDeclaration
{
    TArrayView<const FRHIStreamOutputEntry> Entries          = { };
    TArrayView<const uint32>                BufferStrides    = { };
    uint32                                  RasterizedStream = 0;
};

struct FRHIGraphicsPipelineStateDesc
{
    FRHIVertexShader*                       VertexShader            = nullptr;
    FRHIHullShader*                         HullShader              = nullptr;
    FRHIDomainShader*                       DomainShader            = nullptr;
    FRHIGeometryShader*                     GeometryShader          = nullptr;
    FRHIPixelShader*                        PixelShader             = nullptr;
    FRHIInputLayout*                        InputLayout             = nullptr;
    FRHIDepthStencilState*                  DepthStencilState       = nullptr;
    FRHIRasterizerState*                    RasterizerState         = nullptr;
    FRHIBlendState*                         BlendState              = nullptr;
    const FRHIStreamOutputDeclaration*      StreamOutputDeclaration = nullptr;
    TArrayView<const FRHIStaticSamplerInfo> StaticSamplers          = { };
    FRHIMultiSampleState                    MultiSampleState        = { };
    FRHIGraphicsPipelineFormats             RasterizerOutputFormats = { };
    FRHIViewInstancingState                 ViewInstancingState     = { };
    EPrimitiveTopology                      PrimitiveTopology       = EPrimitiveTopology::TriangleList;
    bool                                    bPrimitiveRestartEnable = false;
};

class FRHIGraphicsPipelineState : public FRHIPipelineState
{
protected:
    FRHIGraphicsPipelineState() = default;
    virtual ~FRHIGraphicsPipelineState() = default;
};

struct FRHIComputePipelineStateDesc
{
    FRHIComputeShader*                      Shader         = nullptr;
    TArrayView<const FRHIStaticSamplerInfo> StaticSamplers = { };
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

    FRHIRayTracingHitGroupInfo(const String& InName, ERayTracingHitGroupType InType, TArrayView<FRHIRayTracingShader*> InRayTracingShaders) noexcept
        : Name(InName)
        , Type(InType)
        , Shaders(InRayTracingShaders)
    {
    }

    bool operator==(const FRHIRayTracingHitGroupInfo& Other) const noexcept = default;

    String                        Name;
    TArray<FRHIRayTracingShader*> Shaders;
    ERayTracingHitGroupType       Type = ERayTracingHitGroupType::Unknown;
};

struct FRHIRayTracingPipelineStateDesc
{
    FRHIRayTracingPipelineStateDesc() noexcept  = default;

    FRHIRayTracingPipelineStateDesc(const TArrayView<FRHIRayGenShader*>& InRayGenShaders, const TArrayView<FRHIRayCallableShader*>& InCallableShaders,
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

    bool operator==(const FRHIRayTracingPipelineStateDesc& Other) const noexcept = default;

    TArray<FRHIRayGenShader*>          RayGenShaders;
    TArray<FRHIRayCallableShader*>     CallableShaders;
    TArray<FRHIRayMissShader*>         MissShaders;
    TArray<FRHIRayTracingHitGroupInfo> HitGroups;
    uint32                             MaxAttributeSizeInBytes = 0;
    uint32                             MaxPayloadSizeInBytes   = 0;
    uint32                             MaxRecursionDepth       = 1;
};

class FRHIRayTracingPipelineState : public FRHIPipelineState
{
protected:
    FRHIRayTracingPipelineState() = default;
    virtual ~FRHIRayTracingPipelineState() = default;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
