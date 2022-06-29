#pragma once
#include "RHI/RHIResources.h"

#include "Core/Utilities/StringUtilities.h"

#include "D3D12Shader.h"
#include "D3D12Core.h"
#include "D3D12RootSignature.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12VertexInputLayout

class FD3D12VertexInputLayout : public FRHIVertexInputLayout, public FD3D12DeviceChild
{
public:

    FD3D12VertexInputLayout(FD3D12Device* InDevice, const FRHIVertexInputLayoutInitializer& CreateInfo)
        : FRHIVertexInputLayout()
        , FD3D12DeviceChild(InDevice)
        , SemanticNames()
        , ElementDesc()
        , Desc()
    {
        SemanticNames.Reserve(CreateInfo.Elements.Size());
        for (const SVertexInputElement& Element : CreateInfo.Elements)
        {
            D3D12_INPUT_ELEMENT_DESC D3D12Element;
            D3D12Element.SemanticName         = SemanticNames.Emplace(Element.Semantic).CStr();
            D3D12Element.SemanticIndex        = Element.SemanticIndex;
            D3D12Element.Format               = ConvertFormat(Element.Format);
            D3D12Element.InputSlot            = Element.InputSlot;
            D3D12Element.AlignedByteOffset    = Element.ByteOffset;
            D3D12Element.InputSlotClass       = ConvertVertexInputClass(Element.InputClass);
            D3D12Element.InstanceDataStepRate = Element.InstanceStepRate;
            ElementDesc.Emplace(D3D12Element);
        }

        Desc.NumElements        = GetElementCount();
        Desc.pInputElementDescs = GetElementData();
    }

    const D3D12_INPUT_ELEMENT_DESC* GetElementData() const { return ElementDesc.Data(); }

    uint32 GetElementCount() const { return ElementDesc.Size(); }

    FORCEINLINE const D3D12_INPUT_LAYOUT_DESC& GetDesc() const { return Desc; }

private:
    D3D12_INPUT_LAYOUT_DESC Desc;

    TArray<String>                   SemanticNames;
    TArray<D3D12_INPUT_ELEMENT_DESC> ElementDesc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12DepthStencilState

class FD3D12DepthStencilState : public FRHIDepthStencilState, public FD3D12DeviceChild
{
public:
    
    FD3D12DepthStencilState(FD3D12Device* InDevice, const D3D12_DEPTH_STENCIL_DESC& InDesc)
        : FRHIDepthStencilState()
        , FD3D12DeviceChild(InDevice)
        , Desc(InDesc)
    { }

    FORCEINLINE const D3D12_DEPTH_STENCIL_DESC& GetDesc() const { return Desc; }

private:
    D3D12_DEPTH_STENCIL_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12RasterizerState

class FD3D12RasterizerState : public FRHIRasterizerState, public FD3D12DeviceChild
{
public:

    FD3D12RasterizerState(FD3D12Device* InDevice, const D3D12_RASTERIZER_DESC& InDesc)
        : FRHIRasterizerState()
        , FD3D12DeviceChild(InDevice)
        , Desc(InDesc)
    { }

    FORCEINLINE const D3D12_RASTERIZER_DESC& GetDesc() const { return Desc; }

private:
    D3D12_RASTERIZER_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12BlendState

class FD3D12BlendState : public FRHIBlendState, public FD3D12DeviceChild
{
public:

    FD3D12BlendState(FD3D12Device* InDevice, const D3D12_BLEND_DESC& InDesc)
        : FRHIBlendState()
        , FD3D12DeviceChild(InDevice)
        , Desc(InDesc)
    { }

    FORCEINLINE const D3D12_BLEND_DESC& GetDesc() const { return Desc; }

private:
    D3D12_BLEND_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12GraphicsPipelineState

class FD3D12GraphicsPipelineState : public FRHIGraphicsPipelineState, public FD3D12DeviceChild
{
public:
    FD3D12GraphicsPipelineState(FD3D12Device* InDevice);
    ~FD3D12GraphicsPipelineState() = default;

    bool Init(const FRHIGraphicsPipelineStateInitializer& CreateInfo);

    virtual void SetName(const String& InName) override final
    {
        WString WideName = CharToWide(InName);
        PipelineState->SetName(WideName.CStr());
    }

    FORCEINLINE ID3D12PipelineState* GetPipeline() const
    {
        return PipelineState.Get();
    }

    FORCEINLINE FD3D12RootSignature* GetRootSignature() const
    {
        return RootSignature.Get();
    }

private:
    TComPtr<ID3D12PipelineState>    PipelineState;
    TSharedRef<FD3D12RootSignature> RootSignature;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12ComputePipelineState

class FD3D12ComputePipelineState : public FRHIComputePipelineState, public FD3D12DeviceChild
{
public:

    FD3D12ComputePipelineState(FD3D12Device* InDevice, const TSharedRef<FD3D12ComputeShader>& InShader);
    ~FD3D12ComputePipelineState() = default;

    bool Init();

    virtual void SetName(const String& InName) override final
    {
        WString WideName = CharToWide(InName);
        PipelineState->SetName(WideName.CStr());
    }

    FORCEINLINE ID3D12PipelineState* GetPipeline() const
    {
        return PipelineState.Get();
    }

    FORCEINLINE FD3D12RootSignature* GetRootSignature() const
    {
        return RootSignature.Get();
    }

private:
    TComPtr<ID3D12PipelineState>    PipelineState;
    TSharedRef<FD3D12ComputeShader> Shader;
    TSharedRef<FD3D12RootSignature> RootSignature;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRayTracingShaderIdentifer

struct FRayTracingShaderIdentifer
{
    char ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12RayTracingPipelineState

class FD3D12RayTracingPipelineState : public FRHIRayTracingPipelineState, public FD3D12DeviceChild
{
public:
    FD3D12RayTracingPipelineState(FD3D12Device* InDevice);
    ~FD3D12RayTracingPipelineState() = default;

    bool Init(const FRHIRayTracingPipelineStateInitializer& CreateInfo);

    virtual void SetName(const String& InName) override
    {
        WString WideName = CharToWide(InName);
        StateObject->SetName(WideName.CStr());
    }

    void* GetShaderIdentifer(const String& ExportName);

    FORCEINLINE ID3D12StateObject* GetStateObject() const
    {
        return StateObject.Get();
    }

    FORCEINLINE ID3D12StateObjectProperties* GetStateObjectProperties() const
    {
        return StateObjectProperties.Get();
    }

    FORCEINLINE FD3D12RootSignature* GetGlobalRootSignature() const
    {
        return GlobalRootSignature.Get();
    }

    FORCEINLINE FD3D12RootSignature* GetRayGenLocalRootSignature() const
    {
        return RayGenLocalRootSignature.Get();
    }

    FORCEINLINE FD3D12RootSignature* GetMissLocalRootSignature() const
    {
        return MissLocalRootSignature.Get();
    }

    FORCEINLINE FD3D12RootSignature* GetHitLocalRootSignature() const
    {
        return HitLocalRootSignature.Get();
    }

private:
    TComPtr<ID3D12StateObject>           StateObject;
    TComPtr<ID3D12StateObjectProperties> StateObjectProperties;

    // TODO: There could be more than one root signature for locals
    TSharedRef<FD3D12RootSignature> GlobalRootSignature;
    TSharedRef<FD3D12RootSignature> RayGenLocalRootSignature;
    TSharedRef<FD3D12RootSignature> MissLocalRootSignature;
    TSharedRef<FD3D12RootSignature> HitLocalRootSignature;

    THashTable<String, FRayTracingShaderIdentifer, SStringHasher> ShaderIdentifers;
};