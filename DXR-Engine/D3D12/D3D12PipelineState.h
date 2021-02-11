#pragma once
#include "RenderLayer/Resources.h"

#include "Utilities/StringUtilities.h"

#include "D3D12Shader.h"
#include "D3D12Helpers.h"
#include "D3D12RootSignature.h"

class D3D12InputLayoutState : public InputLayoutState, public D3D12DeviceChild
{
public:
    D3D12InputLayoutState(D3D12Device* InDevice, const InputLayoutStateCreateInfo& CreateInfo)
        : InputLayoutState()
        , D3D12DeviceChild(InDevice)
        , SemanticNames()
        , ElementDesc()
        , Desc()
    {
        SemanticNames.Reserve(CreateInfo.Elements.Size());
        for (const InputElement& Element : CreateInfo.Elements)
        {
            D3D12_INPUT_ELEMENT_DESC DxElement;
            DxElement.SemanticName         = SemanticNames.EmplaceBack(Element.Semantic).c_str();
            DxElement.SemanticIndex        = Element.SemanticIndex;
            DxElement.Format               = ConvertFormat(Element.Format);
            DxElement.InputSlot            = Element.InputSlot;
            DxElement.AlignedByteOffset    = Element.ByteOffset;
            DxElement.InputSlotClass       = ConvertInputClassification(Element.InputClassification);
            DxElement.InstanceDataStepRate = Element.InstanceStepRate;
            ElementDesc.EmplaceBack(DxElement);
        }

        Desc.NumElements        = GetElementCount();
        Desc.pInputElementDescs = GetElementData();
    }

    virtual Bool IsValid() const override { return true; }

    const D3D12_INPUT_ELEMENT_DESC* GetElementData() const { return ElementDesc.Data(); }

    UInt32 GetElementCount() const { return ElementDesc.Size(); }

    const D3D12_INPUT_LAYOUT_DESC& GetDesc() const { return Desc; }

private:
    TArray<std::string> SemanticNames;
    TArray<D3D12_INPUT_ELEMENT_DESC> ElementDesc;
    D3D12_INPUT_LAYOUT_DESC Desc;
};

class D3D12DepthStencilState : public DepthStencilState, public D3D12DeviceChild
{
public:
    D3D12DepthStencilState(D3D12Device* InDevice, const D3D12_DEPTH_STENCIL_DESC& InDesc)
        : DepthStencilState()
        , D3D12DeviceChild(InDevice)
        , Desc(InDesc)
    {
    }

    virtual Bool IsValid() const override { return true; }

    const D3D12_DEPTH_STENCIL_DESC& GetDesc() const { return Desc; }

private:
    D3D12_DEPTH_STENCIL_DESC Desc;
};

class D3D12RasterizerState : public RasterizerState, public D3D12DeviceChild
{
public:
    D3D12RasterizerState(D3D12Device* InDevice, const D3D12_RASTERIZER_DESC& InDesc)
        : RasterizerState()
        , D3D12DeviceChild(InDevice)
        , Desc(InDesc)
    {
    }

    virtual Bool IsValid() const override { return true; }

    const D3D12_RASTERIZER_DESC& GetDesc() const { return Desc; }

private:
    D3D12_RASTERIZER_DESC Desc;
};

class D3D12BlendState : public BlendState, public D3D12DeviceChild
{
public:
    D3D12BlendState(D3D12Device* InDevice, const D3D12_BLEND_DESC& InDesc)
        : BlendState()
        , D3D12DeviceChild(InDevice)
        , Desc(InDesc)
    {
    }

    virtual Bool IsValid() const override { return true; }

    const D3D12_BLEND_DESC& GetDesc() const { return Desc; }

private:
    D3D12_BLEND_DESC Desc;
};

class D3D12GraphicsPipelineState : public GraphicsPipelineState, public D3D12DeviceChild
{
public:
    D3D12GraphicsPipelineState(D3D12Device* InDevice, const TRef<D3D12RootSignature>& InRootSignature);
    ~D3D12GraphicsPipelineState() = default;

    Bool Init(const GraphicsPipelineStateCreateInfo& CreateInfo);

    virtual void SetName(const std::string& InName) override final
    {
        Resource::SetName(InName);

        std::wstring WideName = ConvertToWide(InName);
        PipelineState->SetName(WideName.c_str());
    }

    virtual void* GetNativeResource() const override final { return reinterpret_cast<void*>(PipelineState.Get()); }

    virtual Bool IsValid() const override { return PipelineState != nullptr && RootSignature != nullptr; }

    ID3D12PipelineState* GetPipeline()      const { return PipelineState.Get(); }
    D3D12RootSignature*  GetRootSignature() const { return RootSignature.Get(); }

private:
    TComPtr<ID3D12PipelineState> PipelineState;
    TRef<D3D12RootSignature>     RootSignature;
};

class D3D12ComputePipelineState : public ComputePipelineState, public D3D12DeviceChild
{
    friend class D3D12RenderLayer;

public:
    D3D12ComputePipelineState(D3D12Device* InDevice, const TRef<D3D12ComputeShader>& InShader, const TRef<D3D12RootSignature>& InRootSignature);
    ~D3D12ComputePipelineState() = default;

    Bool Init();

    virtual void SetName(const std::string& InName) override final
    {
        Resource::SetName(InName);

        std::wstring WideName = ConvertToWide(InName);
        PipelineState->SetName(WideName.c_str());
    }

    virtual void* GetNativeResource() const override final { return reinterpret_cast<void*>(PipelineState.Get()); }

    virtual Bool IsValid() const override { return PipelineState != nullptr && RootSignature != nullptr; }

    ID3D12PipelineState* GetPipeline()      const { return PipelineState.Get(); }
    D3D12RootSignature*  GetRootSignature() const { return RootSignature.Get(); }

private:
    TComPtr<ID3D12PipelineState> PipelineState;
    TRef<D3D12ComputeShader>     Shader;
    TRef<D3D12RootSignature>     RootSignature;
};

class D3D12RayTracingPipelineState : public RayTracingPipelineState, public D3D12DeviceChild
{
public:
    D3D12RayTracingPipelineState(D3D12Device* InDevice);
    ~D3D12RayTracingPipelineState() = default;

    Bool Init(const RayTracingPipelineStateCreateInfo& CreateInfo, const D3D12DefaultRootSignatures& DefaultRootSignatures);

    virtual void SetName(const std::string& InName) override
    {
        Resource::SetName(InName);

        std::wstring WideName = ConvertToWide(InName);
        StateObject->SetName(WideName.c_str());
    }

    virtual void* GetNativeResource() const override final { return reinterpret_cast<void*>(StateObject.Get()); }

    virtual Bool IsValid() const { return StateObject != nullptr; }

    ID3D12StateObject*  GetStateObject()         const { return StateObject.Get(); }
    D3D12RootSignature* GetGlobalRootSignature() const { return GlobalRootSignature.Get(); }

private:
    TComPtr<ID3D12StateObject> StateObject;
    TRef<D3D12RootSignature>   GlobalRootSignature;
};