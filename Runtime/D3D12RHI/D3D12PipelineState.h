#pragma once
#include "D3D12Shader.h"
#include "D3D12RootSignature.h"
#include "D3D12DeviceChild.h"
#include "D3D12RefCounted.h"
#include "RHI/RHIResources.h"
#include "Core/Utilities/StringUtilities.h"

typedef TSharedRef<class FD3D12VertexInputLayout>       FD3D12VertexInputLayoutRef;
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


class FD3D12VertexInputLayout : public FRHIVertexInputLayout
{
public:
    FD3D12VertexInputLayout(const FRHIVertexInputLayoutInitializer& Initializer);
    virtual ~FD3D12VertexInputLayout() = default;

    const D3D12_INPUT_ELEMENT_DESC* GetElementData() const
    {
        return ElementDesc.Data();
    }

    uint32 GetElementCount() const 
    {
        return ElementDesc.Size();
    }

    FORCEINLINE const D3D12_INPUT_LAYOUT_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_INPUT_LAYOUT_DESC          Desc;
    TArray<FString>                  SemanticNames;
    TArray<D3D12_INPUT_ELEMENT_DESC> ElementDesc;
};


class FD3D12DepthStencilState : public FRHIDepthStencilState
{
public:
    FD3D12DepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer);
    virtual ~FD3D12DepthStencilState() = default;

    virtual FRHIDepthStencilStateInitializer GetInitializer() const override final
    {
        return Initializer;
    }

    FORCEINLINE const D3D12_DEPTH_STENCIL_DESC& GetD3D12Desc() const
    {
        return Desc;
    }

private:
    FRHIDepthStencilStateInitializer Initializer;
    D3D12_DEPTH_STENCIL_DESC         Desc;
};


class FD3D12RasterizerState : public FRHIRasterizerState
{
public:
    FD3D12RasterizerState(const FRHIRasterizerStateInitializer& InInitializer);
    virtual ~FD3D12RasterizerState() = default;

    virtual FRHIRasterizerStateInitializer GetInitializer() const override final
    {
        return Initializer;
    }

    FORCEINLINE const D3D12_RASTERIZER_DESC& GetD3D12Desc() const
    {
        return Desc;
    }

private:
    FRHIRasterizerStateInitializer Initializer;
    D3D12_RASTERIZER_DESC          Desc;
};


class FD3D12BlendState : public FRHIBlendState
{
public:
    FD3D12BlendState(const FRHIBlendStateInitializer& InInitializer);
    virtual ~FD3D12BlendState() = default;

    virtual FRHIBlendStateInitializer GetInitializer() const override final
    {
        return Initializer;
    }

    FORCEINLINE const D3D12_BLEND_DESC& GetD3D12Desc() const
    {
        return Desc;
    }

private:
    FRHIBlendStateInitializer Initializer;
    D3D12_BLEND_DESC          Desc;
};


class FD3D12PipelineStateCommon : public FD3D12DeviceChild
{
public:
    FD3D12PipelineStateCommon(FD3D12Device* InDevice);
    virtual ~FD3D12PipelineStateCommon() = default;

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
    FString DebugName;
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
};

class FD3D12GraphicsPipelineState : public FRHIGraphicsPipelineState, public FD3D12PipelineStateCommon
{
public:
    FD3D12GraphicsPipelineState(FD3D12Device* InDevice);
    virtual ~FD3D12GraphicsPipelineState() = default;

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
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology;
    
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

class FD3D12ComputePipelineState : public FRHIComputePipelineState, public FD3D12PipelineStateCommon
{
public:
    FD3D12ComputePipelineState(FD3D12Device* InDevice, const TSharedRef<FD3D12ComputeShader>& InShader);
    virtual ~FD3D12ComputePipelineState() = default;

    bool Initialize();

    virtual void SetDebugName(const FString& InName) override final
    {
        FD3D12PipelineStateCommon::SetDebugName(InName);
    }

    FORCEINLINE FD3D12ComputeShader* GetComputeShader() const { return Shader.Get(); }

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
    virtual ~FD3D12RayTracingPipelineState() = default;

    bool Initialize(const FRHIRayTracingPipelineStateDesc& Initializer);

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
    TComPtr<ID3D12StateObject>           StateObject;
    TComPtr<ID3D12StateObjectProperties> StateObjectProperties;

    // TODO: There could be more than one root signature for locals
    FD3D12RootSignatureRef GlobalRootSignature;
    FD3D12RootSignatureRef RayGenLocalRootSignature;
    FD3D12RootSignatureRef MissLocalRootSignature;
    FD3D12RootSignatureRef HitLocalRootSignature;

    TMap<FString, FD3D12RayTracingShaderIdentifer> ShaderIdentifers;
};