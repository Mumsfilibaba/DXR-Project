#include "DebugUI.h"

#include "Debug/Profiler.h"

#include "Core/Engine/Engine.h"
#include "Core/Application/Generic/GenericCursor.h"
#include "Core/Application/Platform/Platform.h"

#include "Time/Timer.h"

#include "Rendering/Resources/TextureFactory.h"
#include "Rendering/Renderer.h"

#include "RenderLayer/Resources.h"
#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

#include "Core/Containers/Array.h"

struct ImGuiState
{
    void Reset()
    {
        FontTexture.Reset();
        PipelineState.Reset();
        PipelineStateNoBlending.Reset();
        VertexBuffer.Reset();
        IndexBuffer.Reset();
        PointSampler.Reset();
        PShader.Reset();
    }

    Timer FrameClock;

    TRef<Texture2D>             FontTexture;
    TRef<GraphicsPipelineState> PipelineState;
    TRef<GraphicsPipelineState> PipelineStateNoBlending;
    TRef<PixelShader>           PShader;
    TRef<VertexBuffer>          VertexBuffer;
    TRef<IndexBuffer>           IndexBuffer;
    TRef<SamplerState>          PointSampler;
    TArray<ImGuiImage*>         Images;
    
    ImGuiContext* Context = nullptr;
};

static ImGuiState GlobalImGuiState;

static uint32 GetMouseButtonIndex(EMouseButton Button)
{
    switch (Button)
    {
    case MouseButton_Left:    return 0;
    case MouseButton_Right:   return 1;
    case MouseButton_Middle:  return 2;
    case MouseButton_Back:    return 3;
    case MouseButton_Forward: return 4;
    default:                  return static_cast<uint32>(-1);
    }
}

static TArray<DebugUI::UIDrawFunc> GlobalDrawFuncs;
static TArray<std::string>         GlobalDebugStrings;

bool DebugUI::Init()
{
    // Create context
    IMGUI_CHECKVERSION();

    GlobalImGuiState.Context = ImGui::CreateContext();
    if (!GlobalImGuiState.Context)
    {
        return false;
    }

    ImGuiIO& IO = ImGui::GetIO();
    IO.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    IO.BackendFlags |= ImGuiBackendFlags_HasSetMousePos; 
    IO.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    IO.BackendPlatformName = "Windows";
    IO.ImeWindowHandle     = GEngine.MainWindow->GetNativeHandle();

    // Keyboard mapping. ImGui will use those indices to peek into the IO.KeysDown[] array that we will update during the application lifetime.
    IO.KeyMap[ImGuiKey_Tab]         = EKey::Key_Tab;
    IO.KeyMap[ImGuiKey_LeftArrow]   = EKey::Key_Left;
    IO.KeyMap[ImGuiKey_RightArrow]  = EKey::Key_Right;
    IO.KeyMap[ImGuiKey_UpArrow]     = EKey::Key_Up;
    IO.KeyMap[ImGuiKey_DownArrow]   = EKey::Key_Down;
    IO.KeyMap[ImGuiKey_PageUp]      = EKey::Key_PageUp;
    IO.KeyMap[ImGuiKey_PageDown]    = EKey::Key_PageDown;
    IO.KeyMap[ImGuiKey_Home]        = EKey::Key_Home;
    IO.KeyMap[ImGuiKey_End]         = EKey::Key_End;
    IO.KeyMap[ImGuiKey_Insert]      = EKey::Key_Insert;
    IO.KeyMap[ImGuiKey_Delete]      = EKey::Key_Delete;
    IO.KeyMap[ImGuiKey_Backspace]   = EKey::Key_Backspace;
    IO.KeyMap[ImGuiKey_Space]       = EKey::Key_Space;
    IO.KeyMap[ImGuiKey_Enter]       = EKey::Key_Enter;
    IO.KeyMap[ImGuiKey_Escape]      = EKey::Key_Escape;
    IO.KeyMap[ImGuiKey_KeyPadEnter] = EKey::Key_KeypadEnter;
    IO.KeyMap[ImGuiKey_A]           = EKey::Key_A;
    IO.KeyMap[ImGuiKey_C]           = EKey::Key_C;
    IO.KeyMap[ImGuiKey_V]           = EKey::Key_V;
    IO.KeyMap[ImGuiKey_X]           = EKey::Key_X;
    IO.KeyMap[ImGuiKey_Y]           = EKey::Key_Y;
    IO.KeyMap[ImGuiKey_Z]           = EKey::Key_Z;

    // Setup style
    ImGui::StyleColorsDark();

    ImGuiStyle& Style = ImGui::GetStyle();
    Style.WindowRounding    = 0.0f;
    Style.FrameRounding     = 0.0f;
    Style.GrabRounding      = 0.0f;
    Style.TabRounding       = 0.0f;
    Style.WindowBorderSize  = 0.0f;
    Style.ScrollbarRounding = 0.0f;
    Style.ScrollbarSize     = 12.0f;

    Style.Colors[ImGuiCol_WindowBg].x = 0.2f;
    Style.Colors[ImGuiCol_WindowBg].y = 0.2f;
    Style.Colors[ImGuiCol_WindowBg].z = 0.2f;
    Style.Colors[ImGuiCol_WindowBg].w = 0.9f;

    Style.Colors[ImGuiCol_Text].x = 1.0f;
    Style.Colors[ImGuiCol_Text].y = 1.0f;
    Style.Colors[ImGuiCol_Text].z = 1.0f;
    Style.Colors[ImGuiCol_Text].w = 1.0f;

    Style.Colors[ImGuiCol_PlotHistogram].x = 0.9f;
    Style.Colors[ImGuiCol_PlotHistogram].y = 0.9f;
    Style.Colors[ImGuiCol_PlotHistogram].z = 0.9f;
    Style.Colors[ImGuiCol_PlotHistogram].w = 1.0f;

    Style.Colors[ImGuiCol_PlotHistogramHovered].x = 0.75f;
    Style.Colors[ImGuiCol_PlotHistogramHovered].y = 0.75f;
    Style.Colors[ImGuiCol_PlotHistogramHovered].z = 0.75f;
    Style.Colors[ImGuiCol_PlotHistogramHovered].w = 1.0f;

    Style.Colors[ImGuiCol_TitleBg].x = 0.3f;
    Style.Colors[ImGuiCol_TitleBg].y = 0.3f;
    Style.Colors[ImGuiCol_TitleBg].z = 0.3f;
    Style.Colors[ImGuiCol_TitleBg].w = 1.0f;

    Style.Colors[ImGuiCol_TitleBgActive].x = 0.15f;
    Style.Colors[ImGuiCol_TitleBgActive].y = 0.15f;
    Style.Colors[ImGuiCol_TitleBgActive].z = 0.15f;
    Style.Colors[ImGuiCol_TitleBgActive].w = 1.0f;

    Style.Colors[ImGuiCol_FrameBg].x = 0.4f;
    Style.Colors[ImGuiCol_FrameBg].y = 0.4f;
    Style.Colors[ImGuiCol_FrameBg].z = 0.4f;
    Style.Colors[ImGuiCol_FrameBg].w = 1.0f;

    Style.Colors[ImGuiCol_FrameBgHovered].x = 0.3f;
    Style.Colors[ImGuiCol_FrameBgHovered].y = 0.3f;
    Style.Colors[ImGuiCol_FrameBgHovered].z = 0.3f;
    Style.Colors[ImGuiCol_FrameBgHovered].w = 1.0f;

    Style.Colors[ImGuiCol_FrameBgActive].x = 0.24f;
    Style.Colors[ImGuiCol_FrameBgActive].y = 0.24f;
    Style.Colors[ImGuiCol_FrameBgActive].z = 0.24f;
    Style.Colors[ImGuiCol_FrameBgActive].w = 1.0f;

    Style.Colors[ImGuiCol_Button].x = 0.4f;
    Style.Colors[ImGuiCol_Button].y = 0.4f;
    Style.Colors[ImGuiCol_Button].z = 0.4f;
    Style.Colors[ImGuiCol_Button].w = 1.0f;

    Style.Colors[ImGuiCol_ButtonHovered].x = 0.3f;
    Style.Colors[ImGuiCol_ButtonHovered].y = 0.3f;
    Style.Colors[ImGuiCol_ButtonHovered].z = 0.3f;
    Style.Colors[ImGuiCol_ButtonHovered].w = 1.0f;

    Style.Colors[ImGuiCol_ButtonActive].x = 0.25f;
    Style.Colors[ImGuiCol_ButtonActive].y = 0.25f;
    Style.Colors[ImGuiCol_ButtonActive].z = 0.25f;
    Style.Colors[ImGuiCol_ButtonActive].w = 1.0f;

    Style.Colors[ImGuiCol_CheckMark].x = 0.15f;
    Style.Colors[ImGuiCol_CheckMark].y = 0.15f;
    Style.Colors[ImGuiCol_CheckMark].z = 0.15f;
    Style.Colors[ImGuiCol_CheckMark].w = 1.0f;

    Style.Colors[ImGuiCol_SliderGrab].x = 0.15f;
    Style.Colors[ImGuiCol_SliderGrab].y = 0.15f;
    Style.Colors[ImGuiCol_SliderGrab].z = 0.15f;
    Style.Colors[ImGuiCol_SliderGrab].w = 1.0f;

    Style.Colors[ImGuiCol_SliderGrabActive].x = 0.16f;
    Style.Colors[ImGuiCol_SliderGrabActive].y = 0.16f;
    Style.Colors[ImGuiCol_SliderGrabActive].z = 0.16f;
    Style.Colors[ImGuiCol_SliderGrabActive].w = 1.0f;

    Style.Colors[ImGuiCol_ResizeGrip].x = 0.25f;
    Style.Colors[ImGuiCol_ResizeGrip].y = 0.25f;
    Style.Colors[ImGuiCol_ResizeGrip].z = 0.25f;
    Style.Colors[ImGuiCol_ResizeGrip].w = 1.0f;

    Style.Colors[ImGuiCol_ResizeGripHovered].x = 0.35f;
    Style.Colors[ImGuiCol_ResizeGripHovered].y = 0.35f;
    Style.Colors[ImGuiCol_ResizeGripHovered].z = 0.35f;
    Style.Colors[ImGuiCol_ResizeGripHovered].w = 1.0f;

    Style.Colors[ImGuiCol_ResizeGripActive].x = 0.5f;
    Style.Colors[ImGuiCol_ResizeGripActive].y = 0.5f;
    Style.Colors[ImGuiCol_ResizeGripActive].z = 0.5f;
    Style.Colors[ImGuiCol_ResizeGripActive].w = 1.0f;

    Style.Colors[ImGuiCol_Tab].x = 0.55f;
    Style.Colors[ImGuiCol_Tab].y = 0.55f;
    Style.Colors[ImGuiCol_Tab].z = 0.55f;
    Style.Colors[ImGuiCol_Tab].w = 1.0f;

    Style.Colors[ImGuiCol_TabHovered].x = 0.4f;
    Style.Colors[ImGuiCol_TabHovered].y = 0.4f;
    Style.Colors[ImGuiCol_TabHovered].z = 0.4f;
    Style.Colors[ImGuiCol_TabHovered].w = 1.0f;

    Style.Colors[ImGuiCol_TabActive].x = 0.25f;
    Style.Colors[ImGuiCol_TabActive].y = 0.25f;
    Style.Colors[ImGuiCol_TabActive].z = 0.25f;
    Style.Colors[ImGuiCol_TabActive].w = 1.0f;

    // Build texture atlas
    uint8* Pixels = nullptr;
    int32 Width  = 0;
    int32 Height = 0;
    IO.Fonts->GetTexDataAsRGBA32(&Pixels, &Width, &Height);

    GlobalImGuiState.FontTexture = TextureFactory::LoadFromMemory(Pixels, Width, Height, 0, EFormat::R8G8B8A8_Unorm);
    if (!GlobalImGuiState.FontTexture)
    {
        return false;
    }

    static const char* VSSource = 
    R"*(
    cbuffer VertexBuffer : register(b0, space1)
    {
        float4x4 ProjectionMatrix;
    };
    struct VS_INPUT
    {
        float2 Position : POSITION;
        float4 Color    : COLOR0;
        float2 TexCoord : TEXCOORD0;
    };
    struct PS_INPUT
    {
        float4 Position : SV_POSITION;
        float4 Color    : COLOR0;
        float2 TexCoord : TEXCOORD0;
    };
    PS_INPUT Main(VS_INPUT Input)
    {
        PS_INPUT Output;
        Output.Position = mul(ProjectionMatrix, float4(Input.Position.xy, 0.0f, 1.0f));
        Output.Color    = Input.Color;
        Output.TexCoord = Input.TexCoord;
        return Output;
    })*";

    TArray<uint8> ShaderCode;
    if (!ShaderCompiler::CompileShader(VSSource, "Main", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    TRef<VertexShader> VShader = CreateVertexShader(ShaderCode);
    if (!VShader)
    {
        Debug::DebugBreak();
        return false;
    }

    static const char* PSSource = 
        R"*(
        struct PS_INPUT
        {
            float4 Position : SV_POSITION;
            float4 Color    : COLOR0;
            float2 TexCoord : TEXCOORD0;
        };
        SamplerState Sampler0 : register(s0);
        Texture2D    Texture0 : register(t0);
        float4 Main(PS_INPUT Input) : SV_Target
        {
            float4 OutColor = Input.Color * Texture0.Sample(Sampler0, Input.TexCoord);
            return OutColor;
        })*";

    if (!ShaderCompiler::CompileShader(PSSource, "Main", nullptr, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    GlobalImGuiState.PShader = CreatePixelShader(ShaderCode);
    if (!GlobalImGuiState.PShader)
    {
        Debug::DebugBreak();
        return false;
    }

    InputLayoutStateCreateInfo InputLayoutInfo =
    {
        { "POSITION", 0, EFormat::R32G32_Float,   0, static_cast<UINT>(IM_OFFSETOF(ImDrawVert, pos)), EInputClassification::Vertex, 0 },
        { "TEXCOORD", 0, EFormat::R32G32_Float,   0, static_cast<UINT>(IM_OFFSETOF(ImDrawVert, uv)),  EInputClassification::Vertex, 0 },
        { "COLOR",    0, EFormat::R8G8B8A8_Unorm, 0, static_cast<UINT>(IM_OFFSETOF(ImDrawVert, col)), EInputClassification::Vertex, 0 },
    };

    TRef<InputLayoutState> InputLayout = CreateInputLayout(InputLayoutInfo);
    if (!InputLayout)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        InputLayout->SetName("ImGui InputLayoutState");
    }

    DepthStencilStateCreateInfo DepthStencilStateInfo;
    DepthStencilStateInfo.DepthEnable    = false;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::Zero;

    TRef<DepthStencilState> DepthStencilState = CreateDepthStencilState(DepthStencilStateInfo);
    if (!DepthStencilState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        DepthStencilState->SetName("ImGui DepthStencilState");
    }

    RasterizerStateCreateInfo RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::None;

    TRef<RasterizerState> RasterizerState = CreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RasterizerState->SetName("ImGui RasterizerState");
    }

    BlendStateCreateInfo BlendStateInfo;
    BlendStateInfo.IndependentBlendEnable         = false;
    BlendStateInfo.RenderTarget[0].BlendEnable    = true;
    BlendStateInfo.RenderTarget[0].SrcBlend       = EBlend::SrcAlpha;
    BlendStateInfo.RenderTarget[0].SrcBlendAlpha  = EBlend::InvSrcAlpha;
    BlendStateInfo.RenderTarget[0].DestBlend      = EBlend::InvSrcAlpha;
    BlendStateInfo.RenderTarget[0].DestBlendAlpha = EBlend::Zero;
    BlendStateInfo.RenderTarget[0].BlendOpAlpha   = EBlendOp::Add;
    BlendStateInfo.RenderTarget[0].BlendOp        = EBlendOp::Add;

    TRef<BlendState> BlendStateBlending = CreateBlendState(BlendStateInfo);
    if (!BlendStateBlending)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        BlendStateBlending->SetName("ImGui BlendState");
    }

    BlendStateInfo.RenderTarget[0].BlendEnable = false;

    TRef<BlendState> BlendStateNoBlending = CreateBlendState(BlendStateInfo);
    if (!BlendStateBlending)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        BlendStateBlending->SetName("ImGui BlendState No Blending");
    }

    GraphicsPipelineStateCreateInfo PSOProperties;
    PSOProperties.ShaderState.VertexShader               = VShader.Get();
    PSOProperties.ShaderState.PixelShader                = GlobalImGuiState.PShader.Get();
    PSOProperties.InputLayoutState                       = InputLayout.Get();
    PSOProperties.DepthStencilState                      = DepthStencilState.Get();
    PSOProperties.BlendState                             = BlendStateBlending.Get();
    PSOProperties.RasterizerState                        = RasterizerState.Get();
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = EFormat::R8G8B8A8_Unorm;
    PSOProperties.PipelineFormats.NumRenderTargets       = 1;
    PSOProperties.PrimitiveTopologyType                  = EPrimitiveTopologyType::Triangle;

    GlobalImGuiState.PipelineState = CreateGraphicsPipelineState(PSOProperties);
    if (!GlobalImGuiState.PipelineState)
    {
        Debug::DebugBreak();
        return false;
    }

    PSOProperties.BlendState = BlendStateNoBlending.Get();

    GlobalImGuiState.PipelineStateNoBlending = CreateGraphicsPipelineState(PSOProperties);
    if (!GlobalImGuiState.PipelineStateNoBlending)
    {
        Debug::DebugBreak();
        return false;
    }

    GlobalImGuiState.VertexBuffer = CreateVertexBuffer<ImDrawVert>(1024 * 1024, BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr);
    if (!GlobalImGuiState.VertexBuffer)
    {
        return false;
    }
    else
    {
        GlobalImGuiState.VertexBuffer->SetName("ImGui VertexBuffer");
    }

    GlobalImGuiState.IndexBuffer = CreateIndexBuffer(
        sizeof(ImDrawIdx) == 2 ? EIndexFormat::uint16 : EIndexFormat::uint32, 
        1024 * 1024, 
        BufferFlag_Default,
        EResourceState::Common, 
        nullptr);
    if (!GlobalImGuiState.IndexBuffer)
    {
        return false;
    }
    else
    {
        GlobalImGuiState.IndexBuffer->SetName("ImGui IndexBuffer");
    }

    SamplerStateCreateInfo CreateInfo;
    CreateInfo.AddressU = ESamplerMode::Clamp;
    CreateInfo.AddressV = ESamplerMode::Clamp;
    CreateInfo.AddressW = ESamplerMode::Clamp;
    CreateInfo.Filter   = ESamplerFilter::MinMagMipPoint;

    GlobalImGuiState.PointSampler = CreateSamplerState(CreateInfo);
    if (!GlobalImGuiState.PointSampler)
    {
        return false;
    }

    GEngine.OnKeyPressedEvent.AddFunction(DebugUI::OnKeyPressed);
    GEngine.OnKeyReleasedEvent.AddFunction(DebugUI::OnKeyReleased);
    GEngine.OnKeyTypedEvent.AddFunction(DebugUI::OnKeyTyped);

    GEngine.OnMousePressedEvent.AddFunction(DebugUI::OnMousePressed);
    GEngine.OnMouseReleasedEvent.AddFunction(DebugUI::OnMouseReleased);
    GEngine.OnMouseScrolledEvent.AddFunction(DebugUI::OnMouseScrolled);

    return true;
}

void DebugUI::Release()
{
    GlobalImGuiState.Reset();

    ImGui::DestroyContext(GlobalImGuiState.Context);
}

void DebugUI::DrawUI(UIDrawFunc DrawFunc)
{
    GlobalDrawFuncs.EmplaceBack(DrawFunc);
}

void DebugUI::DrawDebugString(const std::string& DebugString)
{
    GlobalDebugStrings.EmplaceBack(DebugString);
}

void DebugUI::OnKeyPressed(const KeyPressedEvent& Event)
{
    ImGuiIO& IO = ImGui::GetIO();
    IO.KeysDown[Event.Key] = true;
}

void DebugUI::OnKeyReleased(const KeyReleasedEvent& Event)
{
    ImGuiIO& IO = ImGui::GetIO();
    IO.KeysDown[Event.Key] = false;
}

void DebugUI::OnKeyTyped(const KeyTypedEvent& Event)
{
    ImGuiIO& IO = ImGui::GetIO();
    IO.AddInputCharacter(Event.Character);
}

void DebugUI::OnMousePressed(const MousePressedEvent& Event)
{
    ImGuiIO& IO = ImGui::GetIO();
    const uint32 ButtonIndex  = GetMouseButtonIndex(Event.Button);
    IO.MouseDown[ButtonIndex] = true;
}

void DebugUI::OnMouseReleased(const MouseReleasedEvent& Event)
{
    ImGuiIO& IO = ImGui::GetIO();
    const uint32 ButtonIndex  = GetMouseButtonIndex(Event.Button);
    IO.MouseDown[ButtonIndex] = false;
}

void DebugUI::OnMouseScrolled(const MouseScrolledEvent& Event)
{
    ImGuiIO& IO = ImGui::GetIO();
    IO.MouseWheel  += Event.VerticalDelta;
    IO.MouseWheelH += Event.HorizontalDelta;
}

void DebugUI::Render(CommandList& CmdList)
{
    GlobalImGuiState.FrameClock.Tick();

    ImGuiIO& IO = ImGui::GetIO();

    GenericWindow* Window = GEngine.MainWindow.Get();
    if (IO.WantSetMousePos)
    {
        Platform::SetCursorPos(Window, static_cast<int32>(IO.MousePos.x), static_cast<int32>(IO.MousePos.y));
    }

    WindowShape CurrentWindowShape;
    Window->GetWindowShape(CurrentWindowShape);

    Timestamp Delta = GlobalImGuiState.FrameClock.GetDeltaTime();
    IO.DeltaTime               = static_cast<float>(Delta.AsSeconds());
    IO.DisplaySize             = ImVec2(float(CurrentWindowShape.Width), float(CurrentWindowShape.Height));
    IO.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    int32 x = 0;
    int32 y = 0;
    Platform::GetCursorPos(Window, x, y);
    
    IO.MousePos = ImVec2(static_cast<float>(x), static_cast<float>(y));

    ModifierKeyState KeyState = Platform::GetModifierKeyState();
    IO.KeyCtrl  = KeyState.IsCtrlDown();
    IO.KeyShift = KeyState.IsShiftDown();
    IO.KeyAlt   = KeyState.IsAltDown();
    IO.KeySuper = KeyState.IsSuperKeyDown();

    if (!(IO.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange))
    {
        ImGuiMouseCursor ImguiCursor = ImGui::GetMouseCursor();
        if (ImguiCursor == ImGuiMouseCursor_None || IO.MouseDrawCursor)
        {
            Platform::SetCursor(nullptr);
        }
        else
        {
            TRef<GenericCursor> Cursor = Platform::GetCursor();
            switch (ImguiCursor)
            {
            case ImGuiMouseCursor_Arrow:      Cursor = GenericCursor::Arrow;      break;
            case ImGuiMouseCursor_TextInput:  Cursor = GenericCursor::TextInput;  break;
            case ImGuiMouseCursor_ResizeAll:  Cursor = GenericCursor::ResizeAll;  break;
            case ImGuiMouseCursor_ResizeEW:   Cursor = GenericCursor::ResizeEW;   break;
            case ImGuiMouseCursor_ResizeNS:   Cursor = GenericCursor::ResizeNS;   break;
            case ImGuiMouseCursor_ResizeNESW: Cursor = GenericCursor::ResizeNESW; break;
            case ImGuiMouseCursor_ResizeNWSE: Cursor = GenericCursor::ResizeNWSE; break;
            case ImGuiMouseCursor_Hand:       Cursor = GenericCursor::Hand;       break;
            case ImGuiMouseCursor_NotAllowed: Cursor = GenericCursor::NotAllowed; break;
            }
            
            Platform::SetCursor(Cursor.Get());
        }
    }

    // Begin new frame
    ImGui::NewFrame();

    for (UIDrawFunc Func : GlobalDrawFuncs)
    {
        Func();
    }
    GlobalDrawFuncs.Clear();

    // Draw DebugWindow with DebugStrings
    if (!GlobalDebugStrings.IsEmpty())
    {
        constexpr float Width = 400.0f;
        ImGui::SetNextWindowPos(ImVec2(static_cast<float>(CurrentWindowShape.Width - Width), 18.0f));
        ImGui::SetNextWindowSize(ImVec2(Width, 0.0f));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));

        ImGui::Begin(
            "DebugWindow", 
            nullptr,
            ImGuiWindowFlags_NoMove          | 
            ImGuiWindowFlags_NoDecoration    | 
            ImGuiWindowFlags_NoSavedSettings);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

        for (const std::string& Str : GlobalDebugStrings)
        {
            ImGui::Text(Str.c_str());
        }
        GlobalDebugStrings.Clear();

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::End();

        // EndFrame
        ImGui::EndFrame();
    }

    // Render ImgGui draw data
    ImGui::Render();

    ImDrawData* DrawData = ImGui::GetDrawData();

    float L = DrawData->DisplayPos.x;
    float R = DrawData->DisplayPos.x + DrawData->DisplaySize.x;
    float T = DrawData->DisplayPos.y;
    float B = DrawData->DisplayPos.y + DrawData->DisplaySize.y;
    float MVP[4][4] =
    {
        { 2.0f / (R - L),    0.0f,              0.0f, 0.0f },
        { 0.0f,              2.0f / (T - B),    0.0f, 0.0f },
        { 0.0f,              0.0f,              0.5f, 0.0f },
        { (R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f },
    };

    // Setup viewport
    CmdList.SetViewport(DrawData->DisplaySize.x, DrawData->DisplaySize.y, 0.0f, 1.0f, 0.0f, 0.0f);

    CmdList.Set32BitShaderConstants(GlobalImGuiState.PShader.Get(), &MVP, 16);

    CmdList.SetVertexBuffers(&GlobalImGuiState.VertexBuffer, 1, 0);
    CmdList.SetIndexBuffer(GlobalImGuiState.IndexBuffer.Get());
    CmdList.SetPrimitiveTopology(EPrimitiveTopology::TriangleList);
    CmdList.SetBlendFactor(ColorF(0.0f, 0.0f, 0.0f, 0.0f));

    // TODO: Do not change to GenericRead, change to vertex/constantbuffer
    CmdList.TransitionBuffer(GlobalImGuiState.VertexBuffer.Get(), EResourceState::GenericRead, EResourceState::CopyDest);
    CmdList.TransitionBuffer(GlobalImGuiState.IndexBuffer.Get(), EResourceState::GenericRead, EResourceState::CopyDest);

    uint32 VertexOffset = 0;
    uint32 IndexOffset  = 0;
    for (int32 i = 0; i < DrawData->CmdListsCount; i++)
    {
        const ImDrawList* ImCmdList = DrawData->CmdLists[i];
        
        const uint32 VertexSize = ImCmdList->VtxBuffer.Size * sizeof(ImDrawVert);
        CmdList.UpdateBuffer(GlobalImGuiState.VertexBuffer.Get(), VertexOffset, VertexSize, ImCmdList->VtxBuffer.Data);

        const uint32 IndexSize = ImCmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
        CmdList.UpdateBuffer(GlobalImGuiState.IndexBuffer.Get(), IndexOffset, IndexSize, ImCmdList->IdxBuffer.Data);

        VertexOffset += VertexSize;
        IndexOffset  += IndexSize;
    }

    CmdList.TransitionBuffer(GlobalImGuiState.VertexBuffer.Get(), EResourceState::CopyDest, EResourceState::GenericRead);
    CmdList.TransitionBuffer(GlobalImGuiState.IndexBuffer.Get(), EResourceState::CopyDest, EResourceState::GenericRead);

    CmdList.SetSamplerState(GlobalImGuiState.PShader.Get(), GlobalImGuiState.PointSampler.Get(), 0);

    int32  GlobalVertexOffset = 0;
    int32  GlobalIndexOffset  = 0;
    ImVec2 ClipOff            = DrawData->DisplayPos;
    for (int32 i = 0; i < DrawData->CmdListsCount; i++)
    {
        const ImDrawList* DrawCmdList = DrawData->CmdLists[i];
        for (int32 CmdIndex = 0; CmdIndex < DrawCmdList->CmdBuffer.Size; CmdIndex++)
        {
            CmdList.SetGraphicsPipelineState(GlobalImGuiState.PipelineState.Get());

            const ImDrawCmd* Cmd = &DrawCmdList->CmdBuffer[CmdIndex];
            if (Cmd->TextureId)
            {
                ImGuiImage* Image = reinterpret_cast<ImGuiImage*>(Cmd->TextureId);
                GlobalImGuiState.Images.EmplaceBack(Image);
                
                if (Image->BeforeState != EResourceState::PixelShaderResource)
                {
                    CmdList.TransitionTexture(Image->Image.Get(), Image->BeforeState, EResourceState::PixelShaderResource);

                    // TODO: Another way to do this? May break somewhere?
                    Image->BeforeState = EResourceState::PixelShaderResource;
                }

                CmdList.SetShaderResourceView(GlobalImGuiState.PShader.Get(), Image->ImageView.Get(), 0);

                if (!Image->AllowBlending)
                {
                    CmdList.SetGraphicsPipelineState(GlobalImGuiState.PipelineStateNoBlending.Get());
                }
            }
            else
            {
                ShaderResourceView* View = GlobalImGuiState.FontTexture->GetShaderResourceView();
                CmdList.SetShaderResourceView(GlobalImGuiState.PShader.Get(), View, 0);
            }

            CmdList.SetScissorRect(Cmd->ClipRect.z - ClipOff.x, Cmd->ClipRect.w - ClipOff.y, Cmd->ClipRect.x - ClipOff.x, Cmd->ClipRect.y - ClipOff.y);
            
            CmdList.DrawIndexedInstanced(Cmd->ElemCount, 1, Cmd->IdxOffset + GlobalIndexOffset, Cmd->VtxOffset + GlobalVertexOffset, 0);
        }

        GlobalIndexOffset  += DrawCmdList->IdxBuffer.Size;
        GlobalVertexOffset += DrawCmdList->VtxBuffer.Size;
    }

    for (ImGuiImage* Image : GlobalImGuiState.Images)
    {
        Assert(Image != nullptr);

        if (Image->AfterState != EResourceState::PixelShaderResource)
        {
            CmdList.TransitionTexture(Image->Image.Get(), EResourceState::PixelShaderResource, Image->AfterState);
        }
    }


    GlobalImGuiState.Images.Clear();
}

ImGuiContext* DebugUI::GetCurrentContext()
{
    return GlobalImGuiState.Context;
}
