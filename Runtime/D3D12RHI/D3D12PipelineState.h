#pragma once
#include "RHI/RHIResources.h"

#include "Core/Utilities/StringUtilities.h"

#include "D3D12Shader.h"
#include "D3D12Core.h"
#include "D3D12RootSignature.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

class FD3D12GraphicsPipelineState;
class FD3D12ComputePipelineState;
class FD3D12RayTracingPipelineState;

typedef TSharedRef<FD3D12GraphicsPipelineState>   FD3D12GraphicsPipelineStateRef;
typedef TSharedRef<FD3D12ComputePipelineState>    FD3D12ComputePipelineStateRef;
typedef TSharedRef<FD3D12RayTracingPipelineState> FD3D12RayTracingPipelineStateRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ED3D12PipelineType

enum class ED3D12PipelineType
{
    Unknown    = 0,
    Graphics   = 1,
    Compute    = 2,
    RayTracing = 3,
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12VertexInputLayout

class FD3D12VertexInputLayout : public FRHIVertexInputLayout, public FD3D12DeviceChild
{
public:

    FD3D12VertexInputLayout(FD3D12Device* InDevice, const FRHIVertexInputLayoutInitializer& Initializer)
        : FRHIVertexInputLayout()
        , FD3D12DeviceChild(InDevice)
        , SemanticNames()
        , ElementDesc()
        , Desc()
    {
        SemanticNames.Reserve(Initializer.Elements.Size());
        for (const FVertexInputElement& Element : Initializer.Elements)
        {
            D3D12_INPUT_ELEMENT_DESC D3D12Element;
            D3D12Element.SemanticName         = SemanticNames.Emplace(Element.Semantic).CStr();
            D3D12Element.SemanticIndex        = Element.SemanticIndex;
            D3D12Element.Format               = ConvertFormat(Element.Format);
            D3D12Element.InputSlot            = Element.InputSlot;
            D3D12Element.AlignedByteOffset    = Element.ByteOffset;
            D3D12Element.InputSlotClass       = ConvertVertexInputClass(Element.InputClass);
            D3D12Element.InstanceDataStepRate = (Element.InputClass == EVertexInputClass::Vertex) ? 0 : Element.InstanceStepRate;
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

    TArray<FString>                   SemanticNames;
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
// FD3D12PipelineState

class FD3D12PipelineState : public FD3D12DeviceChild
{
public:

    FD3D12PipelineState(FD3D12Device* InDevice)
        : FD3D12DeviceChild(InDevice)
    { }

    ~FD3D12PipelineState() = default;

    void SetDebugName(const FString& InName)
    {
        FWString WideName = CharToWide(InName);
        PipelineState->SetName(WideName.CStr());
    }

    FORCEINLINE ID3D12PipelineState* GetD3D12PipelineState() const { return PipelineState.Get(); }
    FORCEINLINE FD3D12RootSignature* GetRootSignature()      const { return RootSignature.Get(); }

protected:
    TComPtr<ID3D12PipelineState> PipelineState;
    FD3D12RootSignatureRef       RootSignature;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12GraphicsPipelineState

class FD3D12GraphicsPipelineState : public FRHIGraphicsPipelineState, public FD3D12PipelineState
{
public:

    FD3D12GraphicsPipelineState(FD3D12Device* InDevice);
    ~FD3D12GraphicsPipelineState() = default;

    bool Initialize(const FRHIGraphicsPipelineStateInitializer& Initializer);

public:
    
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIPipelineState

    virtual void SetName(const FString& InName) override final { FD3D12PipelineState::SetDebugName(InName); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12ComputePipelineState

class FD3D12ComputePipelineState : public FRHIComputePipelineState, public FD3D12PipelineState
{
public:

    FD3D12ComputePipelineState(FD3D12Device* InDevice, const TSharedRef<FD3D12ComputeShader>& InShader);
    ~FD3D12ComputePipelineState() = default;

    bool Initialize();

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIPipelineState

    virtual void SetName(const FString& InName) override final { FD3D12PipelineState::SetDebugName(InName); }

private:
    TSharedRef<FD3D12ComputeShader> Shader;
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

    bool Initialize(const FRHIRayTracingPipelineStateInitializer& Initializer);

    virtual void SetName(const FString& InName) override
    {
        FWString WideName = CharToWide(InName);
        StateObject->SetName(WideName.CStr());
    }

    void* GetShaderIdentifer(const FString& ExportName);

    FORCEINLINE ID3D12StateObjectProperties* GetD3D12StateObjectProperties() const { return StateObjectProperties.Get(); }
    FORCEINLINE ID3D12StateObject*           GetD3D12StateObject()           const { return StateObject.Get(); }

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

    THashTable<FString, FRayTracingShaderIdentifer, FStringHasher> ShaderIdentifers;
};