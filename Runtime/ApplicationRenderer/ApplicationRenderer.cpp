#include "ApplicationRenderer/ApplicationRenderer.h"
#include "ApplicationRenderer/UIScreenshot.h"
#include "Core/Math/Math.h"
#include "Core/Math/VectorMath/VectorMath.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Modules/ModuleManager.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/Tasks/Tasks.h"
#include "Core/Time/Timespan.h"
#include "Application/Application.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Elements/Window.h"
#include "Application/Style/UIStyle.h"
#include "Application/Text/FontAtlas.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIResources.h"
#include "RHI/ShaderCompiler.h"
#include "RendererCore/TextureFactory.h"

IMPLEMENT_ENGINE_MODULE(IModule, ApplicationRenderer);

static TAutoConsoleVariable<bool> CVarUIAntiAliasing(
    "UI.AntiAliasing",
    "True lays a one pixel soft edge over curved user interface geometry",
    true,
    EConsoleVariableFlags::Default);

static constexpr int32 GVertexGrowth = 4096;
static constexpr int32 GIndexGrowth  = 8192;

static TAutoConsoleVariable<int32> CVarDumpDrawData(
    "ApplicationRenderer.DumpDrawData",
    "Logs the commands, geometry and batches of every window for this many frames, counting itself back down to zero",
    0);

static TAutoConsoleVariable<bool> CVarPaintTiming(
    "ApplicationRenderer.PaintTiming",
    "Logs what the per-frame UI rebuild costs each window, averaged over the reporting interval",
    false,
    EConsoleVariableFlags::Default);

static bool GVSyncEnabled = false;
static FAutoConsoleVariableRef CVarVSyncEnabled(
    "Renderer.Feature.VerticalSync",
    "Enables Vertical-Sync",
    GVSyncEnabled,
    EConsoleVariableFlags::Default);

static constexpr int32   GPaintTimingFrames = 120;
static constexpr EFormat GSnapshotFormat    = EFormat::R8G8B8A8_Unorm;

static float ToMillisecondsSince(uint64 StartTime)
{
    const uint64 Elapsed = FPlatformTime::QueryPerformanceCounter() - StartTime;
    return static_cast<float>((static_cast<double>(Elapsed) * 1000.0) / static_cast<double>(FPlatformTime::QueryPerformanceFrequency()));
}

static void ConvertIndicesToUInt16(const uint32* Source, uint16* Destination, int32 Count)
{
    int32 Index = 0;

#if USE_INT_VECTOR_MATH
    for (; Index + 8 <= Count; Index += 8)
    {
        const FInt128 Low    = FVectorMath::VectorLoadUInt(Source + Index);
        const FInt128 High   = FVectorMath::VectorLoadUInt(Source + Index + 4);
        const FInt128 Packed = FVectorMath::VectorPackUInt32ToUInt16(Low, High);
        FVectorMath::VectorStoreUInt16(Packed, Destination + Index);
    }
#endif

    for (; Index < Count; ++Index)
    {
        Destination[Index] = static_cast<uint16>(Source[Index]);
    }
}

struct FApplicationUIConstants
{
    float ProjectionMatrix[4][4];
};

struct FScopedApplicationGPUTrace
{
    FScopedApplicationGPUTrace(IGPUProfiler* InProfiler, FRHICommandList& InCommandList, const CHAR* InName)
        : Profiler(InProfiler)
        , CommandList(InCommandList)
        , Name(InName)
    {
        if (Profiler)
        {
            Profiler->BeginGPUTrace(CommandList, Name);
        }
    }

    ~FScopedApplicationGPUTrace()
    {
        if (Profiler)
        {
            Profiler->EndGPUTrace(CommandList, Name);
        }
    }

    IGPUProfiler*    Profiler;
    FRHICommandList& CommandList;
    const CHAR*      Name;
};

static String DescribeRectangle(const FRectangle& Rectangle)
{
    return String::Printf("(%d, %d, %d x %d)", Rectangle.Position.X, Rectangle.Position.Y, Rectangle.Width, Rectangle.Height);
}

static const CHAR* DescribeBatchTexture(const FUITextureHandle& Texture)
{
    if (Texture.Atlas)
    {
        return "atlas";
    }

    return Texture.Texture ? "image" : "white";
}

static void DumpWindowDrawData(const FWindow& Window, const FDrawCommandList& Commands, const FUIDrawData& DrawData)
{
    LOG_INFO("[FApplicationRenderer]: Draw data for '%s': %d commands (%d box, %d outline, %d text, %d line, %d polyline, "
        "%d polygon, %d image, %d clip push, %d clip pop), %d vertices, %d indices, %d batches",
        *Window.GetTitle(), Commands.Size(), Commands.CountCommandsOfType(EDrawCommandType::Box),
        Commands.CountCommandsOfType(EDrawCommandType::BoxOutline), Commands.CountCommandsOfType(EDrawCommandType::Text),
        Commands.CountCommandsOfType(EDrawCommandType::Line), Commands.CountCommandsOfType(EDrawCommandType::Polyline),
        Commands.CountCommandsOfType(EDrawCommandType::ConvexPolygon), Commands.CountCommandsOfType(EDrawCommandType::Image),
        0, 0, DrawData.GetVertices().Size() + DrawData.GetShapeVertices().Size(),
        DrawData.GetIndices().Size() + DrawData.GetShapeIndices().Size(), DrawData.GetBatches().Size());

    if (DrawData.GetVertices().Size() >= FUIDrawData::MaxVertexCount)
    {
        LOG_WARNING("[FApplicationRenderer]: The vertex budget of %d is full, so geometry was dropped this frame", FUIDrawData::MaxVertexCount);
    }

    const TArray<FUIDrawBatch>& Batches = DrawData.GetBatches();
    for (int32 Index = 0; Index < Batches.Size(); ++Index)
    {
        const FUIDrawBatch& Batch = Batches[Index];
        LOG_INFO("[FApplicationRenderer]:   Batch %d: %s, scissor %s, indices %d to %d",
            Index, DescribeBatchTexture(Batch.Texture), Batch.bIsClipped ? *DescribeRectangle(Batch.ScissorRectangle) : "none",
            Batch.IndexOffset, Batch.IndexOffset + Batch.IndexCount);
    }
}

FApplicationRenderer::FApplicationRenderer()
    : WindowStates()
    , RetiredTextures()
    , CommandList()
    , ResizeCommandList()
    , GPUProfiler(nullptr)
    , LastFrameFinishedEvent(nullptr)
    , FrameCounter(0)
    , bHasOpenFrame(false)
    , VShader(nullptr)
    , PShader(nullptr)
    , InputLayout(nullptr)
    , ShapeVShader(nullptr)
    , ShapePShader(nullptr)
    , ShapeInputLayout(nullptr)
    , TextVShader(nullptr)
    , TextInputLayout(nullptr)
    , DepthStencilState(nullptr)
    , RasterizerState(nullptr)
    , BlendState(nullptr)
    , LinearSampler(nullptr)
    , PipelineState(nullptr)
    , ShapePipelineState(nullptr)
    , TextPipelineState(nullptr)
    , DefaultTexture(nullptr)
    , AtlasTextures()
    , PipelineStateFormat(EFormat::Unknown)
    , ActiveWindowState(nullptr)
{
}

FApplicationRenderer::~FApplicationRenderer()
{
    ReleaseRHI();
}

bool FApplicationRenderer::InitializeRHI()
{
    TArray<uint8> ShaderCode;

    FShaderCompileInfo CompileInfo("VSMain", EShaderModel::SM_6_2, EShaderStage::Vertex);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/UserInterface.hlsl", CompileInfo, ShaderCode))
    {
        LOG_ERROR("[FApplicationRenderer]: Failed to compile the vertex shader");
        return false;
    }

    VShader = RHI::CreateVertexShader(ShaderCode);
    if (!VShader)
    {
        return false;
    }

    CompileInfo = FShaderCompileInfo("PSMain", EShaderModel::SM_6_2, EShaderStage::Pixel);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/UserInterface.hlsl", CompileInfo, ShaderCode))
    {
        LOG_ERROR("[FApplicationRenderer]: Failed to compile the pixel shader");
        return false;
    }

    PShader = RHI::CreatePixelShader(ShaderCode);
    if (!PShader)
    {
        return false;
    }

    TArray<FRHIInputElementDesc> InputElements =
    {
        { "POSITION", 0, EFormat::R32G32_Float,   sizeof(FUIVertex), 0, static_cast<uint32>(offsetof(FUIVertex, Position)), 0, EVertexInputClass::Vertex, 0 },
        { "TEXCOORD", 0, EFormat::R32G32_Float,   sizeof(FUIVertex), 0, static_cast<uint32>(offsetof(FUIVertex, TexCoord)), 1, EVertexInputClass::Vertex, 0 },
        { "COLOR",    0, EFormat::R8G8B8A8_Unorm, sizeof(FUIVertex), 0, static_cast<uint32>(offsetof(FUIVertex, Color)),    2, EVertexInputClass::Vertex, 0 },
    };

    InputLayout = RHI::CreateInputLayout(InputElements);
    if (!InputLayout)
    {
        return false;
    }

    CompileInfo = FShaderCompileInfo("VSText", EShaderModel::SM_6_2, EShaderStage::Vertex);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/UserInterface.hlsl", CompileInfo, ShaderCode))
    {
        LOG_ERROR("[FApplicationRenderer]: Failed to compile the text vertex shader");
        return false;
    }

    TextVShader = RHI::CreateVertexShader(ShaderCode);
    if (!TextVShader)
    {
        return false;
    }

    TArray<FRHIInputElementDesc> TextInputElements =
    {
        { "POSITION", 0, EFormat::R32G32_Float,   sizeof(FUITextGlyphInstance), 0, static_cast<uint32>(offsetof(FUITextGlyphInstance, Position)),    0, EVertexInputClass::Instance, 1 },
        { "TEXCOORD", 0, EFormat::R32G32_Float,   sizeof(FUITextGlyphInstance), 0, static_cast<uint32>(offsetof(FUITextGlyphInstance, Size)),        1, EVertexInputClass::Instance, 1 },
        { "TEXCOORD", 1, EFormat::R32G32_Float,   sizeof(FUITextGlyphInstance), 0, static_cast<uint32>(offsetof(FUITextGlyphInstance, MinTexCoord)), 2, EVertexInputClass::Instance, 1 },
        { "TEXCOORD", 2, EFormat::R32G32_Float,   sizeof(FUITextGlyphInstance), 0, static_cast<uint32>(offsetof(FUITextGlyphInstance, MaxTexCoord)), 3, EVertexInputClass::Instance, 1 },
        { "COLOR",    0, EFormat::R8G8B8A8_Unorm, sizeof(FUITextGlyphInstance), 0, static_cast<uint32>(offsetof(FUITextGlyphInstance, Color)),       4, EVertexInputClass::Instance, 1 },
    };

    TextInputLayout = RHI::CreateInputLayout(TextInputElements);
    if (!TextInputLayout)
    {
        return false;
    }

    CompileInfo = FShaderCompileInfo("VSShape", EShaderModel::SM_6_2, EShaderStage::Vertex);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/UserInterface.hlsl", CompileInfo, ShaderCode))
    {
        LOG_ERROR("[FApplicationRenderer]: Failed to compile the shape vertex shader");
        return false;
    }

    ShapeVShader = RHI::CreateVertexShader(ShaderCode);
    if (!ShapeVShader)
    {
        return false;
    }

    CompileInfo = FShaderCompileInfo("PSShape", EShaderModel::SM_6_2, EShaderStage::Pixel);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/UserInterface.hlsl", CompileInfo, ShaderCode))
    {
        LOG_ERROR("[FApplicationRenderer]: Failed to compile the shape pixel shader");
        return false;
    }

    ShapePShader = RHI::CreatePixelShader(ShaderCode);
    if (!ShapePShader)
    {
        return false;
    }

    TArray<FRHIInputElementDesc> ShapeInputElements =
    {
        { "POSITION",  0, EFormat::R32G32_Float,       sizeof(FUIShapeInstance), 0, static_cast<uint32>(offsetof(FUIShapeInstance, Position)),    0, EVertexInputClass::Instance, 1 },
        { "COLOR",     0, EFormat::R8G8B8A8_Unorm,     sizeof(FUIShapeInstance), 0, static_cast<uint32>(offsetof(FUIShapeInstance, Color)),       1, EVertexInputClass::Instance, 1 },
        { "TEXCOORD",  0, EFormat::R32G32_Float,       sizeof(FUIShapeInstance), 0, static_cast<uint32>(offsetof(FUIShapeInstance, DrawSize)),    2, EVertexInputClass::Instance, 1 },
        { "TEXCOORD",  1, EFormat::R32G32_Float,       sizeof(FUIShapeInstance), 0, static_cast<uint32>(offsetof(FUIShapeInstance, LocalOrigin)), 3, EVertexInputClass::Instance, 1 },
        { "TEXCOORD",  2, EFormat::R32G32_Float,       sizeof(FUIShapeInstance), 0, static_cast<uint32>(offsetof(FUIShapeInstance, RectSize)),    4, EVertexInputClass::Instance, 1 },
        { "TEXCOORD",  3, EFormat::R32G32B32A32_Float, sizeof(FUIShapeInstance), 0, static_cast<uint32>(offsetof(FUIShapeInstance, RadiusTL)),    5, EVertexInputClass::Instance, 1 },
        { "TEXCOORD",  4, EFormat::R32G32_Float,       sizeof(FUIShapeInstance), 0, static_cast<uint32>(offsetof(FUIShapeInstance, Thickness)),   6, EVertexInputClass::Instance, 1 },
    };

    ShapeInputLayout = RHI::CreateInputLayout(ShapeInputElements);
    if (!ShapeInputLayout)
    {
        return false;
    }

    FRHIDepthStencilStateDesc DepthStencilStateDesc;
    DepthStencilStateDesc.bDepthEnable      = false;
    DepthStencilStateDesc.bDepthWriteEnable = false;

    DepthStencilState = RHI::CreateDepthStencilState(DepthStencilStateDesc);
    if (!DepthStencilState)
    {
        return false;
    }

    FRHIRasterizerStateDesc RasterizerStateDesc;
    RasterizerStateDesc.CullMode = ECullMode::None;

    RasterizerState = RHI::CreateRasterizerState(RasterizerStateDesc);
    if (!RasterizerState)
    {
        return false;
    }

    FRHIBlendStateDesc BlendStateDesc;
    BlendStateDesc.bIndependentBlendEnable        = false;
    BlendStateDesc.NumRenderTargets               = 1;
    BlendStateDesc.RenderTargets[0].bBlendEnable  = true;
    BlendStateDesc.RenderTargets[0].SrcBlend      = EBlendType::One;
    BlendStateDesc.RenderTargets[0].SrcBlendAlpha = EBlendType::One;
    BlendStateDesc.RenderTargets[0].DstBlend      = EBlendType::InvSrcAlpha;
    BlendStateDesc.RenderTargets[0].DstBlendAlpha = EBlendType::InvSrcAlpha;
    BlendStateDesc.RenderTargets[0].BlendOp       = EBlendOp::Add;
    BlendStateDesc.RenderTargets[0].BlendOpAlpha  = EBlendOp::Add;

    BlendState = RHI::CreateBlendState(BlendStateDesc);
    if (!BlendState)
    {
        return false;
    }

    FRHISamplerStateDesc SamplerDesc;
    SamplerDesc.AddressU = ESamplerMode::Clamp;
    SamplerDesc.AddressV = ESamplerMode::Clamp;
    SamplerDesc.AddressW = ESamplerMode::Clamp;
    SamplerDesc.Filter   = ESamplerFilter::MinMagMipLinear;

    LinearSampler = RHI::CreateSamplerState(SamplerDesc);
    if (!LinearSampler)
    {
        return false;
    }

    const uint8 WhiteTexel[] = { 255, 255, 255, 255 };
    DefaultTexture = FTextureFactory::Get().LoadFromMemory(WhiteTexel, 1, 1, ETextureFactoryFlags::None, EFormat::R8G8B8A8_Unorm);

    if (!DefaultTexture)
    {
        return false;
    }

    DefaultTexture->SetDebugName("ApplicationUI DefaultTexture");
    return true;
}

void FApplicationRenderer::ReleaseRHI()
{
    if (LastFrameFinishedEvent)
    {
        LastFrameFinishedEvent->Wait(FTimespan::Infinity());
        FPlatformEvent::Recycle(LastFrameFinishedEvent);
        LastFrameFinishedEvent = nullptr;
    }

    CommandList.Reset();
    ResizeCommandList.Reset();

    bHasOpenFrame = false;
    GPUProfiler   = nullptr;

    ReleaseWindowSurfaces();

    WindowStates.Clear();

    ReleaseRetiredResources(true);

    VShader.Reset();
    PShader.Reset();
    InputLayout.Reset();
    ShapeVShader.Reset();
    ShapePShader.Reset();
    ShapeInputLayout.Reset();
    TextVShader.Reset();
    TextInputLayout.Reset();
    DepthStencilState.Reset();
    RasterizerState.Reset();
    BlendState.Reset();
    LinearSampler.Reset();
    PipelineState.Reset();
    ShapePipelineState.Reset();
    TextPipelineState.Reset();
    DefaultTexture.Reset();
    AtlasTextures.Clear();

    PipelineStateFormat = EFormat::Unknown;
    ActiveWindowState   = nullptr;
}

FDrawCommandList* FApplicationRenderer::BeginWindow(const TSharedPtr<FWindow>& InWindow)
{
    if (!InWindow)
    {
        return nullptr;
    }

    FWindowDrawState* WindowState = FindOrAddWindowState(InWindow);
    if (!WindowState)
    {
        return nullptr;
    }

    WindowState->Commands.Reset();
    WindowState->DrawData.Reset();

    WindowState->WalkStartTime = FPlatformTime::QueryPerformanceCounter();
    ActiveWindowState = WindowState;
    return &WindowState->Commands;
}

void FApplicationRenderer::EndWindow(const TSharedPtr<FWindow>& InWindow)
{
    if (!InWindow)
    {
        return;
    }

    FWindowDrawState* WindowState = ActiveWindowState;
    if (!WindowState || WindowState->Window != InWindow)
    {
        WindowState = FindWindowState(InWindow);
    }

    if (!WindowState)
    {
        return;
    }

    WindowState->Stats.ElementWalkTime = ToMillisecondsSince(WindowState->WalkStartTime);

    const uint64 BuildStartTime = FPlatformTime::QueryPerformanceCounter();
    WindowState->DrawData.SetAntiAliasingEnabled(CVarUIAntiAliasing.GetValue());
    WindowState->DrawData.BuildFromCommandList(WindowState->Commands);

    WindowState->Stats.GeometryBuildTime = ToMillisecondsSince(BuildStartTime);
    WindowState->Stats.CommandCount      = WindowState->Commands.GetCommands().Size();
    
    WindowState->Stats.VertexCount = WindowState->DrawData.GetVertices().Size() 
        + WindowState->DrawData.GetShapeInstances().Size()
        + WindowState->DrawData.GetTextGlyphInstances().Size();

    WindowState->Stats.BatchCount = WindowState->DrawData.GetBatches().Size();

    const int32 NumFramesToDump = CVarDumpDrawData.GetValue();
    if (NumFramesToDump > 0)
    {
        DumpWindowDrawData(*InWindow, WindowState->Commands, WindowState->DrawData);
        CVarDumpDrawData->SetAsInt(NumFramesToDump - 1, EConsoleVariableFlags::SetByCode);
    }

    ActiveWindowState = nullptr;
}

void FApplicationRenderer::ReportPaintStats(FWindowDrawState& WindowState)
{
    if (!CVarPaintTiming.GetValue())
    {
        WindowState.TimedFrameCount = 0;
        return;
    }

    FUIPaintStats& Total = WindowState.AccumulatedStats;
    const FUIPaintStats& Frame = WindowState.Stats;

    if (WindowState.TimedFrameCount == 0)
    {
        Total = FUIPaintStats();
    }

    Total.ElementWalkTime   += Frame.ElementWalkTime;
    Total.GeometryBuildTime += Frame.GeometryBuildTime;
    Total.BufferUploadTime  += Frame.BufferUploadTime;
    Total.CommandCount      += Frame.CommandCount;
    Total.VertexCount       += Frame.VertexCount;
    Total.BatchCount        += Frame.BatchCount;

    if (++WindowState.TimedFrameCount < GPaintTimingFrames)
    {
        return;
    }

    const TSharedPtr<FWindow> Window = WindowState.Window.ToSharedPtr();
    const float               Frames = static_cast<float>(WindowState.TimedFrameCount);

    LOG_INFO("[FApplicationRenderer]: '%s' paint over %d frames: walk %.3f ms, build %.3f ms, upload %.3f ms, total %.3f ms (%d commands, %d vertices, %d batches)",
        Window ? *Window->GetTitle() : "<closed>", WindowState.TimedFrameCount, Total.ElementWalkTime / Frames,
        Total.GeometryBuildTime / Frames, Total.BufferUploadTime / Frames, Total.GetTotalTime() / Frames,
        WindowState.TimedFrameCount > 0 ? Total.CommandCount / WindowState.TimedFrameCount : 0,
        WindowState.TimedFrameCount > 0 ? Total.VertexCount / WindowState.TimedFrameCount : 0,
        WindowState.TimedFrameCount > 0 ? Total.BatchCount / WindowState.TimedFrameCount : 0);

    WindowState.TimedFrameCount = 0;
}

void FApplicationRenderer::OnWindowDestroyed(const TSharedPtr<FWindow>& InWindow)
{
    if (!InWindow)
    {
        return;
    }

    for (int32 Index = 0; Index < WindowStates.Size(); ++Index)
    {
        if (WindowStates[Index].Window == InWindow)
        {
            RetireWindowBuffers(WindowStates[Index]);
            WindowStates.RemoveAt(Index);
            return;
        }
    }
}

FWindowDrawState* FApplicationRenderer::FindWindowState(const TSharedPtr<FWindow>& InWindow)
{
    for (FWindowDrawState& WindowState : WindowStates)
    {
        if (WindowState.Window == InWindow)
        {
            return &WindowState;
        }
    }

    return nullptr;
}

FWindowDrawState* FApplicationRenderer::FindOrAddWindowState(const TSharedPtr<FWindow>& InWindow)
{
    for (int32 Index = WindowStates.Size() - 1; Index >= 0; --Index)
    {
        FWindowDrawState& WindowState = WindowStates[Index];
        if (WindowState.Window == InWindow)
        {
            return &WindowState;
        }

        if (!WindowState.Window.IsValid())
        {
            RetireWindowBuffers(WindowState);
            WindowStates.RemoveAt(Index);
        }
    }

    FWindowDrawState& NewState = WindowStates.Emplace();
    NewState.Window = InWindow;
    return &NewState;
}

bool FApplicationRenderer::PreparePipelineState(EFormat OutputFormat)
{
    if (PipelineState && ShapePipelineState && TextPipelineState && PipelineStateFormat == OutputFormat)
    {
        return true;
    }

    FRHIGraphicsPipelineStateDesc PipelineStateDesc;
    PipelineStateDesc.VertexShader                                   = VShader.Get();
    PipelineStateDesc.PixelShader                                    = PShader.Get();
    PipelineStateDesc.InputLayout                                    = InputLayout.Get();
    PipelineStateDesc.DepthStencilState                              = DepthStencilState.Get();
    PipelineStateDesc.RasterizerState                                = RasterizerState.Get();
    PipelineStateDesc.BlendState                                     = BlendState.Get();
    PipelineStateDesc.RasterizerOutputFormats.NumRenderTargets       = 1;
    PipelineStateDesc.RasterizerOutputFormats.RenderTargetFormats[0] = OutputFormat;
    PipelineStateDesc.PrimitiveTopology                              = EPrimitiveTopology::TriangleList;

    FRHIGraphicsPipelineStateRef NewPipelineState = RHI::CreateGraphicsPipelineState(PipelineStateDesc);
    if (!NewPipelineState)
    {
        LOG_ERROR("[FApplicationRenderer]: Failed to create the pipeline state");
        return false;
    }

    PipelineStateDesc.VertexShader = ShapeVShader.Get();
    PipelineStateDesc.PixelShader  = ShapePShader.Get();
    PipelineStateDesc.InputLayout  = ShapeInputLayout.Get();

    FRHIGraphicsPipelineStateRef NewShapePipelineState = RHI::CreateGraphicsPipelineState(PipelineStateDesc);
    if (!NewShapePipelineState)
    {
        LOG_ERROR("[FApplicationRenderer]: Failed to create the shape pipeline state");
        return false;
    }

    PipelineStateDesc.VertexShader = TextVShader.Get();
    PipelineStateDesc.PixelShader  = PShader.Get();
    PipelineStateDesc.InputLayout  = TextInputLayout.Get();

    FRHIGraphicsPipelineStateRef NewTextPipelineState = RHI::CreateGraphicsPipelineState(PipelineStateDesc);
    if (!NewTextPipelineState)
    {
        LOG_ERROR("[FApplicationRenderer]: Failed to create the text pipeline state");
        return false;
    }

    PipelineState       = NewPipelineState;
    ShapePipelineState  = NewShapePipelineState;
    TextPipelineState   = NewTextPipelineState;
    PipelineStateFormat = OutputFormat;
    return true;
}

bool FApplicationRenderer::PrepareGeometry(FRHICommandList& InCommandList, FWindowDrawState& WindowState)
{
    const FUIDrawData& DrawData = WindowState.DrawData;

    WindowState.Stats.BufferUploadTime = 0.0f;

    const bool bHasTextured = DrawData.GetVertices().Size() > 0 && DrawData.GetIndices().Size() > 0;
    const bool bHasShape    = !DrawData.GetShapeInstances().IsEmpty();
    const bool bHasText     = !DrawData.GetTextGlyphInstances().IsEmpty();

    if (!bHasTextured && !bHasShape && !bHasText)
    {
        return false;
    }

    const bool bTexturedReady = !bHasTextured || (WindowState.VertexBuffer && WindowState.IndexBuffer);
    const bool bShapeReady    = !bHasShape || WindowState.ShapeVertexBuffer;
    const bool bTextReady     = !bHasText || WindowState.TextGlyphBuffer;

    const uint64 GeometryHash = DrawData.ComputeGeometryHash();
    if (GeometryHash == WindowState.UploadedGeometryHash && bTexturedReady && bShapeReady && bTextReady)
    {
        ReportPaintStats(WindowState);
        return true;
    }

    const uint64 UploadStartTime = FPlatformTime::QueryPerformanceCounter();

    if (bHasTextured)
    {
        if (!UploadStream(InCommandList, WindowState.VertexBuffer, WindowState.IndexBuffer, WindowState.VertexCapacity,
                WindowState.IndexCapacity, WindowState.IndexFormat, DrawData.GetVertices().Data(), static_cast<int32>(sizeof(FUIVertex)),
                DrawData.GetVertices().Size(), DrawData.GetIndices().Data(), DrawData.GetIndices().Size(),
                "ApplicationUI VertexBuffer", "ApplicationUI IndexBuffer"))
        {
            return false;
        }
    }

    if (bHasShape)
    {
        if (!UploadVertexStream(InCommandList, WindowState.ShapeVertexBuffer, WindowState.ShapeVertexCapacity,
                DrawData.GetShapeInstances().Data(), static_cast<int32>(sizeof(FUIShapeInstance)),
                DrawData.GetShapeInstances().Size(), "ApplicationUI ShapeInstanceBuffer"))
        {
            return false;
        }
    }

    if (bHasText)
    {
        if (!UploadVertexStream(InCommandList, WindowState.TextGlyphBuffer, WindowState.TextGlyphCapacity,
                DrawData.GetTextGlyphInstances().Data(), static_cast<int32>(sizeof(FUITextGlyphInstance)),
                DrawData.GetTextGlyphInstances().Size(), "ApplicationUI TextGlyphBuffer"))
        {
            return false;
        }
    }

    WindowState.UploadedGeometryHash = GeometryHash;
    WindowState.Stats.BufferUploadTime = ToMillisecondsSince(UploadStartTime);

    ReportPaintStats(WindowState);
    return true;
}

bool FApplicationRenderer::UploadStream(
    FRHICommandList& InCommandList, 
    FRHIBufferRef&   VertexBuffer, 
    FRHIBufferRef&   IndexBuffer,
    int32&           VertexCapacity, 
    int32&           IndexCapacity, 
    EIndexFormat&    IndexFormat, 
    const void*      Vertices, 
    int32            VertexStride, 
    int32            VertexCount,
    const uint32*    Indices, 
    int32            IndexCount, 
    const CHAR*      VertexDebugName, 
    const CHAR*      IndexDebugName)
{
    if (!VertexBuffer || VertexCount > VertexCapacity)
    {
        const int32 NewCapacity = VertexCount + GVertexGrowth;

        const FRHIBufferDesc VertexBufferDesc = FRHIBufferDesc::CreateVertexBuffer(
            static_cast<uint32>(VertexStride), static_cast<uint32>(NewCapacity), EBufferFlags::Default | EBufferFlags::CopyDest);

        FRHIBufferRef NewVertexBuffer = RHI::CreateBuffer(VertexBufferDesc, ERHIResourceState::GenericRead, nullptr);
        if (!NewVertexBuffer)
        {
            return false;
        }

        NewVertexBuffer->SetDebugName(VertexDebugName);

        if (VertexBuffer)
        {
            RetiredBuffers.Add(FRetiredBuffer{ Move(VertexBuffer), FrameCounter });
        }

        VertexBuffer   = NewVertexBuffer;
        VertexCapacity = NewCapacity;
    }

    const EIndexFormat DesiredFormat = (VertexCount <= 65535) 
        ? EIndexFormat::uint16 
        : EIndexFormat::uint32;
    
    const int32 IndexStride = (DesiredFormat == EIndexFormat::uint16) 
        ? static_cast<int32>(sizeof(uint16)) 
        : static_cast<int32>(sizeof(uint32));

    if (!IndexBuffer || IndexCount > IndexCapacity || IndexFormat != DesiredFormat)
    {
        const int32 NewCapacity = IndexCount + GIndexGrowth;

        const FRHIBufferDesc IndexBufferDesc = FRHIBufferDesc::CreateIndexBuffer(
            static_cast<uint32>(IndexStride), static_cast<uint32>(NewCapacity), EBufferFlags::Default | EBufferFlags::CopyDest);

        FRHIBufferRef NewIndexBuffer = RHI::CreateBuffer(IndexBufferDesc, ERHIResourceState::GenericRead, nullptr);
        if (!NewIndexBuffer)
        {
            return false;
        }

        NewIndexBuffer->SetDebugName(IndexDebugName);

        if (IndexBuffer)
        {
            RetiredBuffers.Add(FRetiredBuffer{ Move(IndexBuffer), FrameCounter });
        }

        IndexBuffer   = NewIndexBuffer;
        IndexCapacity = NewCapacity;
        IndexFormat   = DesiredFormat;
    }

    const FRHITransitionBarrierDesc ToCopyDest[] =
    {
        FRHITransitionBarrierDesc::CreateBuffer(VertexBuffer.Get(), ERHIResourceState::GenericRead, ERHIResourceState::CopyDest),
        FRHITransitionBarrierDesc::CreateBuffer(IndexBuffer.Get(), ERHIResourceState::GenericRead, ERHIResourceState::CopyDest),
    };

    InCommandList.TransitionBarrier(ToCopyDest);
    InCommandList.UpdateBuffer(VertexBuffer.Get(), FBufferRegion(0, VertexCount * VertexStride), Vertices);

    if (DesiredFormat == EIndexFormat::uint16)
    {
        Index16Scratch.ResizeUninitialized(IndexCount);
        ConvertIndicesToUInt16(Indices, Index16Scratch.Data(), IndexCount);

        InCommandList.UpdateBuffer(IndexBuffer.Get(), FBufferRegion(0, IndexCount * static_cast<int32>(sizeof(uint16))), Index16Scratch.Data());
    }
    else
    {
        InCommandList.UpdateBuffer(IndexBuffer.Get(), FBufferRegion(0, IndexCount * static_cast<int32>(sizeof(uint32))), Indices);
    }

    const FRHITransitionBarrierDesc ToGenericRead[] =
    {
        FRHITransitionBarrierDesc::CreateBuffer(VertexBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::GenericRead),
        FRHITransitionBarrierDesc::CreateBuffer(IndexBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::GenericRead),
    };

    InCommandList.TransitionBarrier(ToGenericRead);
    return true;
}

bool FApplicationRenderer::UploadVertexStream(
    FRHICommandList& InCommandList, 
    FRHIBufferRef&   VertexBuffer, 
    int32&           VertexCapacity,
    const void*      Vertices, 
    int32            VertexStride, 
    int32            VertexCount, 
    const CHAR*      VertexDebugName)
{
    if (!VertexBuffer || VertexCount > VertexCapacity)
    {
        const int32 NewCapacity = VertexCount + GVertexGrowth;
        const FRHIBufferDesc VertexBufferDesc = FRHIBufferDesc::CreateVertexBuffer(
            static_cast<uint32>(VertexStride), static_cast<uint32>(NewCapacity), EBufferFlags::Default | EBufferFlags::CopyDest);

        FRHIBufferRef NewVertexBuffer = RHI::CreateBuffer(VertexBufferDesc, ERHIResourceState::GenericRead, nullptr);
        if (!NewVertexBuffer)
        {
            return false;
        }

        NewVertexBuffer->SetDebugName(VertexDebugName);
        if (VertexBuffer)
        {
            RetiredBuffers.Add(FRetiredBuffer{ Move(VertexBuffer), FrameCounter });
        }

        VertexBuffer   = NewVertexBuffer;
        VertexCapacity = NewCapacity;
    }

    InCommandList.TransitionBarrier(
        FRHITransitionBarrierDesc::CreateBuffer(VertexBuffer.Get(), ERHIResourceState::GenericRead, ERHIResourceState::CopyDest));
    InCommandList.UpdateBuffer(VertexBuffer.Get(), FBufferRegion(0, VertexCount * VertexStride), Vertices);
    InCommandList.TransitionBarrier(
        FRHITransitionBarrierDesc::CreateBuffer(VertexBuffer.Get(), ERHIResourceState::CopyDest, ERHIResourceState::GenericRead));
    return true;
}

FRHIShaderResourceView* FApplicationRenderer::PrepareAtlasTexture(FRHICommandList& InCommandList, const FFontAtlas* Atlas)
{
    if (!Atlas || !Atlas->IsValid())
    {
        return GetDefaultShaderResourceView();
    }

    FAtlasEntry* Entry = AtlasTextures.Find(Atlas);
    if (!Entry || Entry->Revision != Atlas->GetRevision())
    {
        FRHITextureRef NewAtlasTexture = FTextureFactory::Get().LoadFromMemory(Atlas->GetPixels(), 
            static_cast<uint32>(Atlas->GetWidth()), static_cast<uint32>(Atlas->GetHeight()),
            ETextureFactoryFlags::None, EFormat::R8G8B8A8_Unorm);

        if (!NewAtlasTexture)
        {
            return GetDefaultShaderResourceView();
        }

        NewAtlasTexture->SetDebugName("ApplicationUI FontAtlas");

        AtlasTextures.Add(Atlas, FAtlasEntry{ NewAtlasTexture, Atlas->GetRevision() });

        Entry = AtlasTextures.Find(Atlas);
        if (!Entry)
        {
            return GetDefaultShaderResourceView();
        }
    }

    FRHITexture* EntryTexture = Entry->Texture.Get();
    if (EntryTexture->GetDesc().TrackingMode != ERHIResourceStateTrackingMode::Static)
    {
        InCommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(EntryTexture, ERHIResourceState::PixelShaderResource));
    }

    return EntryTexture->GetShaderResourceView();
}

FRHIShaderResourceView* FApplicationRenderer::PrepareBrushTexture(FRHICommandList& InCommandList, FRHITexture* Texture)
{
    if (!Texture)
    {
        return GetDefaultShaderResourceView();
    }

    if (Texture->GetDesc().TrackingMode != ERHIResourceStateTrackingMode::Static)
    {
        InCommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Texture, ERHIResourceState::PixelShaderResource));
    }

    FRHIShaderResourceView* TextureView = Texture->GetShaderResourceView();
    return TextureView ? TextureView : GetDefaultShaderResourceView();
}

void FApplicationRenderer::PrepareBatchTextures(FRHICommandList& InCommandList, const FUIDrawData& DrawData)
{
    TRACE_SCOPE("UI Prepare Textures");

    for (const FUIDrawBatch& Batch : DrawData.GetBatches())
    {
        if (Batch.Texture.Atlas)
        {
            PrepareAtlasTexture(InCommandList, Batch.Texture.Atlas);
        }
        else if (Batch.Texture.Texture)
        {
            PrepareBrushTexture(InCommandList, Batch.Texture.Texture);
        }
    }
}

FRHIShaderResourceView* FApplicationRenderer::GetBatchShaderResourceView(const FUITextureHandle& Texture) const
{
    if (Texture.Atlas)
    {
        return GetAtlasShaderResourceView(Texture.Atlas);
    }

    if (Texture.Texture)
    {
        FRHIShaderResourceView* TextureView = Texture.Texture->GetShaderResourceView();
        if (TextureView)
        {
            return TextureView;
        }
    }

    return GetDefaultShaderResourceView();
}

void FApplicationRenderer::RegisterWindowSwapChain(const TSharedPtr<FWindow>& InWindow, FRHISwapChain* SwapChain)
{
    if (!InWindow)
    {
        return;
    }

    if (FWindowDrawState* WindowState = FindOrAddWindowState(InWindow))
    {
        WindowState->SwapChain = SwapChain;
    }
}

void FApplicationRenderer::RenderWindowToSwapChain(FRHICommandList& InCommandList, const TSharedPtr<FWindow>& InWindow, EAttachmentLoadAction LoadAction)
{
    if (!InWindow)
    {
        return;
    }

    FWindowDrawState* WindowState = FindWindowState(InWindow);
    if (!WindowState || !WindowState->SwapChain)
    {
        return;
    }

    FRHISwapChain* SwapChain = WindowState->SwapChain;
    InCommandList.AcquireNextBackBuffer(SwapChain);

    if (LoadAction == EAttachmentLoadAction::Clear)
    {
        FRHITexture* BackBuffer = SwapChain->GetBackBuffer();
        InCommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(BackBuffer, ERHIResourceState::Undefined, ERHIResourceState::RenderTarget));
    }

    const bool bHasGeometry = !WindowState->DrawData.IsEmpty()
        && PrepareGeometry(InCommandList, *WindowState)
        && PreparePipelineState(SwapChain->GetDesc().ColorFormat);

    if (bHasGeometry)
    {
        PrepareBatchTextures(InCommandList, WindowState->DrawData);
    }

    const FFloatColor ClearColor = SwapChain->GetDesc().IsTransparent()
        ? FFloatColor(0.0f, 0.0f, 0.0f, 0.0f)
        : FUIStyle::GetDefault().Colors.WindowBackground;

    const FRHIRenderTargetAttachment Attachment(SwapChain->GetRenderTargetView(), LoadAction,
        EAttachmentStoreAction::Store, ClearColor);

    FRHIBeginRenderPassDesc RenderPassDesc({ Attachment }, 1);
    InCommandList.BeginRenderPass(RenderPassDesc);

    if (bHasGeometry)
    {
        RenderWindow(InCommandList, *WindowState);
    }

    InCommandList.EndRenderPass();
}

const FApplicationRenderer::FWindowSurface* FApplicationRenderer::FindWindowSurface(const TSharedPtr<FWindow>& InWindow) const
{
    for (const FWindowSurface& Surface : WindowSurfaces)
    {
        if (Surface.Window == InWindow)
        {
            return &Surface;
        }
    }

    return nullptr;
}

bool FApplicationRenderer::AddWindowSurface(const TSharedPtr<FWindow>& InWindow)
{
    const bool bIsPrimary = (PrimaryWindow == InWindow);

    const IntVector2 WindowSize = InWindow->GetSize();

    FRHISwapChainDesc SwapChainDesc;
    SwapChainDesc.WindowHandle = InWindow->GetPlatformWindow()->GetPlatformHandle();
    SwapChainDesc.Width        = static_cast<uint16>(Math::Max(WindowSize.X, 1));
    SwapChainDesc.Height       = static_cast<uint16>(Math::Max(WindowSize.Y, 1));
    SwapChainDesc.ColorFormat  = EFormat::Unknown;
    SwapChainDesc.ColorSpace   = EColorSpace::Unknown;
    SwapChainDesc.Usage        = ESwapChainUsageFlags::RenderTarget;
    SwapChainDesc.bFramePacing = bIsPrimary;

    if ((InWindow->GetStyle() & EWindowStyleFlags::Opaque) == EWindowStyleFlags::None)
    {
        SwapChainDesc.Flags |= ESwapChainFlags::Transparent;
    }

    FRHISwapChainRef SwapChain = RHI::CreateSwapChain(SwapChainDesc);
    if (!SwapChain)
    {
        LOG_ERROR("[FApplicationRenderer]: Failed to create a swap chain for a window");
        return false;
    }

    FWindowSurface Surface;
    Surface.Window            = InWindow;
    Surface.SwapChain         = SwapChain;
    Surface.Size              = WindowSize;
    Surface.DeferredShowState = InWindow->ShowOnCreate() ? EDeferredShowState::None : EDeferredShowState::AwaitingPresent;
    Surface.bIsPrimary        = bIsPrimary;

    WindowSurfaces.Add(Surface);

    RegisterWindowSwapChain(InWindow, SwapChain.Get());
    return true;
}

void FApplicationRenderer::SetPrimaryWindow(const TSharedPtr<FWindow>& InWindow)
{
    PrimaryWindow = InWindow;
    EnsureWindowSurface(InWindow);
}

void FApplicationRenderer::EnsureWindowSurface(const TSharedPtr<FWindow>& InWindow)
{
    if (InWindow && !InWindow->HasExternalSurface() && !FindWindowSurface(InWindow))
    {
        AddWindowSurface(InWindow);
    }
}

FRHISwapChainRef FApplicationRenderer::GetWindowSwapChain(const TSharedPtr<FWindow>& InWindow) const
{
    const FWindowSurface* Surface = FindWindowSurface(InWindow);
    return Surface ? Surface->SwapChain : nullptr;
}

void FApplicationRenderer::SyncWindowSurfaces()
{
    ++FrameCounter;
    ReleaseRetiredResources(false);

    const TArray<TSharedPtr<FWindow>>& Windows = FApplication::Get().GetWindows();
    for (int32 Index = WindowSurfaces.Size() - 1; Index >= 0; --Index)
    {
        if (Windows.Contains(WindowSurfaces[Index].Window))
        {
            continue;
        }

        FRHICommandListExecutor::Get().WaitForGPU();
        WindowSurfaces.RemoveAt(Index);

        ReleaseRetiredResources(true);
    }

    for (const TSharedPtr<FWindow>& CurrentWindow : Windows)
    {
        if (CurrentWindow->HasExternalSurface())
        {
            continue;
        }

        if (FindWindowSurface(CurrentWindow))
        {
            continue;
        }

        AddWindowSurface(CurrentWindow);
    }
}

void FApplicationRenderer::RenderWindowSurfaces(FRHICommandList& InCommandList)
{
    for (FWindowSurface& Surface : WindowSurfaces)
    {
        ReconcileSurfaceSize(InCommandList, Surface);
        RenderWindowToSwapChain(InCommandList, Surface.Window, EAttachmentLoadAction::Clear);

        Surface.bIsRendered = true;
    }
}

void FApplicationRenderer::PresentWindowSurfaces(FRHICommandList& InCommandList)
{
    for (FWindowSurface& Surface : WindowSurfaces)
    {
        if (!Surface.bIsRendered)
        {
            continue;
        }

        Surface.bIsRendered = false;

        FRHITexture* BackBuffer = Surface.SwapChain->GetBackBuffer();
        InCommandList.TransitionBarrier(
            FRHITransitionBarrierDesc::CreateTexture(BackBuffer, ERHIResourceState::RenderTarget, ERHIResourceState::Present));
        InCommandList.PresentSwapChain(Surface.SwapChain.Get(), Surface.bIsPrimary && GVSyncEnabled);

        switch (Surface.DeferredShowState)
        {
            case EDeferredShowState::AwaitingPresent:
            {
                Surface.DeferredShowState = EDeferredShowState::PresentRecorded;
                break;
            }

            case EDeferredShowState::PresentRecorded:
            {
                Surface.Window->Show(Surface.Window->ActivateOnShow());
                Surface.DeferredShowState = EDeferredShowState::None;
                break;
            }

            default:
            {
                break;
            }
        }
    }
}

void FApplicationRenderer::BeginFrame()
{
    CHECK_MAIN_THREAD();

    {
        TRACE_SCOPE("UI Sync Surfaces");
        SyncWindowSurfaces();
    }

    CommandList.BeginFrame();
    bHasOpenFrame = true;
}

void FApplicationRenderer::RecordWindows()
{
    CHECK_MAIN_THREAD();

    RHI_EVENT_SCOPE(CommandList, "UI Render");
    TRACE_SCOPE("Record UI");

#if SUPPORT_VARIABLE_RATE_SHADING
    if (RHISupportsVariableRateShading())
    {
        CommandList.SetShadingRate(EShadingRate::VRS_1x1);
        CommandList.SetShadingRateImage(nullptr);
    }
#endif

    {
        TRACE_SCOPE("UI Draw Windows");
        FApplication::Get().DrawWindows();
    }

    {
        TRACE_SCOPE("UI Record Surfaces");

        FScopedApplicationGPUTrace GPUTrace(GPUProfiler, CommandList, "UI Render");
        RenderWindowSurfaces(CommandList);
    }
}

void FApplicationRenderer::EndFrameAndPresent()
{
    CHECK_MAIN_THREAD();

    if (!bHasOpenFrame)
    {
        return;
    }

    bHasOpenFrame = false;

    if (GPUProfiler)
    {
        GPUProfiler->MarkGPUFrameEnd(CommandList);
    }

    {
        TRACE_SCOPE("UI Present Windows");
        PresentWindowSurfaces(CommandList);
    }

    {
        TRACE_SCOPE("UI Finalize Command List");

        CommandList.EndFrame();
        CommandList.FlushDeletedResources();
    }

    {
        TRACE_SCOPE("ExecuteCommandList");

        if (LastFrameFinishedEvent)
        {
            {
                TRACE_SCOPE("UI RHI Wait Prior Frame");
                LastFrameFinishedEvent->Wait(FTimespan::Infinity());
            }

            FPlatformEvent::Recycle(LastFrameFinishedEvent);
            LastFrameFinishedEvent = nullptr;
        }

        LastFrameFinishedEvent = FPlatformEvent::Create(false);
        if (LastFrameFinishedEvent)
        {
            CommandList.SetEvent(LastFrameFinishedEvent);
        }

        {
            TRACE_SCOPE("UI RHI Dispatch Command List");
            FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
        }
    }

    UIScreenshot::Tick();
}

void FApplicationRenderer::RedrawWindow(const TSharedPtr<FWindow>& InWindow)
{
    CHECK_MAIN_THREAD();

    FWindowSurface* Surface = InWindow ? FindRedrawableSurface(InWindow) : nullptr;
    if (!Surface)
    {
        return;
    }

    TRACE_SCOPE("Redraw Window");

    FApplication::Get().DrawWindow(InWindow);

    ResizeCommandList.BeginFrame();

    {
        RHI_EVENT_SCOPE(ResizeCommandList, "Resize Redraw");

        RedrawWindowSurface(ResizeCommandList, *Surface);
    }

    ResizeCommandList.EndFrame();
    ResizeCommandList.FlushDeletedResources();

    FRHICommandListExecutor::Get().ExecuteCommandList(ResizeCommandList);
    FRHICommandListExecutor::Get().WaitForGPU();
}

FApplicationRenderer::FWindowSurface* FApplicationRenderer::FindRedrawableSurface(const TSharedPtr<FWindow>& InWindow)
{
    const int32 Index = WindowSurfaces.FindWithPredicate([&InWindow](const FWindowSurface& Surface)
    {
        return Surface.Window == InWindow && Surface.DeferredShowState == EDeferredShowState::None;
    });

    return Index != TArray<FWindowSurface>::InvalidIndex ? &WindowSurfaces[Index] : nullptr;
}

void FApplicationRenderer::RedrawWindowSurface(FRHICommandList& InCommandList, FWindowSurface& Surface)
{
    ReconcileSurfaceSize(InCommandList, Surface);
    RenderWindowToSwapChain(InCommandList, Surface.Window, EAttachmentLoadAction::Clear);

    FRHITexture* BackBuffer = Surface.SwapChain->GetBackBuffer();
    InCommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(BackBuffer, ERHIResourceState::RenderTarget, ERHIResourceState::Present));
    InCommandList.PresentSwapChain(Surface.SwapChain.Get(), false);
}

void FApplicationRenderer::ReconcileSurfaceSize(FRHICommandList& InCommandList, FWindowSurface& Surface)
{
    const IntVector2 WindowSize = Surface.Window->GetSize();
    if (WindowSize != Surface.Size && WindowSize.X > 0 && WindowSize.Y > 0)
    {
        Surface.Size = WindowSize;
        InCommandList.ResizeSwapChain(Surface.SwapChain.Get(), static_cast<uint32>(WindowSize.X), static_cast<uint32>(WindowSize.Y));
    }
}

void FApplicationRenderer::ReleaseWindowSurfaces()
{
    for (const FWindowSurface& Surface : WindowSurfaces)
    {
        RegisterWindowSwapChain(Surface.Window, nullptr);
    }

    WindowSurfaces.Clear();
    PrimaryWindow.Reset();
}

void FApplicationRenderer::RetireWindowBuffers(FWindowDrawState& WindowState)
{
    if (WindowState.VertexBuffer)
    {
        RetiredBuffers.Add(FRetiredBuffer{ Move(WindowState.VertexBuffer), FrameCounter });
    }

    if (WindowState.IndexBuffer)
    {
        RetiredBuffers.Add(FRetiredBuffer{ Move(WindowState.IndexBuffer), FrameCounter });
    }

    if (WindowState.ShapeVertexBuffer)
    {
        RetiredBuffers.Add(FRetiredBuffer{ Move(WindowState.ShapeVertexBuffer), FrameCounter });
    }

    if (WindowState.ShapeIndexBuffer)
    {
        RetiredBuffers.Add(FRetiredBuffer{ Move(WindowState.ShapeIndexBuffer), FrameCounter });
    }

    if (WindowState.TextGlyphBuffer)
    {
        RetiredBuffers.Add(FRetiredBuffer{ Move(WindowState.TextGlyphBuffer), FrameCounter });
    }

    WindowState.VertexCapacity       = 0;
    WindowState.IndexCapacity        = 0;
    WindowState.ShapeVertexCapacity  = 0;
    WindowState.ShapeIndexCapacity   = 0;
    WindowState.TextGlyphCapacity    = 0;
    WindowState.UploadedGeometryHash = 0;
}

void FApplicationRenderer::ReleaseRetiredResources(bool bReleaseEverything)
{
    if (bReleaseEverything)
    {
        RetiredBuffers.Clear();
        RetiredTextures.Clear();
        return;
    }

    for (int32 Index = RetiredBuffers.Size() - 1; Index >= 0; --Index)
    {
        if ((FrameCounter - RetiredBuffers[Index].Frame) >= NumRetiredFrames)
        {
            RetiredBuffers.RemoveAt(Index);
        }
    }

    for (int32 Index = RetiredTextures.Size() - 1; Index >= 0; --Index)
    {
        if ((FrameCounter - RetiredTextures[Index].Frame) >= NumRetiredFrames)
        {
            RetiredTextures.RemoveAt(Index);
        }
    }
}

void FApplicationRenderer::RetireTexture(const FRHITextureRef& Texture)
{
    if (Texture)
    {
        RetiredTextures.Add(FRetiredTexture{ Texture, FrameCounter });
    }
}

void FApplicationRenderer::RenderWindow(FRHICommandList& InCommandList, const FWindowDrawState& WindowState)
{
    TWeakPtr<FWindow>   WeakWindow = WindowState.Window;
    TSharedPtr<FWindow> Window     = WeakWindow.ToSharedPtr();

    if (!Window)
    {
        return;
    }

    const IntVector2 WindowSize = Window->GetSize();
    if (WindowSize.X <= 0 || WindowSize.Y <= 0)
    {
        return;
    }

    RenderDrawData(InCommandList, WindowState, WindowSize, 1.0f);
}

void FApplicationRenderer::RenderDrawData(FRHICommandList& InCommandList, const FWindowDrawState& WindowState, const IntVector2& GeometrySize, float SupersampleScale)
{
    const float GeometryWidth  = static_cast<float>(GeometrySize.X);
    const float GeometryHeight = static_cast<float>(GeometrySize.Y);
    const float FramebufferW   = GeometryWidth * SupersampleScale;
    const float FramebufferH   = GeometryHeight * SupersampleScale;

    const float Matrix[4][4] =
    {
        { 2.0f / GeometryWidth,  0.0f,                  0.0f, 0.0f },
        { 0.0f,                 -2.0f / GeometryHeight, 0.0f, 0.0f },
        { 0.0f,                 0.0f,                 0.5f, 0.0f },
        { -1.0f,                1.0f,                 0.5f, 1.0f },
    };

    FApplicationUIConstants Constants;
    Memory::Memcpy(&Constants.ProjectionMatrix, Matrix, sizeof(Matrix));

    InCommandList.SetViewport(FViewportRegion(FramebufferW, FramebufferH, 0.0f, 0.0f, 0.0f, 1.0f));
    InCommandList.SetBlendFactor(Vector4{ 0.0f, 0.0f, 0.0f, 0.0f });

    EUIDrawBatchKind        BoundKind     = EUIDrawBatchKind::Textured;
    FRHIShaderResourceView* BoundView     = nullptr;
    bool                    bScissorValid = false;
    float                   BoundScissorX = -1.0f;
    float                   BoundScissorY = -1.0f;
    float                   BoundScissorW = -1.0f;
    float                   BoundScissorH = -1.0f;
    bool                    bHasBoundKind = false;

    for (const FUIDrawBatch& Batch : WindowState.DrawData.GetBatches())
    {
        if (Batch.IndexCount <= 0)
        {
            continue;
        }

        float ScissorX      = 0.0f;
        float ScissorY      = 0.0f;
        float ScissorWidth  = FramebufferW;
        float ScissorHeight = FramebufferH;

        if (Batch.bIsClipped)
        {
            const float MinX = Math::Max(0.0f, static_cast<float>(Batch.ScissorRectangle.Position.X) * SupersampleScale);
            const float MinY = Math::Max(0.0f, static_cast<float>(Batch.ScissorRectangle.Position.Y) * SupersampleScale);
            const float MaxX = Math::Min(FramebufferW, static_cast<float>(Batch.ScissorRectangle.GetRight()) * SupersampleScale);
            const float MaxY = Math::Min(FramebufferH, static_cast<float>(Batch.ScissorRectangle.GetBottom()) * SupersampleScale);

            if (MaxX <= MinX || MaxY <= MinY)
            {
                continue;
            }

            ScissorX      = MinX;
            ScissorY      = MinY;
            ScissorWidth  = MaxX - MinX;
            ScissorHeight = MaxY - MinY;
        }

        if (!bHasBoundKind || BoundKind != Batch.Kind)
        {
            if (Batch.Kind == EUIDrawBatchKind::Shape)
            {
                if (!WindowState.ShapeVertexBuffer)
                {
                    continue;
                }

                InCommandList.SetGraphicsPipelineState(ShapePipelineState.Get());
                InCommandList.SetVertexBuffers(MakeArrayView(&WindowState.ShapeVertexBuffer, 1), 0);
                InCommandList.SetShaderConstants(ShapePShader.Get(), &Constants, 16);
            }
            else if (Batch.Kind == EUIDrawBatchKind::Text)
            {
                if (!WindowState.TextGlyphBuffer)
                {
                    continue;
                }

                InCommandList.SetGraphicsPipelineState(TextPipelineState.Get());
                InCommandList.SetVertexBuffers(MakeArrayView(&WindowState.TextGlyphBuffer, 1), 0);
                InCommandList.SetSamplerState(PShader.Get(), LinearSampler.Get(), 0);
                InCommandList.SetShaderConstants(PShader.Get(), &Constants, 16);
            }
            else
            {
                if (!WindowState.VertexBuffer || !WindowState.IndexBuffer)
                {
                    continue;
                }

                InCommandList.SetGraphicsPipelineState(PipelineState.Get());
                InCommandList.SetVertexBuffers(MakeArrayView(&WindowState.VertexBuffer, 1), 0);
                InCommandList.SetIndexBuffer(WindowState.IndexBuffer.Get(), WindowState.IndexFormat);
                InCommandList.SetSamplerState(PShader.Get(), LinearSampler.Get(), 0);
                InCommandList.SetShaderConstants(PShader.Get(), &Constants, 16);
            }

            BoundKind     = Batch.Kind;
            bHasBoundKind = true;
            BoundView     = nullptr;
        }

        if (Batch.Kind != EUIDrawBatchKind::Shape)
        {
            FRHIShaderResourceView* TextureView = GetBatchShaderResourceView(Batch.Texture);
            if (!TextureView)
            {
                continue;
            }

            if (TextureView != BoundView)
            {
                InCommandList.SetShaderResourceView(PShader.Get(), TextureView, 0);
                BoundView = TextureView;
            }
        }

        if (!bScissorValid || ScissorX != BoundScissorX || ScissorY != BoundScissorY || ScissorWidth != BoundScissorW
            || ScissorHeight != BoundScissorH)
        {
            InCommandList.SetScissorRect(FScissorRegion(ScissorWidth, ScissorHeight, ScissorX, ScissorY));
            BoundScissorX = ScissorX;
            BoundScissorY = ScissorY;
            BoundScissorW = ScissorWidth;
            BoundScissorH = ScissorHeight;
            bScissorValid = true;
        }

        if (Batch.Kind == EUIDrawBatchKind::Text)
        {
            InCommandList.DrawInstanced(6, static_cast<uint32>(Batch.IndexCount), 0, static_cast<uint32>(Batch.IndexOffset));
        }
        else if (Batch.Kind == EUIDrawBatchKind::Shape)
        {
            InCommandList.DrawInstanced(6, static_cast<uint32>(Batch.IndexCount / 6), 0, static_cast<uint32>(Batch.IndexOffset / 6));
        }
        else
        {
            InCommandList.DrawIndexedInstanced(static_cast<uint32>(Batch.IndexCount), 1, static_cast<uint32>(Batch.IndexOffset), 0, 0);
        }
    }
}

FRHITextureRef FApplicationRenderer::RenderElementToTexture(const TSharedPtr<FVisualElement>& Element, const IntVector2& Size, float DPIScale)
{
    if (!Element || Size.X <= 0 || Size.Y <= 0)
    {
        return nullptr;
    }

    const float ClampedScale = Math::Max(1.0f, DPIScale);

    FWindowDrawState SnapshotState;
    Element->OnDraw(FDrawGeometry(FRectangle(IntVector2(0, 0), Size.X, Size.Y), ClampedScale), SnapshotState.Commands, 0);

    SnapshotState.DrawData.BuildFromCommandList(SnapshotState.Commands);

    if (SnapshotState.DrawData.IsEmpty())
    {
        return nullptr;
    }

    const ETextureUsageFlags UsageFlags = 
        ETextureUsageFlags::RenderTarget |
        ETextureUsageFlags::ShaderResourceTexture |
        ETextureUsageFlags::CopySource;

    const FClearValue ClearValue(GSnapshotFormat, 0.0f, 0.0f, 0.0f, 0.0f);

    const FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(GSnapshotFormat,
        static_cast<uint32>(static_cast<float>(Size.X) * ClampedScale),
        static_cast<uint32>(static_cast<float>(Size.Y) * ClampedScale),
        1, 1, UsageFlags, ClearValue, ERHIResourceStateTrackingMode::Tracked);

    FRHITextureRef Texture = RHI::CreateTexture(TextureDesc, ERHIResourceState::RenderTarget);
    if (!Texture)
    {
        LOG_ERROR("[FApplicationRenderer]: Failed to create a %d x %d texture to snapshot an element into", Size.X, Size.Y);
        return nullptr;
    }

    Texture->SetDebugName("ApplicationUI ElementSnapshot");

    FRHIRenderTargetViewRef RenderTargetView = RHI::CreateRenderTargetView(Texture.Get(),
        FRHIRenderTargetViewDesc::CreateTexture2D(GSnapshotFormat, 0));

    if (!RenderTargetView)
    {
        return nullptr;
    }

    FRHICommandList SnapshotCommandList;

    if (!PrepareGeometry(SnapshotCommandList, SnapshotState) || !PreparePipelineState(GSnapshotFormat))
    {
        return nullptr;
    }

    PrepareBatchTextures(SnapshotCommandList, SnapshotState.DrawData);

    const FRHIRenderTargetAttachment Attachment(RenderTargetView.Get(), EAttachmentLoadAction::Clear,
        EAttachmentStoreAction::Store, FFloatColor(0.0f, 0.0f, 0.0f, 0.0f));

    FRHIBeginRenderPassDesc RenderPassDesc({ Attachment }, 1);
    SnapshotCommandList.BeginRenderPass(RenderPassDesc);

    RenderDrawData(SnapshotCommandList, SnapshotState, Size, ClampedScale);

    SnapshotCommandList.EndRenderPass();
    SnapshotCommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Texture.Get(), ERHIResourceState::PixelShaderResource));

    FRHICommandListExecutor::Get().ExecuteCommandList(SnapshotCommandList);

    RetireWindowBuffers(SnapshotState);
    return Texture;
}

FRHIShaderResourceView* FApplicationRenderer::GetDefaultShaderResourceView() const
{
    return DefaultTexture ? DefaultTexture->GetShaderResourceView() : nullptr;
}

FRHIShaderResourceView* FApplicationRenderer::GetAtlasShaderResourceView(const FFontAtlas* Atlas) const
{
    if (const FAtlasEntry* Entry = AtlasTextures.Find(Atlas))
    {
        return Entry->Texture->GetShaderResourceView();
    }

    return GetDefaultShaderResourceView();
}
