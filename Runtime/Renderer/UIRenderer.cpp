#include "UIRenderer.h"

#include "Engine/Engine.h"

#include "Renderer.h"
#include "Resources/TextureFactory.h"

#include "RHI/RHICore.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIShaderCompiler.h"

#include "Core/Time/Timer.h"
#include "Core/Application/ICursor.h"
#include "Core/Application/Application.h"
#include "Core/Application/Platform/PlatformApplicationMisc.h"
#include "Core/Debug/FrameProfiler.h"
#include "Core/Containers/Array.h"

static uint32 GetMouseButtonIndex( EMouseButton Button )
{
    switch ( Button )
    {
        case MouseButton_Left:    return 0;
        case MouseButton_Right:   return 1;
        case MouseButton_Middle:  return 2;
        case MouseButton_Back:    return 3;
        case MouseButton_Forward: return 4;
        default:                  return static_cast<uint32>(-1);
    }
}

CUIRenderer::~CUIRenderer()
{
    if ( Context )
    {
        ImGui::DestroyContext( Context );
    }
}

bool CUIRenderer::Init()
{
    // Create context
    IMGUI_CHECKVERSION();

    Context = ImGui::CreateContext();
    if ( !Context )
    {
        return false;
    }

    ImGuiIO& IO = ImGui::GetIO();
    IO.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    IO.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    IO.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    IO.BackendPlatformName = "Windows";
    IO.ImeWindowHandle = GEngine->MainWindow->GetNativeHandle();

    // Keyboard mapping. ImGui will use those indices to peek into the IO.KeysDown[] array that we will update during the application lifetime.
    IO.KeyMap[ImGuiKey_Tab] = EKey::Key_Tab;
    IO.KeyMap[ImGuiKey_LeftArrow] = EKey::Key_Left;
    IO.KeyMap[ImGuiKey_RightArrow] = EKey::Key_Right;
    IO.KeyMap[ImGuiKey_UpArrow] = EKey::Key_Up;
    IO.KeyMap[ImGuiKey_DownArrow] = EKey::Key_Down;
    IO.KeyMap[ImGuiKey_PageUp] = EKey::Key_PageUp;
    IO.KeyMap[ImGuiKey_PageDown] = EKey::Key_PageDown;
    IO.KeyMap[ImGuiKey_Home] = EKey::Key_Home;
    IO.KeyMap[ImGuiKey_End] = EKey::Key_End;
    IO.KeyMap[ImGuiKey_Insert] = EKey::Key_Insert;
    IO.KeyMap[ImGuiKey_Delete] = EKey::Key_Delete;
    IO.KeyMap[ImGuiKey_Backspace] = EKey::Key_Backspace;
    IO.KeyMap[ImGuiKey_Space] = EKey::Key_Space;
    IO.KeyMap[ImGuiKey_Enter] = EKey::Key_Enter;
    IO.KeyMap[ImGuiKey_Escape] = EKey::Key_Escape;
    IO.KeyMap[ImGuiKey_KeyPadEnter] = EKey::Key_KeypadEnter;
    IO.KeyMap[ImGuiKey_A] = EKey::Key_A;
    IO.KeyMap[ImGuiKey_C] = EKey::Key_C;
    IO.KeyMap[ImGuiKey_V] = EKey::Key_V;
    IO.KeyMap[ImGuiKey_X] = EKey::Key_X;
    IO.KeyMap[ImGuiKey_Y] = EKey::Key_Y;
    IO.KeyMap[ImGuiKey_Z] = EKey::Key_Z;

    // Setup style
    ImGui::StyleColorsDark();

    ImGuiStyle& Style = ImGui::GetStyle();
    Style.WindowRounding = 0.0f;
    Style.FrameRounding = 0.0f;
    Style.GrabRounding = 0.0f;
    Style.TabRounding = 0.0f;
    Style.WindowBorderSize = 0.0f;
    Style.ScrollbarRounding = 0.0f;
    Style.ScrollbarSize = 12.0f;

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
    int32 Width = 0;
    int32 Height = 0;
    IO.Fonts->GetTexDataAsRGBA32( &Pixels, &Width, &Height );

    FontTexture = CTextureFactory::LoadFromMemory( Pixels, Width, Height, 0, EFormat::R8G8B8A8_Unorm );
    if ( !FontTexture )
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
    if ( !CRHIShaderCompiler::CompileShader( VSSource, "Main", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode ) )
    {
        CDebug::DebugBreak();
        return false;
    }

    TSharedRef<CRHIVertexShader> VShader = RHICreateVertexShader( ShaderCode );
    if ( !VShader )
    {
        CDebug::DebugBreak();
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

    if ( !CRHIShaderCompiler::CompileShader( PSSource, "Main", nullptr, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode ) )
    {
        CDebug::DebugBreak();
        return false;
    }

    PShader = RHICreatePixelShader( ShaderCode );
    if ( !PShader )
    {
        CDebug::DebugBreak();
        return false;
    }

    SInputLayoutStateCreateInfo InputLayoutInfo =
    {
        { "POSITION", 0, EFormat::R32G32_Float,   0, static_cast<uint32>(IM_OFFSETOF( ImDrawVert, pos )), EInputClassification::Vertex, 0 },
        { "TEXCOORD", 0, EFormat::R32G32_Float,   0, static_cast<uint32>(IM_OFFSETOF( ImDrawVert, uv )),  EInputClassification::Vertex, 0 },
        { "COLOR",    0, EFormat::R8G8B8A8_Unorm, 0, static_cast<uint32>(IM_OFFSETOF( ImDrawVert, col )), EInputClassification::Vertex, 0 },
    };

    TSharedRef<CRHIInputLayoutState> InputLayout = RHICreateInputLayout( InputLayoutInfo );
    if ( !InputLayout )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        InputLayout->SetName( "ImGui InputLayoutState" );
    }

    SDepthStencilStateCreateInfo DepthStencilStateInfo;
    DepthStencilStateInfo.DepthEnable = false;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::Zero;

    TSharedRef<CRHIDepthStencilState> DepthStencilState = RHICreateDepthStencilState( DepthStencilStateInfo );
    if ( !DepthStencilState )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        DepthStencilState->SetName( "ImGui DepthStencilState" );
    }

    SRasterizerStateCreateInfo RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::None;

    TSharedRef<CRHIRasterizerState> RasterizerState = RHICreateRasterizerState( RasterizerStateInfo );
    if ( !RasterizerState )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        RasterizerState->SetName( "ImGui RasterizerState" );
    }

    SBlendStateCreateInfo BlendStateInfo;
    BlendStateInfo.IndependentBlendEnable = false;
    BlendStateInfo.RenderTarget[0].BlendEnable = true;
    BlendStateInfo.RenderTarget[0].SrcBlend = EBlend::SrcAlpha;
    BlendStateInfo.RenderTarget[0].SrcBlendAlpha = EBlend::InvSrcAlpha;
    BlendStateInfo.RenderTarget[0].DestBlend = EBlend::InvSrcAlpha;
    BlendStateInfo.RenderTarget[0].DestBlendAlpha = EBlend::Zero;
    BlendStateInfo.RenderTarget[0].BlendOpAlpha = EBlendOp::Add;
    BlendStateInfo.RenderTarget[0].BlendOp = EBlendOp::Add;

    TSharedRef<CRHIBlendState> BlendStateBlending = RHICreateBlendState( BlendStateInfo );
    if ( !BlendStateBlending )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        BlendStateBlending->SetName( "ImGui BlendState" );
    }

    BlendStateInfo.RenderTarget[0].BlendEnable = false;

    TSharedRef<CRHIBlendState> BlendStateNoBlending = RHICreateBlendState( BlendStateInfo );
    if ( !BlendStateBlending )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        BlendStateBlending->SetName( "ImGui BlendState No Blending" );
    }

    SGraphicsPipelineStateCreateInfo PSOProperties;
    PSOProperties.ShaderState.VertexShader = VShader.Get();
    PSOProperties.ShaderState.PixelShader = PShader.Get();
    PSOProperties.InputLayoutState = InputLayout.Get();
    PSOProperties.DepthStencilState = DepthStencilState.Get();
    PSOProperties.BlendState = BlendStateBlending.Get();
    PSOProperties.RasterizerState = RasterizerState.Get();
    PSOProperties.PipelineFormats.RenderTargetFormats[0] = EFormat::R8G8B8A8_Unorm;
    PSOProperties.PipelineFormats.NumRenderTargets = 1;
    PSOProperties.PrimitiveTopologyType = EPrimitiveTopologyType::Triangle;

    PipelineState = RHICreateGraphicsPipelineState( PSOProperties );
    if ( !PipelineState )
    {
        CDebug::DebugBreak();
        return false;
    }

    PSOProperties.BlendState = BlendStateNoBlending.Get();

    PipelineStateNoBlending = RHICreateGraphicsPipelineState( PSOProperties );
    if ( !PipelineStateNoBlending )
    {
        CDebug::DebugBreak();
        return false;
    }

    VertexBuffer = RHICreateVertexBuffer<ImDrawVert>( 1024 * 1024, BufferFlag_Default, EResourceState::VertexAndConstantBuffer, nullptr );
    if ( !VertexBuffer )
    {
        return false;
    }
    else
    {
        VertexBuffer->SetName( "ImGui VertexBuffer" );
    }

    const EIndexFormat IndexFormat = (sizeof( ImDrawIdx ) == 2) ? EIndexFormat::uint16 : EIndexFormat::uint32;
    IndexBuffer = RHICreateIndexBuffer( IndexFormat, 1024 * 1024, BufferFlag_Default, EResourceState::Common, nullptr );
    if ( !IndexBuffer )
    {
        return false;
    }
    else
    {
        IndexBuffer->SetName( "ImGui IndexBuffer" );
    }

    SSamplerStateCreateInfo CreateInfo;
    CreateInfo.AddressU = ESamplerMode::Clamp;
    CreateInfo.AddressV = ESamplerMode::Clamp;
    CreateInfo.AddressW = ESamplerMode::Clamp;
    CreateInfo.Filter = ESamplerFilter::MinMagMipPoint;

    PointSampler = RHICreateSamplerState( CreateInfo );
    if ( !PointSampler )
    {
        return false;
    }

    // Setup input callbacks
    InputHandler.KeyEventDelegate.BindRaw( this, &CUIRenderer::OnKeyEvent );
    InputHandler.KeyTypedDelegate.BindRaw( this, &CUIRenderer::OnKeyTyped );
    InputHandler.MouseButtonDelegate.BindRaw( this, &CUIRenderer::OnMouseButtonEvent );
    InputHandler.MouseScrolledDelegate.BindRaw( this, &CUIRenderer::OnMouseScrolled );

    // Add the input handler
    CApplication::Get().AddInputHandler( &InputHandler );

    return true;
}

void CUIRenderer::OnKeyEvent( const SKeyEvent& KeyEvent )
{
    ImGuiIO& IO = ImGui::GetIO();
    IO.KeysDown[KeyEvent.KeyCode] = KeyEvent.IsDown;
}

void CUIRenderer::OnKeyTyped( SKeyTypedEvent Event )
{
    ImGuiIO& IO = ImGui::GetIO();
    IO.AddInputCharacter( Event.Character );
}

void CUIRenderer::OnMouseButtonEvent( const SMouseButtonEvent& Event )
{
    ImGuiIO& IO = ImGui::GetIO();
    const uint32 ButtonIndex = GetMouseButtonIndex( Event.Button );
    IO.MouseDown[ButtonIndex] = Event.IsDown;
}

void CUIRenderer::OnMouseScrolled( const SMouseScrolledEvent& Event )
{
    ImGuiIO& IO = ImGui::GetIO();
    IO.MouseWheel += Event.VerticalDelta;
    IO.MouseWheelH += Event.HorizontalDelta;
}

void CUIRenderer::BeginTick()
{
    FrameClock.Tick();

    ImGuiIO& IO = ImGui::GetIO();

    TSharedRef<CCoreWindow> Window = GEngine->MainWindow;
    if ( IO.WantSetMousePos )
    {
        CApplication::Get().SetCursorPos( Window, CIntVector2( static_cast<int32>(IO.MousePos.x), static_cast<int32>(IO.MousePos.y) ) );
    }

    SWindowShape CurrentWindowShape;
    Window->GetWindowShape( CurrentWindowShape );

    CTimestamp Delta = FrameClock.GetDeltaTime();
    IO.DeltaTime = static_cast<float>(Delta.AsSeconds());
    IO.DisplaySize = ImVec2( float( CurrentWindowShape.Width ), float( CurrentWindowShape.Height ) );
    IO.DisplayFramebufferScale = ImVec2( 1.0f, 1.0f );

    CIntVector2 Position = CApplication::Get().GetCursorPos( Window );
    IO.MousePos = ImVec2( static_cast<float>(Position.x), static_cast<float>(Position.y) );

    SModifierKeyState KeyState = PlatformApplicationMisc::GetModifierKeyState();
    IO.KeyCtrl = KeyState.IsCtrlDown;
    IO.KeyShift = KeyState.IsShiftDown;
    IO.KeyAlt = KeyState.IsAltDown;
    IO.KeySuper = KeyState.IsSuperKeyDown;

    if ( !(IO.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) )
    {
        ImGuiMouseCursor ImguiCursor = ImGui::GetMouseCursor();
        if ( ImguiCursor == ImGuiMouseCursor_None || IO.MouseDrawCursor )
        {
            CApplication::Get().SetCursor( ECursor::None );
        }
        else
        {
            ECursor Cursor = ECursor::Arrow;
            switch ( ImguiCursor )
            {
                case ImGuiMouseCursor_Arrow:      Cursor = ECursor::Arrow;      break;
                case ImGuiMouseCursor_TextInput:  Cursor = ECursor::TextInput;  break;
                case ImGuiMouseCursor_ResizeAll:  Cursor = ECursor::ResizeAll;  break;
                case ImGuiMouseCursor_ResizeEW:   Cursor = ECursor::ResizeEW;   break;
                case ImGuiMouseCursor_ResizeNS:   Cursor = ECursor::ResizeNS;   break;
                case ImGuiMouseCursor_ResizeNESW: Cursor = ECursor::ResizeNESW; break;
                case ImGuiMouseCursor_ResizeNWSE: Cursor = ECursor::ResizeNWSE; break;
                case ImGuiMouseCursor_Hand:       Cursor = ECursor::Hand;       break;
                case ImGuiMouseCursor_NotAllowed: Cursor = ECursor::NotAllowed; break;
            }

            CApplication::Get().SetCursor( Cursor );
        }
    }

    // Begin new frame
    ImGui::NewFrame();
}

void CUIRenderer::EndTick()
{
    // EndFrame
    ImGui::EndFrame();
}

TSharedRef<CUIRenderer> CUIRenderer::Make()
{
    TSharedRef<CUIRenderer> NewRenderer = dbg_new CUIRenderer();
    if ( NewRenderer->Init() )
    {
        return NewRenderer;
    }

    return nullptr;
}

void CUIRenderer::Render( CRHICommandList& CmdList )
{
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
    CmdList.SetViewport( DrawData->DisplaySize.x, DrawData->DisplaySize.y, 0.0f, 1.0f, 0.0f, 0.0f );

    CmdList.Set32BitShaderConstants( PShader.Get(), &MVP, 16 );

    CmdList.SetVertexBuffers( &VertexBuffer, 1, 0 );
    CmdList.SetIndexBuffer( IndexBuffer.Get() );
    CmdList.SetPrimitiveTopology( EPrimitiveTopology::TriangleList );
    CmdList.SetBlendFactor( SColorF( 0.0f, 0.0f, 0.0f, 0.0f ) );

    // TODO: Do not change to GenericRead, change to vertex/constantbuffer
    CmdList.TransitionBuffer( VertexBuffer.Get(), EResourceState::GenericRead, EResourceState::CopyDest );
    CmdList.TransitionBuffer( IndexBuffer.Get(), EResourceState::GenericRead, EResourceState::CopyDest );

    uint32 VertexOffset = 0;
    uint32 IndexOffset = 0;
    for ( int32 i = 0; i < DrawData->CmdListsCount; i++ )
    {
        const ImDrawList* ImCmdList = DrawData->CmdLists[i];

        const uint32 VertexSize = ImCmdList->VtxBuffer.Size * sizeof( ImDrawVert );
        CmdList.UpdateBuffer( VertexBuffer.Get(), VertexOffset, VertexSize, ImCmdList->VtxBuffer.Data );

        const uint32 IndexSize = ImCmdList->IdxBuffer.Size * sizeof( ImDrawIdx );
        CmdList.UpdateBuffer( IndexBuffer.Get(), IndexOffset, IndexSize, ImCmdList->IdxBuffer.Data );

        VertexOffset += VertexSize;
        IndexOffset += IndexSize;
    }

    CmdList.TransitionBuffer( VertexBuffer.Get(), EResourceState::CopyDest, EResourceState::GenericRead );
    CmdList.TransitionBuffer( IndexBuffer.Get(), EResourceState::CopyDest, EResourceState::GenericRead );

    CmdList.SetSamplerState( PShader.Get(), PointSampler.Get(), 0 );

    int32  GlobalVertexOffset = 0;
    int32  GlobalIndexOffset = 0;
    ImVec2 ClipOff = DrawData->DisplayPos;
    for ( int32 i = 0; i < DrawData->CmdListsCount; i++ )
    {
        const ImDrawList* DrawCmdList = DrawData->CmdLists[i];
        for ( int32 CmdIndex = 0; CmdIndex < DrawCmdList->CmdBuffer.Size; CmdIndex++ )
        {
            CmdList.SetGraphicsPipelineState( PipelineState.Get() );

            const ImDrawCmd* Cmd = &DrawCmdList->CmdBuffer[CmdIndex];
            if ( Cmd->TextureId )
            {
                SImGuiImage* Image = reinterpret_cast<SImGuiImage*>(Cmd->TextureId);
                Images.Emplace( Image );

                if ( Image->BeforeState != EResourceState::PixelShaderResource )
                {
                    CmdList.TransitionTexture( Image->Image.Get(), Image->BeforeState, EResourceState::PixelShaderResource );

                    // TODO: Another way to do this? May break somewhere?
                    Image->BeforeState = EResourceState::PixelShaderResource;
                }

                CmdList.SetShaderResourceView( PShader.Get(), Image->ImageView.Get(), 0 );

                if ( !Image->AllowBlending )
                {
                    CmdList.SetGraphicsPipelineState( PipelineStateNoBlending.Get() );
                }
            }
            else
            {
                CRHIShaderResourceView* View = FontTexture->GetShaderResourceView();
                CmdList.SetShaderResourceView( PShader.Get(), View, 0 );
            }

            CmdList.SetScissorRect( Cmd->ClipRect.z - ClipOff.x, Cmd->ClipRect.w - ClipOff.y, Cmd->ClipRect.x - ClipOff.x, Cmd->ClipRect.y - ClipOff.y );

            CmdList.DrawIndexedInstanced( Cmd->ElemCount, 1, Cmd->IdxOffset + GlobalIndexOffset, Cmd->VtxOffset + GlobalVertexOffset, 0 );
        }

        GlobalIndexOffset += DrawCmdList->IdxBuffer.Size;
        GlobalVertexOffset += DrawCmdList->VtxBuffer.Size;
    }

    for ( SImGuiImage* Image : Images )
    {
        Assert( Image != nullptr );

        if ( Image->AfterState != EResourceState::PixelShaderResource )
        {
            CmdList.TransitionTexture( Image->Image.Get(), EResourceState::PixelShaderResource, Image->AfterState );
        }
    }


    Images.Clear();
}

UIContextHandle CUIRenderer::GetContext() const
{
    return reinterpret_cast<UIContextHandle>(Context);
}
