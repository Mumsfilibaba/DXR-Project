#include "DebugUI.h"

#include "Debug/Profiler.h"

#include "Application/Events/EventDispatcher.h"
#include "Application/Generic/GenericCursor.h"
#include "Application/Platform/PlatformApplication.h"

#include "Time/Clock.h"

#include "Containers/TArray.h"

#include "Rendering/TextureFactory.h"
#include "Rendering/Renderer.h"

#include "RenderLayer/Resources.h"
#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

struct ImGuiState
{
    void Reset()
    {
        FontTexture.Texture.Reset();
        FontTexture.View.Reset();
        PipelineState.Reset();
        VertexBuffer.Reset();
        IndexBuffer.Reset();
    }

    Clock FrameClock;

    SampledTexture2D                  FontTexture;
    TSharedRef<GraphicsPipelineState> PipelineState;
    TSharedRef<GraphicsPipelineState> PipelineStateNoBlending;
    TSharedRef<VertexBuffer>          VertexBuffer;
    TSharedRef<IndexBuffer>           IndexBuffer;
    TArray<ImGuiImage*>               Images;
    
    ImGuiContext* Context = nullptr;
};

static ImGuiState GlobalImGuiState;

static UInt32 GetMouseButtonIndex(EMouseButton Button)
{
    switch (Button)
    {
    case MouseButton_Left:    return 0;
    case MouseButton_Right:   return 1;
    case MouseButton_Middle:  return 2;
    case MouseButton_Back:    return 3;
    case MouseButton_Forward: return 4;
    default:                  return static_cast<UInt32>(-1);
    }
}

static TArray<DebugUI::UIDrawFunc> GlobalDrawFuncs;
static TArray<std::string>         GlobalDebugStrings;

Bool DebugUI::Init()
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

#ifdef WIN32
    IO.ImeWindowHandle = gMainWindow->GetNativeHandle();
#endif

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

    Style.Colors[ImGuiCol_WindowBg].x = 0.15f;
    Style.Colors[ImGuiCol_WindowBg].y = 0.15f;
    Style.Colors[ImGuiCol_WindowBg].z = 0.15f;
    Style.Colors[ImGuiCol_WindowBg].w = 0.9f;

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
    Byte* Pixels = nullptr;
    Int32 Width  = 0;
    Int32 Height = 0;
    IO.Fonts->GetTexDataAsRGBA32(&Pixels, &Width, &Height);

    GlobalImGuiState.FontTexture = TextureFactory::LoadSampledTextureFromMemory(Pixels, Width, Height, 0, EFormat::R8G8B8A8_Unorm);
    if (!GlobalImGuiState.FontTexture)
    {
        return false;
    }

    static const Char* VSSource = 
    R"*(
    cbuffer vertexBuffer : register(b0)
    {
        float4x4 ProjectionMatrix;
    };
    struct VS_INPUT
    {
        float2 pos : POSITION;
        float4 col : COLOR0;
        float2 uv : TEXCOORD0;
    };
    struct PS_INPUT
    {
        float4 pos : SV_POSITION;
        float4 col : COLOR0;
        float2 uv : TEXCOORD0;
    };
    PS_INPUT Main(VS_INPUT input)
    {
        PS_INPUT output;
        output.pos    = mul(ProjectionMatrix, float4(input.pos.xy, 0.0f, 1.0f));
        output.col    = input.col;
        output.uv    = input.uv;
        return output;
    })*";

    TArray<UInt8> ShaderCode;
    if (!ShaderCompiler::CompileShader(VSSource, "Main", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
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

    static const Char* PSSource = 
        R"*(
        struct PS_INPUT
        {
            float4 pos : SV_POSITION;
            float4 col : COLOR0;
            float2 uv  : TEXCOORD0;
        };
        SamplerState sampler0    : register(s0);
        Texture2D texture0        : register(t0);
        float4 Main(PS_INPUT input) : SV_Target
        {
            float4 out_col = input.col * texture0.Sample(sampler0, input.uv);
            return out_col;
        })*";

    if (!ShaderCompiler::CompileShader(PSSource, "Main", nullptr, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
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

    InputLayoutStateCreateInfo InputLayoutInfo =
    {
        { "POSITION", 0, EFormat::R32G32_Float,   0, static_cast<UINT>(IM_OFFSETOF(ImDrawVert, pos)), EInputClassification::Vertex, 0 },
        { "TEXCOORD", 0, EFormat::R32G32_Float,   0, static_cast<UINT>(IM_OFFSETOF(ImDrawVert, uv)),  EInputClassification::Vertex, 0 },
        { "COLOR",    0, EFormat::R8G8B8A8_Unorm, 0, static_cast<UINT>(IM_OFFSETOF(ImDrawVert, col)), EInputClassification::Vertex, 0 },
    };

    TSharedRef<InputLayoutState> InputLayout = RenderLayer::CreateInputLayout(InputLayoutInfo);
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

    TSharedRef<DepthStencilState> DepthStencilState = RenderLayer::CreateDepthStencilState(DepthStencilStateInfo);
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

    TSharedRef<RasterizerState> RasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
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

    TSharedRef<BlendState> BlendStateBlending = RenderLayer::CreateBlendState(BlendStateInfo);
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

    TSharedRef<BlendState> BlendStateNoBlending = RenderLayer::CreateBlendState(BlendStateInfo);
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
    PSOProperties.ShaderState.PixelShader                = PShader.Get();
    PSOProperties.InputLayoutState                       = InputLayout.Get();
    PSOProperties.DepthStencilState                      = DepthStencilState.Get();
    PSOProperties.BlendState                             = BlendStateBlending.Get();
    PSOProperties.RasterizerState                        = RasterizerState.Get();
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = EFormat::R8G8B8A8_Unorm;
    PSOProperties.PipelineFormats.NumRenderTargets       = 1;
    PSOProperties.PrimitiveTopologyType                  = EPrimitiveTopologyType::Triangle;

    GlobalImGuiState.PipelineState = RenderLayer::CreateGraphicsPipelineState(PSOProperties);
    if (!GlobalImGuiState.PipelineState)
    {
        Debug::DebugBreak();
        return false;
    }

    PSOProperties.BlendState = BlendStateNoBlending.Get();

    GlobalImGuiState.PipelineStateNoBlending = RenderLayer::CreateGraphicsPipelineState(PSOProperties);
    if (!GlobalImGuiState.PipelineStateNoBlending)
    {
        Debug::DebugBreak();
        return false;
    }

    GlobalImGuiState.VertexBuffer = RenderLayer::CreateVertexBuffer<ImDrawVert>(1024 * 1024, BufferUsage_Default, EResourceState::VertexAndConstantBuffer, nullptr);
    if (!GlobalImGuiState.VertexBuffer)
    {
        return false;
    }

    GlobalImGuiState.IndexBuffer = RenderLayer::CreateIndexBuffer(
        sizeof(ImDrawIdx) == 2 ? EIndexFormat::UInt16 : EIndexFormat::UInt32, 
        1024 * 1024, 
        BufferUsage_Default, 
        EResourceState::Common, 
        nullptr);
    if (!GlobalImGuiState.IndexBuffer)
    {
        return false;
    }

    gEventDispatcher->RegisterEventHandler(DebugUI::OnEvent, EEventCategory::EventCategory_Input);

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

Bool DebugUI::OnEvent(const Event& Event)
{
    ImGuiIO& IO = ImGui::GetIO();
    if (IsEventOfType<KeyPressedEvent>(Event))
    {
        const EKey Key = CastEvent<KeyReleasedEvent>(Event).Key;
        IO.KeysDown[Key] = true;
    }
    else if (IsEventOfType<KeyReleasedEvent>(Event))
    {
        const EKey Key = CastEvent<KeyReleasedEvent>(Event).Key;
        IO.KeysDown[Key] = false;
    }
    else if (IsEventOfType<KeyTypedEvent>(Event))
    {
        IO.AddInputCharacter(CastEvent<KeyTypedEvent>(Event).Character);
    }
    else if (IsEventOfType<MousePressedEvent>(Event))
    {
        const UInt32 ButtonIndex  = GetMouseButtonIndex(CastEvent<MousePressedEvent>(Event).Button);
        IO.MouseDown[ButtonIndex] = true;
    }
    else if (IsEventOfType<MouseReleasedEvent>(Event))
    {
        const UInt32 ButtonIndex  = GetMouseButtonIndex(CastEvent<MousePressedEvent>(Event).Button);
        IO.MouseDown[ButtonIndex] = false;
    }
    else if (IsEventOfType<MouseScrolledEvent>(Event))
    {
        IO.MouseWheel  += CastEvent<MouseScrolledEvent>(Event).VerticalDelta;
        IO.MouseWheelH += CastEvent<MouseScrolledEvent>(Event).HorizontalDelta;
    }

    return false;
}

void DebugUI::Render(CommandList& CmdList)
{
    GlobalImGuiState.FrameClock.Tick();

    ImGuiIO& IO = ImGui::GetIO();
    GenericWindow* Window = gMainWindow;
    if (IO.WantSetMousePos)
    {
        gApplication->SetCursorPos(Window, static_cast<Int32>(IO.MousePos.x), static_cast<Int32>(IO.MousePos.y));
    }

    WindowShape CurrentWindowShape;
    Window->GetWindowShape(CurrentWindowShape);

    Timestamp Delta = GlobalImGuiState.FrameClock.GetDeltaTime();
    IO.DeltaTime               = static_cast<Float>(Delta.AsSeconds());
    IO.DisplaySize             = ImVec2(Float(CurrentWindowShape.Width), Float(CurrentWindowShape.Height));
    IO.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    Int32 x = 0;
    Int32 y = 0;
    gApplication->GetCursorPos(Window, x, y);
    
    IO.MousePos = ImVec2(static_cast<Float>(x), static_cast<Float>(y));

    ModifierKeyState KeyState = PlatformApplication::GetModifierKeyState();
    IO.KeyCtrl  = KeyState.IsCtrlDown();
    IO.KeyShift = KeyState.IsShiftDown();
    IO.KeyAlt   = KeyState.IsAltDown();
    IO.KeySuper = KeyState.IsSuperKeyDown();

    if (!(IO.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange))
    {
        ImGuiMouseCursor ImguiCursor = ImGui::GetMouseCursor();
        if (ImguiCursor == ImGuiMouseCursor_None || IO.MouseDrawCursor)
        {
            gApplication->SetCursor(nullptr);
        }
        else
        {
            TSharedRef<GenericCursor> Cursor = GlobalCursors::Arrow;
            switch (ImguiCursor)
            {
            case ImGuiMouseCursor_Arrow:      Cursor = GlobalCursors::Arrow;      break;
            case ImGuiMouseCursor_TextInput:  Cursor = GlobalCursors::TextInput;  break;
            case ImGuiMouseCursor_ResizeAll:  Cursor = GlobalCursors::ResizeAll;  break;
            case ImGuiMouseCursor_ResizeEW:   Cursor = GlobalCursors::ResizeEW;   break;
            case ImGuiMouseCursor_ResizeNS:   Cursor = GlobalCursors::ResizeNS;   break;
            case ImGuiMouseCursor_ResizeNESW: Cursor = GlobalCursors::ResizeNESW; break;
            case ImGuiMouseCursor_ResizeNWSE: Cursor = GlobalCursors::ResizeNWSE; break;
            case ImGuiMouseCursor_Hand:       Cursor = GlobalCursors::Hand;       break;
            case ImGuiMouseCursor_NotAllowed: Cursor = GlobalCursors::NotAllowed; break;
            }
            
            gApplication->SetCursor(Cursor.Get());
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
        constexpr Float Width = 400.0f;
        ImGui::SetNextWindowPos(ImVec2(static_cast<Float>(CurrentWindowShape.Width - Width), 18.0f));
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

    Float L = DrawData->DisplayPos.x;
    Float R = DrawData->DisplayPos.x + DrawData->DisplaySize.x;
    Float T = DrawData->DisplayPos.y;
    Float B = DrawData->DisplayPos.y + DrawData->DisplaySize.y;
    Float MVP[4][4] =
    {
        { 2.0f / (R - L),    0.0f,              0.0f, 0.0f },
        { 0.0f,              2.0f / (T - B),    0.0f, 0.0f },
        { 0.0f,              0.0f,              0.5f, 0.0f },
        { (R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f },
    };

    // Setup viewport
    CmdList.BindViewport(DrawData->DisplaySize.x, DrawData->DisplaySize.y, 0.0f, 1.0f, 0.0f, 0.0f);

    CmdList.Bind32BitShaderConstants(EShaderStage::Pixel, &MVP, 16);

    CmdList.BindVertexBuffers(&GlobalImGuiState.VertexBuffer, 1, 0);
    CmdList.BindIndexBuffer(GlobalImGuiState.IndexBuffer.Get());
    CmdList.BindPrimitiveTopology(EPrimitiveTopology::TriangleList);
    CmdList.BindBlendFactor(ColorF(0.0f, 0.0f, 0.0f, 0.0f));

    // TODO: Do not change to GenericRead, change to vertex/constantbuffer
    CmdList.TransitionBuffer(GlobalImGuiState.VertexBuffer.Get(), EResourceState::GenericRead, EResourceState::CopyDest);
    CmdList.TransitionBuffer(GlobalImGuiState.IndexBuffer.Get(), EResourceState::GenericRead, EResourceState::CopyDest);

    UInt32 VertexOffset = 0;
    UInt32 IndexOffset  = 0;
    for (Int32 i = 0; i < DrawData->CmdListsCount; i++)
    {
        const ImDrawList* ImCmdList = DrawData->CmdLists[i];
        
        const UInt32 VertexSize = ImCmdList->VtxBuffer.Size * sizeof(ImDrawVert);
        CmdList.UpdateBuffer(GlobalImGuiState.VertexBuffer.Get(), VertexOffset, VertexSize, ImCmdList->VtxBuffer.Data);

        const UInt32 IndexSize = ImCmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
        CmdList.UpdateBuffer(GlobalImGuiState.IndexBuffer.Get(), IndexOffset, IndexSize, ImCmdList->IdxBuffer.Data);

        VertexOffset += VertexSize;
        IndexOffset  += IndexSize;
    }

    CmdList.TransitionBuffer(GlobalImGuiState.VertexBuffer.Get(), EResourceState::CopyDest, EResourceState::GenericRead);
    CmdList.TransitionBuffer(GlobalImGuiState.IndexBuffer.Get(), EResourceState::CopyDest, EResourceState::GenericRead);

    Int32  GlobalVertexOffset = 0;
    Int32  GlobalIndexOffset  = 0;
    ImVec2 ClipOff            = DrawData->DisplayPos;
    for (Int32 i = 0; i < DrawData->CmdListsCount; i++)
    {
        const ImDrawList* DrawCmdList = DrawData->CmdLists[i];
        for (Int32 CmdIndex = 0; CmdIndex < DrawCmdList->CmdBuffer.Size; CmdIndex++)
        {
            CmdList.BindGraphicsPipelineState(GlobalImGuiState.PipelineState.Get());

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

                CmdList.BindShaderResourceViews(EShaderStage::Pixel, &Image->ImageView, 1, 0);

                if (!Image->AllowBlending)
                {
                    CmdList.BindGraphicsPipelineState(GlobalImGuiState.PipelineStateNoBlending.Get());
                }
            }
            else
            {
                CmdList.BindShaderResourceViews(EShaderStage::Pixel, &GlobalImGuiState.FontTexture.View, 1, 0);
            }

            CmdList.BindScissorRect(Cmd->ClipRect.z - ClipOff.x, Cmd->ClipRect.w - ClipOff.y, Cmd->ClipRect.x - ClipOff.x, Cmd->ClipRect.y - ClipOff.y);

            CmdList.DrawIndexedInstanced(Cmd->ElemCount, 1, Cmd->IdxOffset + GlobalIndexOffset, Cmd->VtxOffset + GlobalVertexOffset, 0);
        }

        GlobalIndexOffset  += DrawCmdList->IdxBuffer.Size;
        GlobalVertexOffset += DrawCmdList->VtxBuffer.Size;
    }

    for (ImGuiImage* Image : GlobalImGuiState.Images)
    {
        VALIDATE(Image != nullptr);

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
