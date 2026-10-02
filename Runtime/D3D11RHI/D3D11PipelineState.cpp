#include "D3D11RHI/D3D11PipelineState.h"
#include "D3D11RHI/D3D11Device.h"
#include "D3D11RHI/D3D11RHI.h"

FD3D11InputLayoutRHI::FD3D11InputLayoutRHI(const TArray<FRHIInputElementDesc>& InInputElements)
    : FRHIInputLayout()
    , InputElements(InInputElements)
    , SemanticNames()
    , ElementDesc()
{
    const int32 NumElements = InInputElements.Size();
    ElementDesc.Reserve(NumElements);
    SemanticNames.Reserve(NumElements);

    for (const FRHIInputElementDesc& Element : InInputElements)
    {
        D3D11_INPUT_ELEMENT_DESC InputElementDesc = {};

        const String& Semantic = SemanticNames.Emplace(Element.Semantic);
        InputElementDesc.SemanticName         = *Semantic;
        InputElementDesc.SemanticIndex        = Element.SemanticIndex;
        InputElementDesc.Format               = ConvertFormat(Element.Format);
        InputElementDesc.InputSlot            = Element.InputSlot;
        InputElementDesc.AlignedByteOffset    = Element.ByteOffset;
        InputElementDesc.InputSlotClass       = ConvertVertexInputClass(Element.InputClass);
        InputElementDesc.InstanceDataStepRate = Element.InputClass == EVertexInputClass::Vertex ? 0 : Element.InstanceStepRate;

        ElementDesc.Emplace(InputElementDesc);
    }
}

FD3D11InputLayoutRHI::~FD3D11InputLayoutRHI() = default;

const FRHIInputElementDesc* FD3D11InputLayoutRHI::GetInputElementDesc(uint32 Index) const
{
    return &InputElements[Index];
}

uint32 FD3D11InputLayoutRHI::GetNumInputElementDescs() const
{
    return InputElements.Size();
}

FD3D11DepthStencilStateRHI::FD3D11DepthStencilStateRHI(FD3D11Device* InDevice, const FRHIDepthStencilStateDesc& InDesc)
    : FRHIDepthStencilState(InDesc)
    , FD3D11DeviceChild(InDevice)
    , State(nullptr)
{
}

FD3D11DepthStencilStateRHI::~FD3D11DepthStencilStateRHI() = default;

bool FD3D11DepthStencilStateRHI::Initialize()
{
    if (Desc.bDepthBoundsTestEnable)
    {
        D3D11_ERROR("[FD3D11DepthStencilStateRHI]: D3D11 does not have a depth bounds test");
        return false;
    }

    D3D11_DEPTH_STENCIL_DESC D3DDepthStencilDesc = {};
    D3DDepthStencilDesc.DepthFunc        = ConvertComparisonFunc(Desc.DepthFunc);
    D3DDepthStencilDesc.DepthEnable      = Desc.bDepthEnable;
    D3DDepthStencilDesc.DepthWriteMask   = Desc.bDepthWriteEnable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    D3DDepthStencilDesc.StencilEnable    = Desc.bStencilEnable;
    D3DDepthStencilDesc.StencilReadMask  = static_cast<uint8>(Desc.StencilReadMask);
    D3DDepthStencilDesc.StencilWriteMask = static_cast<uint8>(Desc.StencilWriteMask);
    D3DDepthStencilDesc.FrontFace        = ConvertStencilState(Desc.FrontFace);
    D3DDepthStencilDesc.BackFace         = ConvertStencilState(Desc.BackFace);

    const HRESULT Result = GetDevice()->GetD3D11Device()->CreateDepthStencilState(&D3DDepthStencilDesc, &State);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11DepthStencilStateRHI]: FAILED to create DepthStencilState (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    return true;
}

FD3D11RasterizerStateRHI::FD3D11RasterizerStateRHI(FD3D11Device* InDevice, const FRHIRasterizerStateDesc& InDesc)
    : FRHIRasterizerState(InDesc)
    , FD3D11DeviceChild(InDevice)
    , State(nullptr)
{
}

FD3D11RasterizerStateRHI::~FD3D11RasterizerStateRHI() = default;

bool FD3D11RasterizerStateRHI::Initialize()
{
    if (Desc.bEnableConservativeRaster)
    {
        D3D11_ERROR("[FD3D11RasterizerStateRHI]: D3D11RHI does not support conservative rasterization");
        return false;
    }

    D3D11_RASTERIZER_DESC1 D3DRasterizerDesc = {};
    D3DRasterizerDesc.AntialiasedLineEnable = Desc.bAntialiasedLineEnable;
    D3DRasterizerDesc.CullMode              = ConvertCullMode(Desc.CullMode);
    D3DRasterizerDesc.DepthBias             = static_cast<INT>(Desc.DepthBias);
    D3DRasterizerDesc.DepthBiasClamp        = Desc.DepthBiasClamp;
    D3DRasterizerDesc.DepthClipEnable       = Desc.bDepthClipEnable;
    D3DRasterizerDesc.SlopeScaledDepthBias  = Desc.SlopeScaledDepthBias;
    D3DRasterizerDesc.FillMode              = ConvertFillMode(Desc.FillMode);
    D3DRasterizerDesc.ForcedSampleCount     = Desc.ForcedSampleCount;
    D3DRasterizerDesc.FrontCounterClockwise = Desc.bFrontCounterClockwise;
    D3DRasterizerDesc.MultisampleEnable     = Desc.bMultisampleEnable;
    D3DRasterizerDesc.ScissorEnable         = TRUE;

    ID3D11Device1* D3D11Device1 = GetDevice()->GetD3D11Device1();
    if (!D3D11Device1)
    {
        D3D11_ERROR("[FD3D11RasterizerStateRHI]: D3D11RHI requires ID3D11Device1");
        return false;
    }

    TComPtr<ID3D11RasterizerState1> NewState;
    const HRESULT Result = D3D11Device1->CreateRasterizerState1(&D3DRasterizerDesc, &NewState);
    State = NewState;

    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11RasterizerStateRHI]: FAILED to create RasterizerState (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    return true;
}

FD3D11BlendStateRHI::FD3D11BlendStateRHI(FD3D11Device* InDevice, const FRHIBlendStateDesc& InDesc)
    : FRHIBlendState(InDesc)
    , FD3D11DeviceChild(InDevice)
    , State(nullptr)
{
}

FD3D11BlendStateRHI::~FD3D11BlendStateRHI() = default;

bool FD3D11BlendStateRHI::Initialize()
{
    D3D11_BLEND_DESC1 D3DBlendDesc = {};
    D3DBlendDesc.AlphaToCoverageEnable  = Desc.bAlphaToCoverageEnable;
    D3DBlendDesc.IndependentBlendEnable = Desc.bIndependentBlendEnable;

    const D3D11_LOGIC_OP LogicOp = ConvertLogicOp(Desc.LogicOp);
    for (int32 Index = 0; Index < D3D11_MAX_RENDER_TARGET_COUNT; ++Index)
    {
        D3D11_RENDER_TARGET_BLEND_DESC1& RenderTarget = D3DBlendDesc.RenderTarget[Index];
        if (Index < Desc.NumRenderTargets)
        {
            RenderTarget.BlendEnable           = Desc.RenderTargets[Index].bBlendEnable;
            RenderTarget.BlendOp               = ConvertBlendOp(Desc.RenderTargets[Index].BlendOp);
            RenderTarget.BlendOpAlpha          = ConvertBlendOp(Desc.RenderTargets[Index].BlendOpAlpha);
            RenderTarget.DestBlend             = ConvertBlend(Desc.RenderTargets[Index].DstBlend);
            RenderTarget.DestBlendAlpha        = ConvertBlend(Desc.RenderTargets[Index].DstBlendAlpha);
            RenderTarget.SrcBlend              = ConvertBlend(Desc.RenderTargets[Index].SrcBlend);
            RenderTarget.SrcBlendAlpha         = ConvertBlend(Desc.RenderTargets[Index].SrcBlendAlpha);
            RenderTarget.RenderTargetWriteMask = ConvertColorWriteFlags(Desc.RenderTargets[Index].ColorWriteMask);
            RenderTarget.LogicOp               = LogicOp;
            RenderTarget.LogicOpEnable         = Desc.bLogicOpEnable;
        }
        else
        {
            RenderTarget.BlendOp               = D3D11_BLEND_OP_ADD;
            RenderTarget.BlendOpAlpha          = D3D11_BLEND_OP_ADD;
            RenderTarget.DestBlend             = D3D11_BLEND_ZERO;
            RenderTarget.DestBlendAlpha        = D3D11_BLEND_ZERO;
            RenderTarget.SrcBlend              = D3D11_BLEND_ONE;
            RenderTarget.SrcBlendAlpha         = D3D11_BLEND_ONE;
            RenderTarget.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            RenderTarget.LogicOp               = D3D11_LOGIC_OP_NOOP;
        }
    }

    ID3D11Device1* D3D11Device1 = GetDevice()->GetD3D11Device1();
    if (!D3D11Device1)
    {
        D3D11_ERROR("[FD3D11BlendStateRHI]: D3D11RHI requires ID3D11Device1");
        return false;
    }

    TComPtr<ID3D11BlendState1> NewState;
    const HRESULT Result = D3D11Device1->CreateBlendState1(&D3DBlendDesc, &NewState);
    State = NewState;

    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11BlendStateRHI]: FAILED to create BlendState (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    return true;
}

FD3D11GraphicsPipelineStateRHI::FD3D11GraphicsPipelineStateRHI(FD3D11Device* InDevice)
    : FRHIGraphicsPipelineState()
    , FD3D11DeviceChild(InDevice)
    , VertexShader(nullptr)
    , HullShader(nullptr)
    , DomainShader(nullptr)
    , GeometryShader(nullptr)
    , PixelShader(nullptr)
    , StreamOutputShader(nullptr)
    , InputLayout(nullptr)
    , RasterizerState(nullptr)
    , DepthStencilState(nullptr)
    , BlendState(nullptr)
    , StaticSamplers()
    , PrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
    , SampleMask(D3D11_DEFAULT_SAMPLE_MASK)
    , DebugName()
{
}

FD3D11GraphicsPipelineStateRHI::~FD3D11GraphicsPipelineStateRHI() = default;

void FD3D11GraphicsPipelineStateRHI::SetDebugName(const String& InName)
{
    DebugName = InName;
}

void FD3D11GraphicsPipelineStateRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName = DebugName;
}

bool FD3D11GraphicsPipelineStateRHI::CreateStreamOutputShader(const FRHIStreamOutputDeclaration& StreamOutputDeclaration)
{
    FD3D11Shader* SourceShader = VertexShader.Get();
    if (GeometryShader)
    {
        SourceShader = GeometryShader.Get();
    }
    else if (DomainShader)
    {
        SourceShader = DomainShader.Get();
    }

    TArray<D3D11_SO_DECLARATION_ENTRY> StreamOutputEntries;
    for (const FRHIStreamOutputEntry& Entry : StreamOutputDeclaration.Entries)
    {
        D3D11_SO_DECLARATION_ENTRY D3DEntry = {};
        D3DEntry.Stream         = 0;
        D3DEntry.SemanticName   = Entry.SemanticName;
        D3DEntry.SemanticIndex  = Entry.SemanticIndex;
        D3DEntry.StartComponent = Entry.StartComponent;
        D3DEntry.ComponentCount = Entry.ComponentCount;
        D3DEntry.OutputSlot     = Entry.OutputSlot;

        StreamOutputEntries.Emplace(D3DEntry);
    }

    TArray<UINT> StreamOutputStrides;
    for (uint32 Stride : StreamOutputDeclaration.BufferStrides)
    {
        StreamOutputStrides.Emplace(Stride);
    }

    const TArray<uint8>& ByteCode = SourceShader->GetByteCode();

    const HRESULT Result = GetDevice()->GetD3D11Device()->CreateGeometryShaderWithStreamOutput(ByteCode.Data(), ByteCode.Size(),
        StreamOutputEntries.Data(), StreamOutputEntries.Size(), StreamOutputStrides.Data(), StreamOutputStrides.Size(),
        StreamOutputDeclaration.RasterizedStream, nullptr, &StreamOutputShader);

    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11GraphicsPipelineStateRHI]: FAILED to create the stream output GeometryShader (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    return true;
}

FD3D11Shader* FD3D11GraphicsPipelineStateRHI::GetShader(EShaderVisibility::Type ShaderStage) const
{
    switch (ShaderStage)
    {
        case EShaderVisibility::Vertex:   return VertexShader.Get();
        case EShaderVisibility::Hull:     return HullShader.Get();
        case EShaderVisibility::Domain:   return DomainShader.Get();
        case EShaderVisibility::Geometry: return GeometryShader.Get();
        case EShaderVisibility::Pixel:    return PixelShader.Get();
        default:                          return nullptr;
    }
}

bool FD3D11GraphicsPipelineStateRHI::Initialize(const FRHIGraphicsPipelineStateDesc& Desc)
{
    if (Desc.ViewInstancingState.bEnableViewInstancing)
    {
        D3D11_ERROR("[FD3D11GraphicsPipelineStateRHI]: D3D11 does not have view instancing");
        return false;
    }

    VertexShader = MakeSharedRef<FD3D11VertexShaderRHI>(FD3D11DeviceRHI::ResourceCast(Desc.VertexShader));
    if (!VertexShader)
    {
        D3D11_ERROR_CRITICAL("VertexShader cannot be nullptr");
        return false;
    }

    HullShader     = MakeSharedRef<FD3D11HullShaderRHI>(FD3D11DeviceRHI::ResourceCast(Desc.HullShader));
    DomainShader   = MakeSharedRef<FD3D11DomainShaderRHI>(FD3D11DeviceRHI::ResourceCast(Desc.DomainShader));
    GeometryShader = MakeSharedRef<FD3D11GeometryShaderRHI>(FD3D11DeviceRHI::ResourceCast(Desc.GeometryShader));
    PixelShader    = MakeSharedRef<FD3D11PixelShaderRHI>(FD3D11DeviceRHI::ResourceCast(Desc.PixelShader));

    if (Desc.StreamOutputDeclaration && !CreateStreamOutputShader(*Desc.StreamOutputDeclaration))
    {
        return false;
    }

    RasterizerState = MakeSharedRef<FD3D11RasterizerStateRHI>(FD3D11DeviceRHI::ResourceCast(Desc.RasterizerState));
    if (!RasterizerState)
    {
        D3D11_ERROR_CRITICAL("RasterizerState cannot be nullptr");
        return false;
    }

    DepthStencilState = MakeSharedRef<FD3D11DepthStencilStateRHI>(FD3D11DeviceRHI::ResourceCast(Desc.DepthStencilState));
    if (!DepthStencilState)
    {
        D3D11_ERROR_CRITICAL("DepthStencilState cannot be nullptr");
        return false;
    }

    BlendState = MakeSharedRef<FD3D11BlendStateRHI>(FD3D11DeviceRHI::ResourceCast(Desc.BlendState));
    if (!BlendState)
    {
        D3D11_ERROR_CRITICAL("BlendState cannot be nullptr");
        return false;
    }

    if (FD3D11InputLayoutRHI* D3D11InputLayout = FD3D11DeviceRHI::ResourceCast(Desc.InputLayout))
    {
        const TArray<D3D11_INPUT_ELEMENT_DESC>& ElementDescs = D3D11InputLayout->GetElementDescs();
        if (!ElementDescs.IsEmpty())
        {
            const TArray<uint8>& VertexShaderCode = VertexShader->GetByteCode();

            const HRESULT Result = GetDevice()->GetD3D11Device()->CreateInputLayout(ElementDescs.Data(), ElementDescs.Size(), VertexShaderCode.Data(), VertexShaderCode.Size(), &InputLayout);
            if (FAILED(Result))
            {
                D3D11_ERROR("[FD3D11GraphicsPipelineStateRHI]: FAILED to create InputLayout (0x%08X)", static_cast<uint32>(Result));
                return false;
            }
        }
    }

    for (const FRHIStaticSamplerInfo& StaticSamplerInfo : Desc.StaticSamplers)
    {
        FD3D11StaticSampler& StaticSampler = StaticSamplers.Emplace();
        StaticSampler.Visibility = GetShaderVisibility(StaticSamplerInfo.ShaderVisibility);
        StaticSampler.Register   = StaticSamplerInfo.ShaderRegister;

        if (!GetDevice()->FindOrCreateSamplerState(StaticSamplerInfo, StaticSampler.Sampler))
        {
            D3D11_ERROR("[FD3D11GraphicsPipelineStateRHI]: Failed to create static sampler for register s%u", StaticSamplerInfo.ShaderRegister);
            return false;
        }
    }

    PrimitiveTopology = ConvertPrimitiveTopology(Desc.PrimitiveTopology);
    SampleMask        = Desc.MultiSampleState.SampleMask;
    return true;
}

FD3D11ComputePipelineStateRHI::FD3D11ComputePipelineStateRHI(FD3D11Device* InDevice)
    : FRHIComputePipelineState()
    , FD3D11DeviceChild(InDevice)
    , Shader(nullptr)
    , StaticSamplers()
    , DebugName()
{
}

FD3D11ComputePipelineStateRHI::~FD3D11ComputePipelineStateRHI() = default;

void FD3D11ComputePipelineStateRHI::SetDebugName(const String& InName)
{
    DebugName = InName;
}

void FD3D11ComputePipelineStateRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName = DebugName;
}

bool FD3D11ComputePipelineStateRHI::Initialize(const FRHIComputePipelineStateDesc& Desc)
{
    Shader = MakeSharedRef<FD3D11ComputeShaderRHI>(FD3D11DeviceRHI::ResourceCast(Desc.Shader));
    if (!Shader)
    {
        D3D11_ERROR_CRITICAL("ComputeShader cannot be nullptr");
        return false;
    }

    for (const FRHIStaticSamplerInfo& StaticSamplerInfo : Desc.StaticSamplers)
    {
        FD3D11StaticSampler& StaticSampler = StaticSamplers.Emplace();
        StaticSampler.Visibility = EShaderVisibility::Compute;
        StaticSampler.Register   = StaticSamplerInfo.ShaderRegister;

        if (!GetDevice()->FindOrCreateSamplerState(StaticSamplerInfo, StaticSampler.Sampler))
        {
            D3D11_ERROR("[FD3D11ComputePipelineStateRHI]: Failed to create static sampler for register s%u", StaticSamplerInfo.ShaderRegister);
            return false;
        }
    }

    return true;
}
