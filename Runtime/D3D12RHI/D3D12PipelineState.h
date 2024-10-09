#pragma once
#include "D3D12Shader.h"
#include "D3D12RootSignature.h"
#include "D3D12DeviceChild.h"
#include "D3D12RefCounted.h"
#include "RHI/RHIResources.h"
#include "Core/Utilities/StringUtilities.h"
#include "Core/Misc/CRC.h"

typedef TSharedRef<class FD3D12VertexLayout>            FD3D12VertexLayoutRef;
typedef TSharedRef<class FD3D12DepthStencilState>       FD3D12DepthStencilStateRef;
typedef TSharedRef<class FD3D12GraphicsPipelineState>   FD3D12GraphicsPipelineStateRef;
typedef TSharedRef<class FD3D12ComputePipelineState>    FD3D12ComputePipelineStateRef;
typedef TSharedRef<class FD3D12RayTracingPipelineState> FD3D12RayTracingPipelineStateRef;

enum class ED3D12PipelineType
{
    Unknown    = 0,
    Graphics   = 1,
    Compute    = 2,
    RayTracing = 3,
};

class FD3D12VertexLayout : public FRHIVertexLayout
{
public:
    FD3D12VertexLayout(const FRHIVertexLayoutInitializerList& InInitializerList);
    virtual ~FD3D12VertexLayout();

    virtual FRHIVertexLayoutInitializerList GetInitializerList() const override final
    {
        return InitializerList;
    }

    const D3D12_INPUT_LAYOUT_DESC& GetDesc() const
    {
        return Desc;
    }

    uint64 GetHash() const 
    {
        return Hash;
    }

private:
    FRHIVertexLayoutInitializerList  InitializerList;
    D3D12_INPUT_LAYOUT_DESC          Desc;
    TArray<FString>                  SemanticNames;
    TArray<D3D12_INPUT_ELEMENT_DESC> ElementDesc;
    uint64                           Hash;
};

class FD3D12DepthStencilState : public FRHIDepthStencilState
{
public:
    FD3D12DepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer);
    virtual ~FD3D12DepthStencilState();

    virtual FRHIDepthStencilStateInitializer GetInitializer() const override final
    {
        return Initializer;
    }

    const D3D12_DEPTH_STENCIL_DESC& GetD3D12Desc() const
    {
        return Desc;
    }

    uint64 GetHash() const
    {
        return Hash;
    }

private:
    FRHIDepthStencilStateInitializer Initializer;
    D3D12_DEPTH_STENCIL_DESC         Desc;
    uint64                           Hash;
};

class FD3D12RasterizerState : public FRHIRasterizerState
{
public:
    FD3D12RasterizerState(const FRHIRasterizerStateInitializer& InInitializer);
    virtual ~FD3D12RasterizerState();

    virtual FRHIRasterizerStateInitializer GetInitializer() const override final
    {
        return Initializer;
    }

    const D3D12_RASTERIZER_DESC& GetD3D12Desc() const
    {
        return Desc;
    }

    uint64 GetHash() const
    {
        return Hash;
    }

private:
    FRHIRasterizerStateInitializer Initializer;
    D3D12_RASTERIZER_DESC          Desc;
    uint64                         Hash;
};

class FD3D12BlendState : public FRHIBlendState
{
public:
    FD3D12BlendState(const FRHIBlendStateInitializer& InInitializer);
    virtual ~FD3D12BlendState();

    virtual FRHIBlendStateInitializer GetInitializer() const override final
    {
        return Initializer;
    }

    const D3D12_BLEND_DESC& GetD3D12Desc() const
    {
        return Desc;
    }

    uint64 GetHash() const
    {
        return Hash;
    }

private:
    FRHIBlendStateInitializer Initializer;
    D3D12_BLEND_DESC          Desc;
    uint64                    Hash;
};

class FD3D12PipelineStateCommon : public FD3D12DeviceChild
{
public:
    FD3D12PipelineStateCommon(FD3D12Device* InDevice);
    virtual ~FD3D12PipelineStateCommon();

    void SetDebugName(const FString& InName);

    ID3D12PipelineState* GetD3D12PipelineState() const
    {
        return PipelineState.Get();
    }

    FD3D12RootSignature* GetRootSignature() const
    {
        return RootSignature.Get();
    }

protected:
    TComPtr<ID3D12PipelineState> PipelineState;
    FD3D12RootSignatureRef       RootSignature;
    FString                      DebugName;
};

struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT) FD3D12GraphicsPipelineStream
{
    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
    {
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type0 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
        ID3D12RootSignature* RootSignature = nullptr;
    };

    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
    {
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type1 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT;
        D3D12_INPUT_LAYOUT_DESC InputLayout = { };
    };

    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
    {
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type2 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY;
        D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType = { };
    };

    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
    {
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type3 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS;
        D3D12_SHADER_BYTECODE VertexShaderCode = { };
    };

    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
    {
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type4 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS;
        D3D12_SHADER_BYTECODE HullShaderCode = { };
    };

    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
    {
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type5 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS;
        D3D12_SHADER_BYTECODE DomainShaderCode = { };
    };

    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
    {
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type6 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS;
        D3D12_SHADER_BYTECODE GeometryShaderCode = { };
    };

    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
    {
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type7 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS;
        D3D12_SHADER_BYTECODE PixelShaderCode = { };
    };

    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
    {
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type8 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS;
        D3D12_RT_FORMAT_ARRAY RenderTargetInfo = { };
    };

    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
    {
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type9 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT;
        DXGI_FORMAT DepthBufferFormat = { };
    };

    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
    {
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type10 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER;
        D3D12_RASTERIZER_DESC RasterizerDesc = { };
    };

    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
    {
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type11 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL;
        D3D12_DEPTH_STENCIL_DESC DepthStencilDesc = { };
    };

    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
    {
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type12 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND;
        D3D12_BLEND_DESC BlendStateDesc = { };
    };

    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
    {
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type13 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC;
        DXGI_SAMPLE_DESC SampleDesc = { };
    };

    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
    {
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type14 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE;
        D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IndexBufferStripCutValue = { };
    };

    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
    {
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type15 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING;
        D3D12_VIEW_INSTANCING_DESC ViewInstancingDesc = { };
    };
};

struct FD3D12HashableViewInstanceDesc
{
    FD3D12HashableViewInstanceDesc()
        : ViewInstanceCount(0)
        , Flags(D3D12_VIEW_INSTANCING_FLAG_NONE)
    {
        FMemory::Memzero(ViewInstanceLocations, sizeof(D3D12_VIEW_INSTANCE_LOCATION) * D3D12_MAX_VIEW_INSTANCE_COUNT);
    }

    uint64 GenerateHash() const
    {
        uint64 Hash = ViewInstanceCount;
        HashCombine(Hash, FCRC32::Generate(ViewInstanceLocations, sizeof(D3D12_VIEW_INSTANCE_LOCATION) * ViewInstanceCount));
        HashCombine(Hash, Flags);
        return Hash;
    }

    D3D12_VIEW_INSTANCE_LOCATION ViewInstanceLocations[D3D12_MAX_VIEW_INSTANCE_COUNT];
    uint32                       ViewInstanceCount;
    D3D12_VIEW_INSTANCING_FLAGS  Flags;
};

struct FD3D12GraphicsPipelineKey
{
    FD3D12ShaderHash                   VSHash = { };
    FD3D12ShaderHash                   HSHash = { };
    FD3D12ShaderHash                   DSHash = { };
    FD3D12ShaderHash                   GSHash = { };
    FD3D12ShaderHash                   PSHash = { };
    uint64                             RootSignatureHash = 0;
    uint64                             InputLayoutHash = 0;
    uint64                             BlendStateHash = 0;
    uint64                             DepthStencilHash = 0;
    uint64                             RasterizerHash = 0;
    uint64                             ViewInstancingHash = 0;
    D3D12_PRIMITIVE_TOPOLOGY_TYPE      PrimitiveTopologyType = { };
    D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IndexBufferStripCutValue = { };
    DXGI_FORMAT                        DepthBufferFormat = { };
    D3D12_RT_FORMAT_ARRAY              RenderTargetInfo = { };
    DXGI_SAMPLE_DESC                   SampleDesc = { };
};

class FD3D12GraphicsPipelineState : public FRHIGraphicsPipelineState, public FD3D12PipelineStateCommon
{
public:
    FD3D12GraphicsPipelineState(FD3D12Device* InDevice);
    virtual ~FD3D12GraphicsPipelineState();

    bool Initialize(const FRHIGraphicsPipelineStateInitializer& Initializer);
    
    virtual void SetDebugName(const FString& InName) override final
    {
        FD3D12PipelineStateCommon::SetDebugName(InName);
    }

    D3D12_PRIMITIVE_TOPOLOGY GetD3D12PrimitiveTopology() const
    {
        return PrimitiveTopology;
    }

    FORCEINLINE FD3D12VertexShader*   GetVertexShader()   const { return VertexShader.Get(); }
    FORCEINLINE FD3D12HullShader*     GetHullShader()     const { return HullShader.Get(); }
    FORCEINLINE FD3D12DomainShader*   GetDomainShader()   const { return DomainShader.Get(); }
    FORCEINLINE FD3D12GeometryShader* GetGeometryShader() const { return GeometryShader.Get(); }
    FORCEINLINE FD3D12PixelShader*    GetPixelShader()    const { return PixelShader.Get(); }

private:
    D3D12_PRIMITIVE_TOPOLOGY         PrimitiveTopology;
    TSharedRef<FD3D12VertexShader>   VertexShader;
    TSharedRef<FD3D12HullShader>     HullShader;
    TSharedRef<FD3D12DomainShader>   DomainShader;
    TSharedRef<FD3D12GeometryShader> GeometryShader;
    TSharedRef<FD3D12PixelShader>    PixelShader;
};

struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT) FD3D12ComputePipelineStream
{
    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
    {
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type0 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
        ID3D12RootSignature* RootSignature = nullptr;
    };

    struct alignas(D3D12_PIPELINE_STATE_STREAM_ALIGNMENT)
    {
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type1 = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS;
        D3D12_SHADER_BYTECODE ComputeShader = { };
    };
};

struct FD3D12ComputePipelineKey
{
    uint64           RootSignatureHash = 0;
    FD3D12ShaderHash CSHash = { 0, 0 };
};

class FD3D12ComputePipelineState : public FRHIComputePipelineState, public FD3D12PipelineStateCommon
{
public:
    FD3D12ComputePipelineState(FD3D12Device* InDevice, const TSharedRef<FD3D12ComputeShader>& InShader);
    virtual ~FD3D12ComputePipelineState();

    bool Initialize();

    virtual void SetDebugName(const FString& InName) override final
    {
        FD3D12PipelineStateCommon::SetDebugName(InName);
    }

    FORCEINLINE FD3D12ComputeShader* GetComputeShader() const
    {
        return Shader.Get();
    }

private:
    TSharedRef<FD3D12ComputeShader> Shader;
};

struct FD3D12RayTracingShaderIdentifer
{
    CHAR ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
};

class FD3D12RayTracingPipelineState : public FRHIRayTracingPipelineState, public FD3D12DeviceChild
{
public:
    FD3D12RayTracingPipelineState(FD3D12Device* InDevice);
    virtual ~FD3D12RayTracingPipelineState();

    bool Initialize(const FRHIRayTracingPipelineStateInitializer& Initializer);

    virtual void SetDebugName(const FString& InName) override
    {
        FStringWide WideName = CharToWide(InName);
        StateObject->SetName(WideName.GetCString());
    }

    void* GetShaderIdentifer(const FString& ExportName);

    FORCEINLINE ID3D12StateObject* GetD3D12StateObject() const 
    {
        return StateObject.Get();
    }

    FORCEINLINE ID3D12StateObjectProperties* GetD3D12StateObjectProperties() const
    {
        return StateObjectProperties.Get();
    }

    FORCEINLINE FD3D12RootSignature* GetGlobalRootSignature()      const { return GlobalRootSignature.Get(); }
    FORCEINLINE FD3D12RootSignature* GetRayGenLocalRootSignature() const { return RayGenLocalRootSignature.Get(); }
    FORCEINLINE FD3D12RootSignature* GetMissLocalRootSignature()   const { return MissLocalRootSignature.Get(); }
    FORCEINLINE FD3D12RootSignature* GetHitLocalRootSignature()    const { return HitLocalRootSignature.Get(); }

private:
    TComPtr<ID3D12StateObject>                     StateObject;
    TComPtr<ID3D12StateObjectProperties>           StateObjectProperties;
    FD3D12RootSignatureRef                         GlobalRootSignature;
    // TODO: There could be more than one root signature for locals
    FD3D12RootSignatureRef                         RayGenLocalRootSignature;
    FD3D12RootSignatureRef                         MissLocalRootSignature;
    FD3D12RootSignatureRef                         HitLocalRootSignature;
    TMap<FString, FD3D12RayTracingShaderIdentifer> ShaderIdentifers;
};

struct FD3D12PipelineDiskHeader
{
    CHAR   Magic[8]; // Always "D3D12PSO"
    uint64 DataCRC;
    uint64 DataSize;
};

class FD3D12PipelineStateManager : public FD3D12DeviceChild
{
public:
    FD3D12PipelineStateManager(FD3D12Device* InDevice);
    ~FD3D12PipelineStateManager();

    bool Initialize();
    bool CreateGraphicsPipeline(const WIDECHAR* PipelineHash, const D3D12_PIPELINE_STATE_STREAM_DESC& PipelineStream, TComPtr<ID3D12PipelineState>& OutPipelineState);
    bool CreateComputePipeline(const WIDECHAR* PipelineHash, const D3D12_PIPELINE_STATE_STREAM_DESC& PipelineStream, TComPtr<ID3D12PipelineState>& OutPipelineState);
    bool SaveCacheData();
    
    ID3D12PipelineLibrary1* GetD3D12PipelineLibrary() const
    {
        return PipelineLibrary.Get();
    }

private:
    bool LoadCacheFromFile();
    void FreePipelineData();
    
    void*                           PipelineData;
    uint64                          PipelineDataSize;
    TComPtr<ID3D12PipelineLibrary1> PipelineLibrary;
    FCriticalSection                PipelineLibraryCS;
    bool                            bPipelineLibraryDirty;
};