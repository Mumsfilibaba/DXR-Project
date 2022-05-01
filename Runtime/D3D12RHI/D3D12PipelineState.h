#pragma once
#include "RHI/RHIResources.h"

#include "Core/Utilities/StringUtilities.h"

#include "D3D12Shader.h"
#include "D3D12Core.h"
#include "D3D12RootSignature.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12VertexInputLayout

class CD3D12VertexInputLayout : public CRHIVertexInputLayout, public CD3D12DeviceChild
{
public:

    CD3D12VertexInputLayout(CD3D12Device* InDevice, const SRHIVertexInputLayoutInitializer& CreateInfo)
        : CRHIVertexInputLayout()
        , CD3D12DeviceChild(InDevice)
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
            D3D12Element.InputSlotClass       = ConvertInputClassification(Element.InputClassification);
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
// CD3D12DepthStencilState

class CD3D12DepthStencilState : public CRHIDepthStencilState, public CD3D12DeviceChild
{
public:
    
    CD3D12DepthStencilState(CD3D12Device* InDevice, const D3D12_DEPTH_STENCIL_DESC& InDesc)
        : CRHIDepthStencilState()
        , CD3D12DeviceChild(InDevice)
        , Desc(InDesc)
    { }

    FORCEINLINE const D3D12_DEPTH_STENCIL_DESC& GetDesc() const { return Desc; }

private:
    D3D12_DEPTH_STENCIL_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RasterizerState

class CD3D12RasterizerState : public CRHIRasterizerState, public CD3D12DeviceChild
{
public:

    CD3D12RasterizerState(CD3D12Device* InDevice, const D3D12_RASTERIZER_DESC& InDesc)
        : CRHIRasterizerState()
        , CD3D12DeviceChild(InDevice)
        , Desc(InDesc)
    { }

    FORCEINLINE const D3D12_RASTERIZER_DESC& GetDesc() const { return Desc; }

private:
    D3D12_RASTERIZER_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BlendState

class CD3D12BlendState : public CRHIBlendState, public CD3D12DeviceChild
{
public:

    CD3D12BlendState(CD3D12Device* InDevice, const D3D12_BLEND_DESC& InDesc)
        : CRHIBlendState()
        , CD3D12DeviceChild(InDevice)
        , Desc(InDesc)
    { }

    FORCEINLINE const D3D12_BLEND_DESC& GetDesc() const { return Desc; }

private:
    D3D12_BLEND_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12GraphicsPipelineState

class CD3D12GraphicsPipelineState : public CRHIGraphicsPipelineState, public CD3D12DeviceChild
{
public:
    CD3D12GraphicsPipelineState(CD3D12Device* InDevice);
    ~CD3D12GraphicsPipelineState() = default;

    bool Init(const SRHIGraphicsPipelineStateInfo& CreateInfo);

    virtual void SetName(const String& InName) override final
    {
        WString WideName = CharToWide(InName);
        PipelineState->SetName(WideName.CStr());
    }

    FORCEINLINE ID3D12PipelineState* GetPipeline() const
    {
        return PipelineState.Get();
    }

    FORCEINLINE CD3D12RootSignature* GetRootSignature() const
    {
        return RootSignature.Get();
    }

private:
    TComPtr<ID3D12PipelineState>    PipelineState;
    TSharedRef<CD3D12RootSignature> RootSignature;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12ComputePipelineState

class CD3D12ComputePipelineState : public CRHIComputePipelineState, public CD3D12DeviceChild
{
public:

    CD3D12ComputePipelineState(CD3D12Device* InDevice, const TSharedRef<D3D12ComputeShader>& InShader);
    ~CD3D12ComputePipelineState() = default;

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

    FORCEINLINE CD3D12RootSignature* GetRootSignature() const
    {
        return RootSignature.Get();
    }

private:
    TComPtr<ID3D12PipelineState>       PipelineState;
    TSharedRef<D3D12ComputeShader> Shader;
    TSharedRef<CD3D12RootSignature>    RootSignature;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRayTracingShaderIdentifer

struct SRayTracingShaderIdentifer
{
    char ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RayTracingPipelineState

class CD3D12RayTracingPipelineState : public CRHIRayTracingPipelineState, public CD3D12DeviceChild
{
public:
    CD3D12RayTracingPipelineState(CD3D12Device* InDevice);
    ~CD3D12RayTracingPipelineState() = default;

    bool Init(const SRHIRayTracingPipelineStateInfo& CreateInfo);

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

    FORCEINLINE CD3D12RootSignature* GetGlobalRootSignature() const
    {
        return GlobalRootSignature.Get();
    }

    FORCEINLINE CD3D12RootSignature* GetRayGenLocalRootSignature() const
    {
        return RayGenLocalRootSignature.Get();
    }

    FORCEINLINE CD3D12RootSignature* GetMissLocalRootSignature() const
    {
        return MissLocalRootSignature.Get();
    }

    FORCEINLINE CD3D12RootSignature* GetHitLocalRootSignature() const
    {
        return HitLocalRootSignature.Get();
    }

private:
    TComPtr<ID3D12StateObject>           StateObject;
    TComPtr<ID3D12StateObjectProperties> StateObjectProperties;

    // TODO: There could be more than one root signature for locals
    TSharedRef<CD3D12RootSignature> GlobalRootSignature;
    TSharedRef<CD3D12RootSignature> RayGenLocalRootSignature;
    TSharedRef<CD3D12RootSignature> MissLocalRootSignature;
    TSharedRef<CD3D12RootSignature> HitLocalRootSignature;

    THashTable<String, SRayTracingShaderIdentifer, SStringHasher> ShaderIdentifers;
};