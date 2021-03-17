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

    virtual bool IsValid() const override { return true; }

    const D3D12_INPUT_ELEMENT_DESC* GetElementData() const { return ElementDesc.Data(); }

    uint32 GetElementCount() const { return ElementDesc.Size(); }

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

    virtual bool IsValid() const override { return true; }

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

    virtual bool IsValid() const override { return true; }

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

    virtual bool IsValid() const override { return true; }

    const D3D12_BLEND_DESC& GetDesc() const { return Desc; }

private:
    D3D12_BLEND_DESC Desc;
};

class D3D12GraphicsPipelineState : public GraphicsPipelineState, public D3D12DeviceChild
{
public:
    D3D12GraphicsPipelineState(D3D12Device* InDevice);
    ~D3D12GraphicsPipelineState() = default;

    bool Init(const GraphicsPipelineStateCreateInfo& CreateInfo);

    virtual void SetName(const std::string& InName) override final
    {
        Resource::SetName(InName);

        std::wstring WideName = ConvertToWide(InName);
        PipelineState->SetName(WideName.c_str());
    }

    virtual void* GetNativeResource() const override final { return reinterpret_cast<void*>(PipelineState.Get()); }

    virtual bool IsValid() const override { return PipelineState != nullptr && RootSignature != nullptr; }

    ID3D12PipelineState* GetPipeline()      const { return PipelineState.Get(); }
    D3D12RootSignature*  GetRootSignature() const { return RootSignature.Get(); }

private:
    TComPtr<ID3D12PipelineState> PipelineState;
    TRef<D3D12RootSignature>     RootSignature;
};

class D3D12ComputePipelineState : public ComputePipelineState, public D3D12DeviceChild
{
public:
    D3D12ComputePipelineState(D3D12Device* InDevice, const TRef<D3D12ComputeShader>& InShader);
    ~D3D12ComputePipelineState() = default;

    bool Init();

    virtual void SetName(const std::string& InName) override final
    {
        Resource::SetName(InName);

        std::wstring WideName = ConvertToWide(InName);
        PipelineState->SetName(WideName.c_str());
    }

    virtual void* GetNativeResource() const override final { return reinterpret_cast<void*>(PipelineState.Get()); }

    virtual bool IsValid() const override { return PipelineState != nullptr && RootSignature != nullptr; }

    ID3D12PipelineState* GetPipeline()      const { return PipelineState.Get(); }
    D3D12RootSignature*  GetRootSignature() const { return RootSignature.Get(); }

private:
    TComPtr<ID3D12PipelineState> PipelineState;
    TRef<D3D12ComputeShader>     Shader;
    TRef<D3D12RootSignature>     RootSignature;
};

struct RayTracingShaderIdentifer
{
    char ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
};

class D3D12RayTracingPipelineState : public RayTracingPipelineState, public D3D12DeviceChild
{
public:
    D3D12RayTracingPipelineState(D3D12Device* InDevice);
    ~D3D12RayTracingPipelineState() = default;

    bool Init(const RayTracingPipelineStateCreateInfo& CreateInfo);

    virtual void SetName(const std::string& InName) override
    {
        Resource::SetName(InName);

        std::wstring WideName = ConvertToWide(InName);
        StateObject->SetName(WideName.c_str());
    }

    virtual void* GetNativeResource() const override final { return reinterpret_cast<void*>(StateObject.Get()); }

    virtual bool IsValid() const { return StateObject != nullptr; }

    void* GetShaderIdentifer(const std::string& ExportName);

    ID3D12StateObject*           GetStateObject()           const { return StateObject.Get(); }
    ID3D12StateObjectProperties* GetStateObjectProperties() const { return StateObjectProperties.Get(); }
    
    D3D12RootSignature* GetGlobalRootSignature()      const { return GlobalRootSignature.Get(); }
    D3D12RootSignature* GetRayGenLocalRootSignature() const { return RayGenLocalRootSignature.Get(); }
    D3D12RootSignature* GetMissLocalRootSignature()   const { return MissLocalRootSignature.Get(); }
    D3D12RootSignature* GetHitLocalRootSignature()    const { return HitLocalRootSignature.Get(); }

private:
    TComPtr<ID3D12StateObject>           StateObject;
    TComPtr<ID3D12StateObjectProperties> StateObjectProperties;
    // TODO: There could be more than one rootdignature for locals
    TRef<D3D12RootSignature> GlobalRootSignature;
    TRef<D3D12RootSignature> RayGenLocalRootSignature;
    TRef<D3D12RootSignature> MissLocalRootSignature;
    TRef<D3D12RootSignature> HitLocalRootSignature;

    std::unordered_map<std::string, RayTracingShaderIdentifer> ShaderIdentifers;
};