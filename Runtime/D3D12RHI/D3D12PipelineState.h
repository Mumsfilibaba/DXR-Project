#pragma once
#include "RHI/RHIResources.h"

#include "Core/Utilities/StringUtilities.h"

#include "D3D12Shader.h"
#include "D3D12Core.h"
#include "D3D12RootSignature.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12InputLayoutState

class CD3D12InputLayoutState : public CRHIInputLayoutState, public CD3D12DeviceObject
{
public:
    CD3D12InputLayoutState(CD3D12Device* InDevice, const SRHIInputLayoutStateDesc& CreateInfo)
        : CRHIInputLayoutState()
        , CD3D12DeviceObject(InDevice)
        , SemanticNames()
        , ElementDesc()
        , Desc()
    {
        SemanticNames.Reserve(CreateInfo.Elements.Size());

        for (const SInputElement& Element : CreateInfo.Elements)
        {
            D3D12_INPUT_ELEMENT_DESC DxElement;
            DxElement.SemanticName         = SemanticNames.Emplace(Element.Semantic).CStr();
            DxElement.SemanticIndex        = Element.SemanticIndex;
            DxElement.Format               = ConvertFormat(Element.Format);
            DxElement.InputSlot            = Element.InputSlot;
            DxElement.AlignedByteOffset    = Element.ByteOffset;
            DxElement.InputSlotClass       = ConvertInputClassification(Element.InputClassification);
            DxElement.InstanceDataStepRate = Element.InstanceStepRate;
            ElementDesc.Emplace(DxElement);
        }

        Desc.NumElements        = GetElementCount();
        Desc.pInputElementDescs = GetElementData();
    }

    virtual bool IsValid() const override
    {
        return true;
    }

    FORCEINLINE const D3D12_INPUT_ELEMENT_DESC* GetElementData() const
    {
        return ElementDesc.Data();
    }

    FORCEINLINE uint32 GetElementCount() const
    {
        return ElementDesc.Size();
    }

    FORCEINLINE const D3D12_INPUT_LAYOUT_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_INPUT_LAYOUT_DESC Desc;

    TArray<String>                   SemanticNames;
    TArray<D3D12_INPUT_ELEMENT_DESC> ElementDesc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12DepthStencilState

class CD3D12DepthStencilState : public CRHIDepthStencilState, public CD3D12DeviceObject
{
public:
    CD3D12DepthStencilState(CD3D12Device* InDevice, const D3D12_DEPTH_STENCIL_DESC& InDesc)
        : CRHIDepthStencilState()
        , CD3D12DeviceObject(InDevice)
        , Desc(InDesc)
    { }

    virtual bool IsValid() const override
    {
        return true;
    }

    FORCEINLINE const D3D12_DEPTH_STENCIL_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_DEPTH_STENCIL_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RasterizerState

class CD3D12RasterizerState : public CRHIRasterizerState, public CD3D12DeviceObject
{
public:
    CD3D12RasterizerState(CD3D12Device* InDevice, const D3D12_RASTERIZER_DESC& InDesc)
        : CRHIRasterizerState()
        , CD3D12DeviceObject(InDevice)
        , Desc(InDesc)
    { }

    virtual bool IsValid() const override
    {
        return true;
    }

    FORCEINLINE const D3D12_RASTERIZER_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_RASTERIZER_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12BlendState

class CD3D12BlendState : public CRHIBlendState, public CD3D12DeviceObject
{
public:
    CD3D12BlendState(CD3D12Device* InDevice, const D3D12_BLEND_DESC& InDesc)
        : CRHIBlendState()
        , CD3D12DeviceObject(InDevice)
        , Desc(InDesc)
    { }

    virtual bool IsValid() const override
    {
        return true;
    }

    FORCEINLINE const D3D12_BLEND_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    D3D12_BLEND_DESC Desc;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12GraphicsPipelineState

class CD3D12GraphicsPipelineState : public CRHIGraphicsPipelineState, public CD3D12DeviceObject
{
public:
    CD3D12GraphicsPipelineState(CD3D12Device* InDevice);
    ~CD3D12GraphicsPipelineState() = default;

    bool Initialize(const SRHIGraphicsPipelineStateDesc& CreateInfo);

    virtual void SetName(const String& InName) override final
    {
        CRHIObject::SetName(InName);
        PipelineState->SetPrivateData(WKPDID_D3DDebugObjectName, InName.Length(), InName.CStr());
    }

    virtual void* GetNativeResource() const override final
    {
        return reinterpret_cast<void*>(PipelineState.Get());
    }

    virtual bool IsValid() const override
    {
        return PipelineState != nullptr && RootSignature != nullptr;
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

class CD3D12ComputePipelineState : public CRHIComputePipelineState, public CD3D12DeviceObject
{
public:

    CD3D12ComputePipelineState(CD3D12Device* InDevice, const TSharedRef<CD3D12ComputeShader>& InShader);
    ~CD3D12ComputePipelineState() = default;

    bool Initialize();

    virtual void SetName(const String& InName) override final
    {
        CRHIObject::SetName(InName);
        PipelineState->SetPrivateData(WKPDID_D3DDebugObjectName, InName.Length(), InName.CStr());
    }

    virtual void* GetNativeResource() const override final
    {
        return reinterpret_cast<void*>(PipelineState.Get());
    }

    virtual bool IsValid() const override
    {
        return PipelineState != nullptr && RootSignature != nullptr;
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
    TSharedRef<CD3D12ComputeShader> Shader;
    TSharedRef<CD3D12RootSignature> RootSignature;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// RayTracingShaderIdentifer

struct SD3D12RayTracingShaderIdentifer
{
    char ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RayTracingPipelineState

class CD3D12RayTracingPipelineState : public CRHIRayTracingPipelineState, public CD3D12DeviceObject
{
public:
    CD3D12RayTracingPipelineState(CD3D12Device* InDevice);
    ~CD3D12RayTracingPipelineState() = default;

    bool Initialize(const SRHIRayTracingPipelineStateDesc& CreateInfo);

    virtual void SetName(const String& InName) override
    {
        CRHIObject::SetName(InName);
        StateObject->SetPrivateData(WKPDID_D3DDebugObjectName, InName.Length(), InName.CStr());
    }

    virtual void* GetNativeResource() const override final
    {
        return reinterpret_cast<void*>(StateObject.Get());
    }

    virtual bool IsValid() const
    {
        return StateObject != nullptr;
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

    THashTable<String, SD3D12RayTracingShaderIdentifer, SStringHasher> ShaderIdentifers;
};