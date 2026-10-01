#pragma once
#include "RHI/RHIResources.h"
#include "D3D11RHI/D3D11DeviceChild.h"
#include "D3D11RHI/D3D11Shader.h"

typedef TSharedRef<class FD3D11InputLayoutRHI>           FD3D11InputLayoutRHIRef;
typedef TSharedRef<class FD3D11DepthStencilStateRHI>     FD3D11DepthStencilStateRHIRef;
typedef TSharedRef<class FD3D11RasterizerStateRHI>       FD3D11RasterizerStateRHIRef;
typedef TSharedRef<class FD3D11BlendStateRHI>            FD3D11BlendStateRHIRef;
typedef TSharedRef<class FD3D11GraphicsPipelineStateRHI> FD3D11GraphicsPipelineStateRHIRef;

class FD3D11InputLayoutRHI : public FRHIInputLayout
{
public:
    FD3D11InputLayoutRHI(const TArray<FRHIInputElementDesc>& InInputElements);
    virtual ~FD3D11InputLayoutRHI();

    // FRHIInputLayout Interface
    virtual void* GetRHINativeState() const override final { return nullptr; }

    virtual const FRHIInputElementDesc* GetInputElementDesc(uint32 Index) const override final;
    virtual uint32 GetNumInputElementDescs() const override final;

    FORCEINLINE const TArray<D3D11_INPUT_ELEMENT_DESC>& GetElementDescs() const
    {
        return ElementDesc;
    }

private:
    TArray<FRHIInputElementDesc>     InputElements;
    TArray<String>                   SemanticNames;
    TArray<D3D11_INPUT_ELEMENT_DESC> ElementDesc;
};

class FD3D11DepthStencilStateRHI : public FRHIDepthStencilState, public FD3D11DeviceChild
{
public:
    FD3D11DepthStencilStateRHI(FD3D11Device* InDevice, const FRHIDepthStencilStateDesc& InDesc);
    virtual ~FD3D11DepthStencilStateRHI();

    // FRHIDepthStencilState Interface
    virtual void* GetRHINativeState() const override final { return State.Get(); }

    bool Initialize();

    FORCEINLINE ID3D11DepthStencilState* GetD3D11State() const
    {
        return State.Get();
    }

private:
    TComPtr<ID3D11DepthStencilState> State;
};

class FD3D11RasterizerStateRHI : public FRHIRasterizerState, public FD3D11DeviceChild
{
public:
    FD3D11RasterizerStateRHI(FD3D11Device* InDevice, const FRHIRasterizerStateDesc& InDesc);
    virtual ~FD3D11RasterizerStateRHI();

    // FRHIRasterizerState Interface
    virtual void* GetRHINativeState() const override final { return State.Get(); }

    bool Initialize();

    FORCEINLINE ID3D11RasterizerState* GetD3D11State() const
    {
        return State.Get();
    }

private:
    TComPtr<ID3D11RasterizerState> State;
};

class FD3D11BlendStateRHI : public FRHIBlendState, public FD3D11DeviceChild
{
public:
    FD3D11BlendStateRHI(FD3D11Device* InDevice, const FRHIBlendStateDesc& InDesc);
    virtual ~FD3D11BlendStateRHI();

    // FRHIBlendState Interface
    virtual void* GetRHINativeState() const override final { return State.Get(); }

    bool Initialize();

    FORCEINLINE ID3D11BlendState* GetD3D11State() const
    {
        return State.Get();
    }

private:
    TComPtr<ID3D11BlendState> State;
};

struct FD3D11StaticSampler
{
    EShaderVisibility::Type     Visibility;
    uint32                      Register;
    TComPtr<ID3D11SamplerState> Sampler;
};

class FD3D11GraphicsPipelineStateRHI : public FRHIGraphicsPipelineState, public FD3D11DeviceChild
{
public:
    FD3D11GraphicsPipelineStateRHI(FD3D11Device* InDevice);
    virtual ~FD3D11GraphicsPipelineStateRHI();

    // FRHIPipelineState Interface
    virtual void* GetRHINativeState() const override final { return nullptr; }

    virtual void SetDebugName(const String& InName) override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    bool Initialize(const FRHIGraphicsPipelineStateDesc& Desc);

    FD3D11Shader* GetShader(EShaderVisibility::Type ShaderStage) const;

    FORCEINLINE FD3D11VertexShaderRHI*      GetVertexShader()      const { return VertexShader.Get(); }
    FORCEINLINE FD3D11HullShaderRHI*        GetHullShader()        const { return HullShader.Get(); }
    FORCEINLINE FD3D11DomainShaderRHI*      GetDomainShader()      const { return DomainShader.Get(); }
    FORCEINLINE FD3D11GeometryShaderRHI*    GetGeometryShader()    const { return GeometryShader.Get(); }
    FORCEINLINE FD3D11PixelShaderRHI*       GetPixelShader()       const { return PixelShader.Get(); }
    FORCEINLINE ID3D11InputLayout*          GetInputLayout()       const { return InputLayout.Get(); }
    FORCEINLINE FD3D11RasterizerStateRHI*   GetRasterizerState()   const { return RasterizerState.Get(); }
    FORCEINLINE FD3D11DepthStencilStateRHI* GetDepthStencilState() const { return DepthStencilState.Get(); }
    FORCEINLINE FD3D11BlendStateRHI*        GetBlendState()        const { return BlendState.Get(); }
    FORCEINLINE D3D11_PRIMITIVE_TOPOLOGY    GetPrimitiveTopology() const { return PrimitiveTopology; }
    FORCEINLINE uint32                      GetSampleMask()        const { return SampleMask; }

    FORCEINLINE const TArray<FD3D11StaticSampler>& GetStaticSamplers() const
    {
        return StaticSamplers;
    }

private:
    FD3D11VertexShaderRHIRef      VertexShader;
    FD3D11HullShaderRHIRef        HullShader;
    FD3D11DomainShaderRHIRef      DomainShader;
    FD3D11GeometryShaderRHIRef    GeometryShader;
    FD3D11PixelShaderRHIRef       PixelShader;
    TComPtr<ID3D11InputLayout>    InputLayout;
    FD3D11RasterizerStateRHIRef   RasterizerState;
    FD3D11DepthStencilStateRHIRef DepthStencilState;
    FD3D11BlendStateRHIRef        BlendState;
    TArray<FD3D11StaticSampler>   StaticSamplers;
    D3D11_PRIMITIVE_TOPOLOGY      PrimitiveTopology;
    uint32                        SampleMask;
    String                        DebugName;
};
