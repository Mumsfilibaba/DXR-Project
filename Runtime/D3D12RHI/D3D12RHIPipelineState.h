#pragma once
#include "RHI/RHIResources.h"

#include "Core/Utilities/StringUtilities.h"

#include "D3D12Shader.h"
#include "D3D12Core.h"
#include "D3D12RootSignature.h"

class CD3D12RHIInputLayoutState : public CRHIInputLayoutState, public CD3D12DeviceChild
{
public:
    CD3D12RHIInputLayoutState( CD3D12Device* InDevice, const SInputLayoutStateCreateInfo& CreateInfo )
        : CRHIInputLayoutState()
        , CD3D12DeviceChild( InDevice )
        , SemanticNames()
        , ElementDesc()
        , Desc()
    {
        SemanticNames.Reserve( CreateInfo.Elements.Size() );
        for ( const SInputElement& Element : CreateInfo.Elements )
        {
            D3D12_INPUT_ELEMENT_DESC DxElement;
            DxElement.SemanticName = SemanticNames.Emplace( Element.Semantic ).CStr();
            DxElement.SemanticIndex = Element.SemanticIndex;
            DxElement.Format = ConvertFormat( Element.Format );
            DxElement.InputSlot = Element.InputSlot;
            DxElement.AlignedByteOffset = Element.ByteOffset;
            DxElement.InputSlotClass = ConvertInputClassification( Element.InputClassification );
            DxElement.InstanceDataStepRate = Element.InstanceStepRate;
            ElementDesc.Emplace( DxElement );
        }

        Desc.NumElements = GetElementCount();
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

    TArray<CString> SemanticNames;
    TArray<D3D12_INPUT_ELEMENT_DESC> ElementDesc;
};

class CD3D12RHIDepthStencilState : public CRHIDepthStencilState, public CD3D12DeviceChild
{
public:
    CD3D12RHIDepthStencilState( CD3D12Device* InDevice, const D3D12_DEPTH_STENCIL_DESC& InDesc )
        : CRHIDepthStencilState()
        , CD3D12DeviceChild( InDevice )
        , Desc( InDesc )
    {
    }

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

class CD3D12RHIRasterizerState : public CRHIRasterizerState, public CD3D12DeviceChild
{
public:
    CD3D12RHIRasterizerState( CD3D12Device* InDevice, const D3D12_RASTERIZER_DESC& InDesc )
        : CRHIRasterizerState()
        , CD3D12DeviceChild( InDevice )
        , Desc( InDesc )
    {
    }

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

class CD3D12RHIBlendState : public CRHIBlendState, public CD3D12DeviceChild
{
public:
    CD3D12RHIBlendState( CD3D12Device* InDevice, const D3D12_BLEND_DESC& InDesc )
        : CRHIBlendState()
        , CD3D12DeviceChild( InDevice )
        , Desc( InDesc )
    {
    }

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

class CD3D12RHIGraphicsPipelineState : public CRHIGraphicsPipelineState, public CD3D12DeviceChild
{
public:
    CD3D12RHIGraphicsPipelineState( CD3D12Device* InDevice );
    ~CD3D12RHIGraphicsPipelineState() = default;

    bool Init( const SGraphicsPipelineStateCreateInfo& CreateInfo );

    virtual void SetName( const CString& InName ) override final
    {
        CRHIResource::SetName( InName );

        WString WideName = CharToWide( InName );
        PipelineState->SetName( WideName.CStr() );
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

class CD3D12RHIComputePipelineState : public CRHIComputePipelineState, public CD3D12DeviceChild
{
public:
    
    CD3D12RHIComputePipelineState( CD3D12Device* InDevice, const TSharedRef<CD3D12RHIComputeShader>& InShader );
    ~CD3D12RHIComputePipelineState() = default;

    bool Init();

    virtual void SetName( const CString& InName ) override final
    {
        CRHIResource::SetName( InName );

        WString WideName = CharToWide( InName );
        PipelineState->SetName( WideName.CStr() );
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
    TSharedRef<CD3D12RHIComputeShader>  Shader;
    TSharedRef<CD3D12RootSignature> RootSignature;
};

struct SRayTracingShaderIdentifer
{
    char ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
};

class CD3D12RHIRayTracingPipelineState : public CRHIRayTracingPipelineState, public CD3D12DeviceChild
{
public:
    CD3D12RHIRayTracingPipelineState( CD3D12Device* InDevice );
    ~CD3D12RHIRayTracingPipelineState() = default;

    bool Init( const SRayTracingPipelineStateCreateInfo& CreateInfo );

    virtual void SetName( const CString& InName ) override
    {
        // Set resource name 
        CRHIResource::SetName( InName );

        // Set native debug name
        WString WideName = CharToWide( InName );
        StateObject->SetName( WideName.CStr() );
    }

    virtual void* GetNativeResource() const override final
    {
        return reinterpret_cast<void*>(StateObject.Get());
    }

    virtual bool IsValid() const
    {
        return StateObject != nullptr;
    }

    void* GetShaderIdentifer( const CString& ExportName );

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

    THashTable<CString, SRayTracingShaderIdentifer, SStringHasher> ShaderIdentifers;
};