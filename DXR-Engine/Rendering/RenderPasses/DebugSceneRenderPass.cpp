#include "DebugSceneRenderPass.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

Bool DebugSceneRenderPass::Init(SharedRenderPassResources& FrameResources)
{
    TArray<UInt8> ShaderCode;
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/Debug.hlsl",
        "VSMain",
        nullptr,
        EShaderStage::ShaderStage_Vertex,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    TSharedRef<VertexShader> VShader = RenderLayer::CreateVertexShader(ShaderCode);
    if (!VShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        VShader->SetName("Debug VertexShader");
    }

    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/Debug.hlsl",
        "PSMain",
        nullptr,
        EShaderStage::ShaderStage_Pixel,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    TSharedRef<PixelShader> PShader = RenderLayer::CreatePixelShader(ShaderCode);
    if (!PShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PShader->SetName("Debug PixelShader");
    }

    InputLayoutStateCreateInfo InputLayout =
    {
        { "POSITION", 0, EFormat::Format_R32G32B32_Float, 0, 0, EInputClassification::InputClassification_Vertex, 0 },
    };

    TSharedRef<InputLayoutState> InputLayoutState = RenderLayer::CreateInputLayout(InputLayout);
    if (!InputLayoutState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        InputLayoutState->SetName("Debug InputLayoutState");
    }

    DepthStencilStateCreateInfo DepthStencilStateInfo;
    DepthStencilStateInfo.DepthFunc      = EComparisonFunc::ComparisonFunc_LessEqual;
    DepthStencilStateInfo.DepthEnable    = false;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::DepthWriteMask_Zero;

    TSharedRef<DepthStencilState> DepthStencilState = RenderLayer::CreateDepthStencilState(DepthStencilStateInfo);
    if (!DepthStencilState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        DepthStencilState->SetName("Debug DepthStencilState");
    }

    RasterizerStateCreateInfo RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::CullMode_None;

    TSharedRef<RasterizerState> RasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RasterizerState->SetName("Debug RasterizerState");
    }

    BlendStateCreateInfo BlendStateInfo;

    TSharedRef<BlendState> BlendState = RenderLayer::CreateBlendState(BlendStateInfo);
    if (!BlendState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        BlendState->SetName("Debug BlendState");
    }

    GraphicsPipelineStateCreateInfo PSOProperties;
    PSOProperties.BlendState        = BlendState.Get();
    PSOProperties.DepthStencilState = DepthStencilState.Get();
    PSOProperties.InputLayoutState  = InputLayoutState.Get();
    PSOProperties.RasterizerState   = RasterizerState.Get();
    PSOProperties.ShaderState.VertexShader  = VShader.Get();
    PSOProperties.ShaderState.PixelShader   = PShader.Get();
    PSOProperties.PrimitiveTopologyType     = EPrimitiveTopologyType::PrimitiveTopologyType_Line;
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = FrameResources.RenderTargetFormat;
    PSOProperties.PipelineFormats.NumRenderTargets       = 1;
    PSOProperties.PipelineFormats.DepthStencilFormat     = FrameResources.DepthBufferFormat;

    PipelineState = RenderLayer::CreateGraphicsPipelineState(PSOProperties);
    if (!PipelineState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PipelineState->SetName("Debug PipelineState");
    }

    XMFLOAT3 Vertices[8] =
    {
        { -0.5f, -0.5f,  0.5f },
        {  0.5f, -0.5f,  0.5f },
        { -0.5f,  0.5f,  0.5f },
        {  0.5f,  0.5f,  0.5f },

        {  0.5f, -0.5f, -0.5f },
        { -0.5f, -0.5f, -0.5f },
        {  0.5f,  0.5f, -0.5f },
        { -0.5f,  0.5f, -0.5f }
    };

    ResourceData VertexData(Vertices);

    AABBVertexBuffer = RenderLayer::CreateVertexBuffer<XMFLOAT3>(&VertexData, 8, BufferUsage_Default);
    if (!AABBVertexBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        AABBVertexBuffer->SetName("AABB VertexBuffer");
    }

    // Create IndexBuffer
    UInt16 Indices[24] =
    {
        0, 1,
        1, 3,
        3, 2,
        2, 0,
        1, 4,
        3, 6,
        6, 4,
        4, 5,
        5, 7,
        7, 6,
        0, 5,
        2, 7,
    };

    ResourceData IndexData(Indices);

    AABBIndexBuffer = RenderLayer::CreateIndexBuffer(
        &IndexData,
        sizeof(UInt16) * 24,
        EIndexFormat::IndexFormat_UInt16,
        BufferUsage_Default);
    if (!AABBIndexBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        AABBIndexBuffer->SetName("AABB IndexBuffer");
    }

    return true;
}

void DebugSceneRenderPass::Render(
    CommandList& CmdList, 
    SharedRenderPassResources& FrameResources,
    const Scene& Scene)
{
}
