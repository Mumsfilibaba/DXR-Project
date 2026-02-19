#pragma once
#include "Core/Misc/CRC.h"
#include "RHI/RHIResources.h"
#include "D3D12RHI/D3D12Shader.h"
#include "D3D12RHI/D3D12RootSignature.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12RefCounted.h"

typedef TSharedRef<class FD3D12InputLayoutRHI>             FD3D12InputLayoutRHIRef;
typedef TSharedRef<class FD3D12DepthStencilStateRHI>       FD3D12DepthStencilStateRHIRef;
typedef TSharedRef<class FD3D12GraphicsPipelineStateRHI>   FD3D12GraphicsPipelineStateRHIRef;
typedef TSharedRef<class FD3D12ComputePipelineStateRHI>    FD3D12ComputePipelineStateRHIRef;
typedef TSharedRef<class FD3D12RayTracingPipelineStateRHI> FD3D12RayTracingPipelineStateRHIRef;

enum class ED3D12PipelineType
{
    Unknown    = 0,
    Graphics   = 1,
    Compute    = 2,
    RayTracing = 3,
};

class FD3D12InputLayoutRHI : public FRHIInputLayout
{
public:
    FD3D12InputLayoutRHI(const TArray<FRHIInputElementDesc>& InInputElements);
    virtual ~FD3D12InputLayoutRHI();

    virtual const FRHIInputElementDesc* GetInputElementDesc(uint32 Index) const override final
    {
        return &InputElements[Index];
    }

    virtual uint32 GetNumInputElementDescs() const override final
    {
        return InputElements.Size();
    }

    const D3D12_INPUT_LAYOUT_DESC& GetDesc() const
    {
        return D3D12Desc;
    }

    uint64 GetHash() const 
    {
        return Hash;
    }

private:
    TArray<FRHIInputElementDesc>     InputElements;
    D3D12_INPUT_LAYOUT_DESC          D3D12Desc;
    TArray<FString>                  SemanticNames;
    TArray<D3D12_INPUT_ELEMENT_DESC> ElementDesc;
    uint64                           Hash;
};

class FD3D12DepthStencilStateRHI : public FRHIDepthStencilState
{
public:
    FD3D12DepthStencilStateRHI(const FRHIDepthStencilStateDesc& InDesc);
    virtual ~FD3D12DepthStencilStateRHI();

    virtual FRHIDepthStencilStateDesc GetDesc() const override final
    {
        return Desc;
    }

    const D3D12_DEPTH_STENCIL_DESC& GetD3D12Desc() const
    {
        return D3D12Desc;
    }

    uint64 GetHash() const
    {
        return Hash;
    }

private:
    FRHIDepthStencilStateDesc Desc;
    D3D12_DEPTH_STENCIL_DESC  D3D12Desc;
    uint64                    Hash;
};

class FD3D12RasterizerStateRHI : public FRHIRasterizerState
{
public:
    FD3D12RasterizerStateRHI(const FRHIRasterizerStateDesc& InDesc);
    virtual ~FD3D12RasterizerStateRHI();

    virtual FRHIRasterizerStateDesc GetDesc() const override final
    {
        return Desc;
    }

    const D3D12_RASTERIZER_DESC& GetD3D12Desc() const
    {
        return D3D12Desc;
    }

    uint64 GetHash() const
    {
        return Hash;
    }

private:
    FRHIRasterizerStateDesc Desc;
    D3D12_RASTERIZER_DESC   D3D12Desc;
    uint64                  Hash;
};

class FD3D12BlendStateRHI : public FRHIBlendState
{
public:
    FD3D12BlendStateRHI(const FRHIBlendStateDesc& InDesc);
    virtual ~FD3D12BlendStateRHI();

    virtual FRHIBlendStateDesc GetDesc() const override final
    {
        return Desc;
    }

    const D3D12_BLEND_DESC& GetD3D12Desc() const
    {
        return D3D12Desc;
    }

    uint64 GetHash() const
    {
        return Hash;
    }

private:
    FRHIBlendStateDesc Desc;
    D3D12_BLEND_DESC   D3D12Desc;
    uint64             Hash;
};

class FD3D12PipelineState : public FD3D12DeviceChild
{
public:
    FD3D12PipelineState(FD3D12Device* InDevice);
    virtual ~FD3D12PipelineState();

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
        D3D12_RT_FORMAT_ARRAY RenderTargetFormatArray = { };
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
        HashCombine(Hash, CRC32::Generate(ViewInstanceLocations, sizeof(D3D12_VIEW_INSTANCE_LOCATION) * ViewInstanceCount));
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
    D3D12_RT_FORMAT_ARRAY              RenderTargetFormatArray = { };
    DXGI_SAMPLE_DESC                   SampleDesc = { };
};

class FD3D12GraphicsPipelineStateRHI : public FRHIGraphicsPipelineState, public FD3D12PipelineState
{
public:
    FD3D12GraphicsPipelineStateRHI(FD3D12Device* InDevice);
    virtual ~FD3D12GraphicsPipelineStateRHI();

    bool Initialize(const FRHIGraphicsPipelineStateDesc& Desc);

    // FRHIPipelineState Interface
    virtual void* GetRHINativeHandle() const override final { return reinterpret_cast<void*>(GetD3D12PipelineState()); }

    virtual void SetDebugName(const FString& InName) override final
    {
        FD3D12PipelineState::SetDebugName(InName);
    }

    D3D12_PRIMITIVE_TOPOLOGY GetD3D12PrimitiveTopology() const
    {
        return PrimitiveTopology;
    }

    FORCEINLINE FD3D12VertexShaderRHI*   GetVertexShader()   const { return VertexShader.Get(); }
    FORCEINLINE FD3D12HullShaderRHI*     GetHullShader()     const { return HullShader.Get(); }
    FORCEINLINE FD3D12DomainShaderRHI*   GetDomainShader()   const { return DomainShader.Get(); }
    FORCEINLINE FD3D12GeometryShaderRHI* GetGeometryShader() const { return GeometryShader.Get(); }
    FORCEINLINE FD3D12PixelShaderRHI*    GetPixelShader()    const { return PixelShader.Get(); }

private:
    D3D12_PRIMITIVE_TOPOLOGY         PrimitiveTopology;
    TSharedRef<FD3D12VertexShaderRHI>   VertexShader;
    TSharedRef<FD3D12HullShaderRHI>     HullShader;
    TSharedRef<FD3D12DomainShaderRHI>   DomainShader;
    TSharedRef<FD3D12GeometryShaderRHI> GeometryShader;
    TSharedRef<FD3D12PixelShaderRHI>    PixelShader;
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

class FD3D12ComputePipelineStateRHI : public FRHIComputePipelineState, public FD3D12PipelineState
{
public:
    FD3D12ComputePipelineStateRHI(FD3D12Device* InDevice, const TSharedRef<FD3D12ComputeShaderRHI>& InShader);
    virtual ~FD3D12ComputePipelineStateRHI();

    bool Initialize();

    // FRHIPipelineState Interface
    virtual void* GetRHINativeHandle() const override final { return reinterpret_cast<void*>(GetD3D12PipelineState()); }

    virtual void SetDebugName(const FString& InName) override final
    {
        FD3D12PipelineState::SetDebugName(InName);
    }

    FORCEINLINE FD3D12ComputeShaderRHI* GetComputeShader() const
    {
        return Shader.Get();
    }

private:
    TSharedRef<FD3D12ComputeShaderRHI> Shader;
};

struct FD3D12RayTracingShaderIdentifier
{
    CHAR ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
};

class FD3D12RayTracingPipelineStateRHI : public FRHIRayTracingPipelineState, public FD3D12DeviceChild
{
public:
    FD3D12RayTracingPipelineStateRHI(FD3D12Device* InDevice);
    virtual ~FD3D12RayTracingPipelineStateRHI();

    bool Initialize(const FRHIRayTracingPipelineStateDesc& Initializer);

    // FRHIPipelineState Interface
    virtual void* GetRHINativeHandle() const override final { return reinterpret_cast<void*>(GetD3D12StateObject()); }

    virtual void SetDebugName(const FString& InName) override final
    {
        FStringWide WideName = CharToWide(InName);
        StateObject->SetName(*WideName);
    }

    void* GetShaderIdentifier(const FString& ExportName);

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
    TComPtr<ID3D12StateObject>                      StateObject;
    TComPtr<ID3D12StateObjectProperties>            StateObjectProperties;
    FD3D12RootSignatureRef                          GlobalRootSignature;
    // TODO: There could be more than one root signature for locals
    FD3D12RootSignatureRef                          RayGenLocalRootSignature;
    FD3D12RootSignatureRef                          MissLocalRootSignature;
    FD3D12RootSignatureRef                          HitLocalRootSignature;
    TMap<FString, FD3D12RayTracingShaderIdentifier> ShaderIdentifiers;
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
