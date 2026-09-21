#pragma once
#include "RHI/RHIResources.h"
#include "RHI/RayTracing/RHIRayTracingPipelineState.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalSamplerState.h"
#include "MetalRHI/MetalShader.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

typedef TSharedRef<class FMetalInputLayoutRHI>             FMetalVertexInputLayoutRef;
typedef TSharedRef<class FMetalDepthStencilStateRHI>       FMetalDepthStencilStateRef;
typedef TSharedRef<class FMetalGraphicsPipelineStateRHI>   FMetalGraphicsPipelineStateRef;
typedef TSharedRef<class FMetalComputePipelineStateRHI>    FMetalComputePipelineStateRef;
typedef TSharedRef<class FMetalMeshletPipelineStateRHI>    FMetalMeshletPipelineStateRef;
typedef TSharedRef<class FMetalRayTracingPipelineStateRHI> FMetalRayTracingPipelineStateRef;

class FMetalInputLayoutRHI : public FRHIInputLayout
{
public:
    FMetalInputLayoutRHI(const TArray<FRHIInputElementDesc>& InInputElements);
    virtual ~FMetalInputLayoutRHI();

    // FRHIInputLayout Interface
    virtual void* GetRHINativeState() const override final;

    virtual const FRHIInputElementDesc* GetInputElementDesc(uint32 Index) const override final;
    virtual uint32 GetNumInputElementDescs() const override final;

    MTLVertexDescriptor* GetMTLVertexDescriptor() const
    {
        return VertexDescriptor;
    }

private:
    TArray<FRHIInputElementDesc> InputElements;
    MTLVertexDescriptor*         VertexDescriptor;
};

class FMetalDepthStencilStateRHI : public FRHIDepthStencilState, public FMetalDeviceChild
{
public:
    FMetalDepthStencilStateRHI(FMetalDevice* InDevice, const FRHIDepthStencilStateDesc& InDesc);
    virtual ~FMetalDepthStencilStateRHI();

    // FRHIDepthStencilState Interface
    virtual void* GetRHINativeState() const override final;

    bool Initialize();

    id<MTLDepthStencilState> GetMTLDepthStencilState() const
    {
        return DepthStencilState;
    }

private:
    id<MTLDepthStencilState> DepthStencilState;
};

class FMetalRasterizerStateRHI : public FRHIRasterizerState
{
public:
    FMetalRasterizerStateRHI(const FRHIRasterizerStateDesc& InDesc);
    virtual ~FMetalRasterizerStateRHI();

    // FRHIRasterizerState Interface
    virtual void* GetRHINativeState() const override final;

    MTLTriangleFillMode GetMTLFillMode() const
    {
        return FillMode;
    }

    MTLWinding GetMTLFrontFaceWinding() const
    {
        return FrontFaceWinding;
    }

    MTLCullMode GetMTLCullMode() const
    {
        return CullMode;
    }

private:
    MTLTriangleFillMode FillMode;
    MTLWinding          FrontFaceWinding;
    MTLCullMode         CullMode;
};

class FMetalBlendStateRHI : public FRHIBlendState
{
public:
    struct FBlendAttachment
    {
        MTLColorWriteMask WriteMask;
        BOOL              bBlendingEnabled;
        MTLBlendOperation AlphaBlendOperation;
        MTLBlendOperation ColorBlendOperation;
        MTLBlendFactor    DestinationAlphaBlendFactor;
        MTLBlendFactor    DestinationColorBlendFactor;
        MTLBlendFactor    SourceAlphaBlendFactor;
        MTLBlendFactor    SourceColorBlendFactor;
    };

public:
    FMetalBlendStateRHI(const FRHIBlendStateDesc& InDesc);
    virtual ~FMetalBlendStateRHI();

    // FRHIBlendState Interface
    virtual void* GetRHINativeState() const override final;

    const FBlendAttachment& GetColorAttachment(uint32 Index) const
    {
        return ColorAttachments[Index];
    }

    bool IsAlphaToCoverageEnabled() const
    {
        return bAlphaToCoverageEnable;
    }

    bool IsLogicOpEnabled() const
    {
        return bLogicOpEnable;
    }

private:
    FBlendAttachment ColorAttachments[RHI_MAX_RENDER_TARGETS];
    bool             bAlphaToCoverageEnable;
    bool             bLogicOpEnable;
};

struct FMetalPipelineBindingLayout
{
public:
    static constexpr uint8 InvalidSlot = UINT8_MAX;

public:
    FMetalPipelineBindingLayout();
    ~FMetalPipelineBindingLayout();

    void Reset();

    bool Collect(const TArray<FMSLShaderBinding>& ShaderBindings, EShaderVisibility::Type ShaderStage, uint16 InShaderConstantsSize);
    bool ConflictsWithVertexInputs(const FMetalInputLayoutRHI* InputLayout) const;

    uint8 GetSlot(EShaderVisibility::Type ShaderVisibility, EMSLBindingType BindingType, uint32 RegisterIndex) const;

    uint16 GetShaderConstantsSize(EShaderVisibility::Type ShaderStage) const
    {
        return ShaderConstantsSize[ShaderStage];
    }

    uint8 GetResourceHeapSlot(EShaderVisibility::Type ShaderStage) const
    {
        return ResourceHeapSlot[ShaderStage];
    }

    uint8 GetSamplerHeapSlot(EShaderVisibility::Type ShaderStage) const
    {
        return SamplerHeapSlot[ShaderStage];
    }

    bool UsesBindlessHeaps() const
    {
        for (uint32 ShaderStage = 0; ShaderStage < EShaderVisibility::Count; ++ShaderStage)
        {
            if (ResourceHeapSlot[ShaderStage] != InvalidSlot || SamplerHeapSlot[ShaderStage] != InvalidSlot)
            {
                return true;
            }
        }

        return false;
    }

private:
    TStaticArray<uint8, MAX_CONSTANT_BUFFERS> ConstantBuffers[EShaderVisibility::Count];
    TStaticArray<uint8, MAX_SRVS>             ShaderResourceBuffers[EShaderVisibility::Count];
    TStaticArray<uint8, MAX_SRVS>             ShaderResourceTextures[EShaderVisibility::Count];
    TStaticArray<uint8, MAX_UAVS>             UnorderedAccessBuffers[EShaderVisibility::Count];
    TStaticArray<uint8, MAX_UAVS>             UnorderedAccessTextures[EShaderVisibility::Count];
    TStaticArray<uint8, MAX_SAMPLER_STATES>   Samplers[EShaderVisibility::Count];
    uint8                                     ShaderConstants[EShaderVisibility::Count];
    uint8                                     ResourceHeapSlot[EShaderVisibility::Count];
    uint8                                     SamplerHeapSlot[EShaderVisibility::Count];
    uint16                                    ShaderConstantsSize[EShaderVisibility::Count];
};

struct FMetalStaticSamplerBinding
{
    /** @brief Stage whose sampler table this entry occupies. */
    EShaderVisibility::Type Stage;

    /** @brief HLSL s# register. */
    uint8 RegisterIndex;

    /** @brief Sampler created from the static-sampler desc. */
    TSharedRef<FMetalSamplerStateRHI> Sampler;
};

struct FMetalSamplerStateCache;

class FMetalGraphicsPipelineStateRHI : public FRHIGraphicsPipelineState, public FMetalDeviceChild
{
public:
    FMetalGraphicsPipelineStateRHI(FMetalDevice* InDevice, const FRHIGraphicsPipelineStateDesc& InDesc);
    virtual ~FMetalGraphicsPipelineStateRHI();

    // FRHIPipelineState Interface
    virtual void* GetRHINativeState() const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    bool Initialize();

    void ApplyStaticSamplers(FMetalSamplerStateCache& Cache) const;
    bool HasStaticSampler(EShaderVisibility::Type ShaderStage, uint32 RegisterIndex) const;

    FMetalBlendStateRHI*        GetMetalBlendState()        const { return BlendState.Get(); }
    FMetalDepthStencilStateRHI* GetMetalDepthStencilState() const { return DepthStencilState.Get(); }
    FMetalRasterizerStateRHI*   GetMetalRasterizerState()   const { return RasterizerState.Get(); }

    const FMetalPipelineBindingLayout& GetBindings() const
    {
        return Bindings;
    }

    id<MTLRenderPipelineState> GetMTLPipelineState() const
    {
        return PipelineState;
    }

    MTLPrimitiveType GetMTLPrimitiveType() const
    {
        return PrimitiveType;
    }

    const FRHIViewInstancingState& GetViewInstancingState() const
    {
        return Desc.ViewInstancingState;
    }

    bool HasDepthStencilAttachment() const
    {
        return Desc.RasterizerOutputFormats.DepthStencilFormat != EFormat::Unknown;
    }

private:
    FRHIGraphicsPipelineStateDesc          Desc;
    TSharedRef<FMetalBlendStateRHI>        BlendState;
    TSharedRef<FMetalDepthStencilStateRHI> DepthStencilState;
    TSharedRef<FMetalRasterizerStateRHI>   RasterizerState;
    id<MTLRenderPipelineState>             PipelineState;
    FMetalPipelineBindingLayout            Bindings;
    TArray<FMetalStaticSamplerBinding>     StaticSamplers;
    MTLPrimitiveType                       PrimitiveType;
    String                                 DebugName;
};

class FMetalComputePipelineStateRHI : public FRHIComputePipelineState, public FMetalDeviceChild
{
public:
    FMetalComputePipelineStateRHI(FMetalDevice* InDevice);
    virtual ~FMetalComputePipelineStateRHI();

    // FRHIPipelineState Interface
    virtual void* GetRHINativeState() const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    bool Initialize(const FRHIComputePipelineStateDesc& InDesc);

    void ApplyStaticSamplers(FMetalSamplerStateCache& Cache) const;
    bool HasStaticSampler(EShaderVisibility::Type ShaderStage, uint32 RegisterIndex) const;

    const FMetalPipelineBindingLayout& GetBindings() const
    {
        return Bindings;
    }

    id<MTLComputePipelineState> GetMTLPipelineState() const
    {
        return PipelineState;
    }

    uint32 GetMaxTotalThreadsPerThreadgroup() const
    {
        return MaxTotalThreadsPerThreadgroup;
    }

    uint16 GetThreadGroupSizeX() const { return ThreadGroupSizeX; }
    uint16 GetThreadGroupSizeY() const { return ThreadGroupSizeY; }
    uint16 GetThreadGroupSizeZ() const { return ThreadGroupSizeZ; }

private:
    id<MTLComputePipelineState>        PipelineState;
    FMetalPipelineBindingLayout        Bindings;
    TArray<FMetalStaticSamplerBinding> StaticSamplers;
    uint32                             MaxTotalThreadsPerThreadgroup;
    uint16                             ThreadGroupSizeX;
    uint16                             ThreadGroupSizeY;
    uint16                             ThreadGroupSizeZ;
    String                             DebugName;
};

class FMetalMeshletPipelineStateRHI : public FRHIMeshletPipelineState, public FMetalDeviceChild
{
public:
    FMetalMeshletPipelineStateRHI(FMetalDevice* InDevice, const FRHIMeshletPipelineStateDesc& InDesc);
    virtual ~FMetalMeshletPipelineStateRHI();

    // FRHIPipelineState Interface
    virtual void* GetRHINativeState() const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    bool Initialize();

    void ApplyStaticSamplers(FMetalSamplerStateCache& Cache) const;
    bool HasStaticSampler(EShaderVisibility::Type ShaderStage, uint32 RegisterIndex) const;

    FMetalDepthStencilStateRHI* GetMetalDepthStencilState() const { return DepthStencilState.Get(); }
    FMetalRasterizerStateRHI*   GetMetalRasterizerState()   const { return RasterizerState.Get(); }

    const FMetalPipelineBindingLayout& GetBindings() const
    {
        return Bindings;
    }

    id<MTLRenderPipelineState> GetMTLPipelineState() const
    {
        return PipelineState;
    }

    /** @return Mesh-shader threadgroup size from the MSL header. */
    MTLSize GetMeshThreadgroupSize() const
    {
        return MTLSizeMake(MeshThreadGroupSizeX, MeshThreadGroupSizeY, MeshThreadGroupSizeZ);
    }

    /** @return Object-shader threadgroup size, or {0,0,0} when there is no amplification shader. */
    MTLSize GetObjectThreadgroupSize() const
    {
        return MTLSizeMake(ObjectThreadGroupSizeX, ObjectThreadGroupSizeY, ObjectThreadGroupSizeZ);
    }

    const FRHIViewInstancingState& GetViewInstancingState() const
    {
        return Desc.ViewInstancingState;
    }

    bool HasDepthStencilAttachment() const
    {
        return Desc.RasterizerOutputFormats.DepthStencilFormat != EFormat::Unknown;
    }

private:
    FRHIMeshletPipelineStateDesc           Desc;
    TSharedRef<FMetalBlendStateRHI>        BlendState;
    TSharedRef<FMetalDepthStencilStateRHI> DepthStencilState;
    TSharedRef<FMetalRasterizerStateRHI>   RasterizerState;
    id<MTLRenderPipelineState>             PipelineState;
    FMetalPipelineBindingLayout            Bindings;
    TArray<FMetalStaticSamplerBinding>     StaticSamplers;
    uint16                                 MeshThreadGroupSizeX;
    uint16                                 MeshThreadGroupSizeY;
    uint16                                 MeshThreadGroupSizeZ;
    uint16                                 ObjectThreadGroupSizeX;
    uint16                                 ObjectThreadGroupSizeY;
    uint16                                 ObjectThreadGroupSizeZ;
    String                                 DebugName;
};

class FMetalRayTracingPipelineStateRHI : public FRHIRayTracingPipelineState, public FMetalDeviceChild
{
public:
    FMetalRayTracingPipelineStateRHI(FMetalDevice* InDevice, const FRHIRayTracingPipelineStateDesc& InDesc);
    virtual ~FMetalRayTracingPipelineStateRHI();

    // FRHIPipelineState Interface
    virtual void* GetRHINativeState() const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    // FRHIRayTracingPipelineState Interface
    virtual void GetExportName(ERayTracingShaderRecordKind Kind, uint32 RecordIndex, String& OutExportName) const override final;
    virtual uint32 GetNumExportNames(ERayTracingShaderRecordKind Kind) const override final;

    bool Initialize();

    id<MTLComputePipelineState> GetMTLPipelineState() const
    {
        return PipelineState;
    }

private:
    TArray<String>& GetExportNameArray(ERayTracingShaderRecordKind Kind);
    const TArray<String>& GetExportNameArray(ERayTracingShaderRecordKind Kind) const;

    FRHIRayTracingPipelineStateDesc Desc;
    id<MTLComputePipelineState>     PipelineState;
    TArray<String>                  RayGenerationExports;
    TArray<String>                  MissExports;
    TArray<String>                  CallableExports;
    TArray<String>                  HitGroupExports;
    String                          DebugName;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
