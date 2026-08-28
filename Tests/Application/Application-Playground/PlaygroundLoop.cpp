#include <Core/Core.h>
#include <Core/CoreGlobals.h>
#include <Core/Containers/UniquePtr.h>
#include <Core/Math/Vector4.h>
#include <Core/Memory/MemoryPagePool.h>
#include <Core/Misc/CommandLine.h>
#include <Core/Misc/Config.h>
#include <Core/Misc/ConsoleManager.h>
#include <Core/Misc/CoreDelegates.h>
#include <Core/Misc/FileOutputDevice.h>
#include <Core/Misc/OutputDeviceLogger.h>
#include <Core/Misc/Paths.h>
#include <Core/Modules/ModuleManager.h>
#include <Core/Platform/PlatformMisc.h>
#include <Core/PlatformInterface/PlatformEventPool.h>
#include <Core/Tasks/TaskGraph.h>
#include <Core/Tasks/Tasks.h>
#include <Core/Threading/ThreadManager.h>
#include <CoreApplication/Platform/PlatformApplicationMisc.h>
#include <CoreApplication/Platform/PlatformConsoleWindow.h>
#include <Application/Application.h>
#include <Application/Console/Console.h>
#include <Application/Elements/Box.h>
#include <Application/Elements/TitleBar.h>
#include <Application/Elements/Window.h>
#include <Application/Menus/MenuInputHandler.h>
#include <Application/Menus/MenuStack.h>
#include <Application/Menus/ToolTipService.h>
#include <Application/Text/TrueTypeFontFace.h>
#include <ApplicationRenderer/ApplicationRenderer.h>
#include <RHI/RHI.h>
#include <RHI/RHICommandList.h>
#include <RHI/ShaderCompiler.h>
#include <RendererCore/Shaders/ShaderBytecodeCache.h>
#include <RendererCore/Shaders/ShaderCache.h>
#include <RendererCore/TextureFactory.h>

#include "PlaygroundLoop.h"
#include "PlaygroundShell.h"

static constexpr int32 GDefaultWindowWidth  = 1280;
static constexpr int32 GDefaultWindowHeight = 860;

static constexpr int32 GBodyFontHeight      = 15;
static constexpr int32 GHeadingFontHeight   = 18;
static constexpr int32 GMonospaceFontHeight = 14;

static TUniquePtr<IPlatformConsoleWindow> GConsoleWindow;
static TUniquePtr<FFileOutputDevice>      GFileOutputDevice;

struct FConsoleToggleHandler final : public FInputHandler
{
    virtual bool OnKeyDown(const FKeyEvent& KeyEvent) override
    {
        if (!FConsole::IsToggleKey(KeyEvent.GetKey()))
        {
            return false;
        }

        TSharedPtr<FConsole> Console = GetPlaygroundConsole();
        if (Console && !KeyEvent.IsRepeat())
        {
            Console->Toggle();

            if (Console->IsOpen())
            {
                FApplication::Get().SetFocusElement(Console->GetInput());
            }
        }

        bSwallowNextChar = true;
        return true;
    }

    virtual bool OnKeyUp(const FKeyEvent& KeyEvent) override
    {
        return FConsole::IsToggleKey(KeyEvent.GetKey());
    }

    virtual bool OnKeyChar(const FKeyEvent& KeyEvent) override
    {
        UNREFERENCED_VARIABLE(KeyEvent);

        const bool bSwallowChar = bSwallowNextChar;
        bSwallowNextChar = false;

        return bSwallowChar;
    }

    bool bSwallowNextChar = false;
};

static void InitializeOutputDevices()
{
    GConsoleWindow = TUniquePtr<IPlatformConsoleWindow>(FPlatformConsoleWindow::Create());
    if (GConsoleWindow)
    {
        GConsoleWindow->Show(true);
        GConsoleWindow->SetTitle("UI Playground Output");
        FOutputDeviceLogger::Get()->RegisterOutputDevice(GConsoleWindow.Get());
    }

    GFileOutputDevice = MakeUniquePtr<FFileOutputDevice>(Paths::GetProjectDir() + "/PlaygroundLog.txt");
    if (GFileOutputDevice && GFileOutputDevice->IsValid())
    {
        FOutputDeviceLogger::Get()->RegisterOutputDevice(GFileOutputDevice.Get());
    }
}

static void ReleaseOutputDevices()
{
    if (GFileOutputDevice)
    {
        FOutputDeviceLogger::Get()->UnregisterOutputDevice(GFileOutputDevice.Get());
        GFileOutputDevice.Reset();
    }

    if (GConsoleWindow)
    {
        FOutputDeviceLogger::Get()->UnregisterOutputDevice(GConsoleWindow.Get());
        GConsoleWindow.Reset();
    }
}

static bool LoadPlaygroundModules()
{
    const CHAR* ModuleNames[] =
    {
        "Core",
        "CoreApplication",
        "Application",
        "RHI",
        "RendererCore",
        "ApplicationRenderer",
    };

    for (const CHAR* ModuleName : ModuleNames)
    {
        if (!FModuleManager::Get().LoadModule(ModuleName))
        {
            LOG_ERROR("[FPlaygroundLoop]: Failed to load the '%s' module", ModuleName);
            return false;
        }
    }

    return true;
}

FPlaygroundLoop::FPlaygroundLoop()
    : FrameTimer()
    , CommandList()
    , Fonts()
    , Scenes()
    , Surfaces()
    , MainWindow(nullptr)
    , Shell(nullptr)
    , Renderer(nullptr)
    , MenuInputHandler(nullptr)
    , ConsoleToggleHandler(nullptr)
    , bIsRHIInitialized(false)
{
}

FPlaygroundLoop::~FPlaygroundLoop()
{
}

int32 FPlaygroundLoop::PreInit(const CHAR** Args, int32 NumArgs)
{
    InitializeOutputDevices();

    if (!CommandLine::Initialize(Args, NumArgs))
    {
        LOG_WARNING("[FPlaygroundLoop]: Invalid command line");
    }

    if (!LoadPlaygroundModules())
    {
        return -1;
    }

    if (!FConfig::Initialize())
    {
        LOG_ERROR("[FPlaygroundLoop]: Failed to initialize the config");
        return -1;
    }

    if (IConsoleVariable* RHIThreadVariable = FConsoleManager::Get().FindConsoleVariable("TaskGraph.EnableRHIThread"))
    {
        RHIThreadVariable->SetAsBool(false, EConsoleVariableFlags::SetByCode);
    }

    FConsoleManager::Get().LoadConsoleVariablesFromCommandLine();

    if (!FThreadManager::Initialize())
    {
        LOG_ERROR("[FPlaygroundLoop]: Failed to initialize the thread manager");
        return -1;
    }

    if (!FApplication::Initialize())
    {
        LOG_ERROR("[FPlaygroundLoop]: Failed to initialize the application");
        return -1;
    }

    CoreDelegates::PostApplicationCreateDelegate.Broadcast();

    if (!FTaskGraph::Initialize())
    {
        LOG_ERROR("[FPlaygroundLoop]: Failed to initialize the task graph");
        return -1;
    }

    if (!FShaderCompiler::Initialize(Paths::GetAssetDir()))
    {
        LOG_ERROR("[FPlaygroundLoop]: Failed to initialize the shader compiler");
        return -1;
    }

    if (!RHI::Initialize())
    {
        LOG_ERROR("[FPlaygroundLoop]: Failed to initialize the RHI");
        return -1;
    }

    bIsRHIInitialized = true;
    CoreDelegates::PostInitRHIDelegate.Broadcast();

    if (!FShaderCache::Initialize())
    {
        LOG_ERROR("[FPlaygroundLoop]: Failed to initialize the shader cache");
        return -1;
    }

    FShaderBytecodeCache::Initialize();

    if (!FTextureFactory::Initialize())
    {
        LOG_ERROR("[FPlaygroundLoop]: Failed to initialize the texture factory");
        return -1;
    }

    return 0;
}

int32 FPlaygroundLoop::Init()
{
    Renderer = MakeSharedPtr<FApplicationRenderer>();
    if (!Renderer->InitializeRHI())
    {
        LOG_ERROR("[FPlaygroundLoop]: Failed to initialize the application renderer");
        return -1;
    }

    FApplication::Get().SetRenderer(Renderer);

    MenuInputHandler = FMenuInputHandler::Register();

    if (!LoadFonts())
    {
        return -1;
    }

    CreatePlaygroundScenes(Fonts, Scenes);

    if (!CreateMainWindow())
    {
        return -1;
    }

    LOG_INFO("[FPlaygroundLoop]: Ready with %d scenes on the %s RHI", Scenes.Size(), ToString(RHI::Device->GetRHIType()));
    return 0;
}

bool FPlaygroundLoop::LoadFonts()
{
    const String FontDir = Paths::GetAssetDir() + "/Editor/Fonts/";

    Fonts.Body      = FTrueTypeFontFace::CreateFromFile(FontDir + "segoeui.ttf", GBodyFontHeight);
    Fonts.Heading   = FTrueTypeFontFace::CreateFromFile(FontDir + "seguisb.ttf", GHeadingFontHeight);
    Fonts.Monospace = FTrueTypeFontFace::CreateFromFile(FontDir + "consola.ttf", GMonospaceFontHeight);

    if (!Fonts.Body || !Fonts.Heading || !Fonts.Monospace)
    {
        LOG_ERROR("[FPlaygroundLoop]: Failed to load the fonts from '%s'", *FontDir);
        return false;
    }

    return true;
}

bool FPlaygroundLoop::CreateMainWindow()
{
    FWindow::FDesc WindowDesc;
    WindowDesc.Title      = "DXR UI Playground";
    WindowDesc.Size       = IntVector2(GDefaultWindowWidth, GDefaultWindowHeight);
    WindowDesc.StyleFlags = EWindowStyleFlags::Default | EWindowStyleFlags::CustomTitleBar;

    MainWindow = FWindow::Create(WindowDesc);
    if (!MainWindow)
    {
        LOG_ERROR("[FPlaygroundLoop]: Failed to create the window");
        return false;
    }

    FTitleBar::FDesc TitleBarDesc;
    TitleBarDesc.SetTitle(WindowDesc.Title).SetFont(Fonts.Body);
    TitleBarDesc.bShowCaptionButtons = true;

    Shell = FPlaygroundShell::Create(Fonts, Scenes);

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(FTitleBar::Create(TitleBarDesc));
    Column->AddSlot(Shell).SetFillCoefficient(1.0f);

    MainWindow->SetContent(Column);
    MainWindow->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &FPlaygroundLoop::OnMainWindowClosed));

    AttachConsole();

    FApplication::Get().CreateWindow(MainWindow);

    SyncSurfaces();
    if (Surfaces.IsEmpty())
    {
        LOG_ERROR("[FPlaygroundLoop]: Failed to create the swap chain");
        return false;
    }

    return true;
}

void FPlaygroundLoop::AttachConsole()
{
    TSharedPtr<FConsole> Console = GetPlaygroundConsole();
    if (!Console)
    {
        return;
    }

    MainWindow->SetOverlay(Console);

    ConsoleToggleHandler = MakeSharedPtr<FConsoleToggleHandler>();
    FApplication::Get().RegisterInputHandler(ConsoleToggleHandler);
}

FRHISwapChainRef FPlaygroundLoop::CreateSwapChain(const TSharedPtr<FWindow>& InWindow) const
{
    const IntVector2 WindowSize = InWindow->GetSize();

    FRHISwapChainDesc SwapChainDesc;
    SwapChainDesc.WindowHandle = InWindow->GetPlatformWindow()->GetPlatformHandle();
    SwapChainDesc.Width        = static_cast<uint16>(Math::Max(WindowSize.X, 1));
    SwapChainDesc.Height       = static_cast<uint16>(Math::Max(WindowSize.Y, 1));
    SwapChainDesc.ColorFormat  = EFormat::Unknown;
    SwapChainDesc.ColorSpace   = EColorSpace::Unknown;
    SwapChainDesc.Usage        = ESwapChainUsageFlags::RenderTarget;
    SwapChainDesc.bFramePacing = InWindow == MainWindow;

    return RHI::CreateSwapChain(SwapChainDesc);
}

void FPlaygroundLoop::SyncSurfaces()
{
    const TArray<TSharedPtr<FWindow>>& Windows = FApplication::Get().GetWindows();

    for (int32 Index = Surfaces.Size() - 1; Index >= 0; --Index)
    {
        if (!Windows.Contains(Surfaces[Index].Window))
        {
            FRHICommandListExecutor::Get().WaitForGPU();
            Renderer->RegisterWindowSwapChain(Surfaces[Index].Window, nullptr);
            Surfaces.RemoveAt(Index);
        }
    }

    for (const TSharedPtr<FWindow>& CurrentWindow : Windows)
    {
        const bool bHasSurface = Surfaces.ContainsWithPredicate([&CurrentWindow](const FPlaygroundSurface& Surface)
        {
            return Surface.Window == CurrentWindow;
        });

        if (bHasSurface)
        {
            continue;
        }

        FPlaygroundSurface Surface;
        Surface.Window    = CurrentWindow;
        Surface.Size      = CurrentWindow->GetSize();
        Surface.SwapChain = CreateSwapChain(CurrentWindow);

        if (!Surface.SwapChain)
        {
            LOG_ERROR("[FPlaygroundLoop]: Failed to create a swap chain for a window");
            continue;
        }

        Renderer->RegisterWindowSwapChain(CurrentWindow, Surface.SwapChain.Get());
        Surfaces.Add(Surface);
    }
}

void FPlaygroundLoop::SyncSurfaceSize(FPlaygroundSurface& Surface)
{
    const IntVector2 WindowSize = Surface.Window->GetSize();
    if (WindowSize == Surface.Size || WindowSize.X <= 0 || WindowSize.Y <= 0)
    {
        return;
    }

    Surface.Size = WindowSize;
    CommandList.ResizeSwapChain(Surface.SwapChain.Get(), static_cast<uint32>(WindowSize.X), static_cast<uint32>(WindowSize.Y));
}

void FPlaygroundLoop::RenderSurface(FPlaygroundSurface& Surface)
{
    SyncSurfaceSize(Surface);

    Renderer->RenderWindowToSwapChain(CommandList, Surface.Window, EAttachmentLoadAction::Clear);

    FRHISwapChain* SwapChain = Surface.SwapChain.Get();
    FRHITexture*   BackBuffer = SwapChain->GetBackBuffer();

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(BackBuffer, ERHIResourceState::RenderTarget, ERHIResourceState::Present));
    CommandList.PresentSwapChain(SwapChain, true);
}

void FPlaygroundLoop::OnMainWindowClosed()
{
    RequestEngineExit("The playground window was closed");
}

void FPlaygroundLoop::Tick()
{
    Tasks::ProcessMainThreadTasks();

    FrameTimer.Tick();

    const float DeltaTime = static_cast<float>(FrameTimer.GetDeltaTime().AsSeconds());

    FMenuInputHandler::Tick(DeltaTime);

    FApplication::Get().Tick(DeltaTime);

    if (IsEngineExitRequested())
    {
        return;
    }

    FApplication::Get().ProcessDeferredEvents();

    SyncSurfaces();
    if (Surfaces.IsEmpty())
    {
        return;
    }

    CommandList.BeginFrame();

    FApplication::Get().DrawWindows();

    for (FPlaygroundSurface& Surface : Surfaces)
    {
        RenderSurface(Surface);
    }

    CommandList.EndFrame();
    CommandList.FlushDeletedResources();

    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

    FMemoryPagePool::Get().Tick();
    FPlatformEventPool::Get().Tick();
}

void FPlaygroundLoop::Release()
{
    if (FRHICommandListExecutor::IsInitialized())
    {
        FRHICommandListExecutor::Get().WaitForGPU();
    }

    if (FApplication::IsInitialized())
    {
        FMenuInputHandler::Unregister(MenuInputHandler);
        MenuInputHandler.Reset();

        if (ConsoleToggleHandler)
        {
            FApplication::Get().UnregisterInputHandler(ConsoleToggleHandler);
            ConsoleToggleHandler.Reset();
        }

        FApplication::Get().SetRenderer(nullptr);
    }

    FMenuStack::Shutdown();
    FToolTipService::Shutdown();

    if (Renderer)
    {
        Renderer->ReleaseRHI();
        Renderer.Reset();
    }

    Shell.Reset();
    Scenes.Clear();
    Fonts = FPlaygroundFonts();
    MainWindow.Reset();
    Surfaces.Clear();

    if (bIsRHIInitialized)
    {
        FTextureFactory::Release();
        FShaderCache::Release();
        FShaderBytecodeCache::Release();

        RHI::Release();
        bIsRHIInitialized = false;
    }

    FShaderCompiler::Destroy();
    FTaskGraph::Release();
    FApplication::Release();
    FThreadManager::Release();
    FConfig::Release();

    CoreDelegates::Shutdown();
    FModuleManager::Shutdown();

    FMemoryPagePool::Get().Flush();
    FPlatformEventPool::Get().Flush();

    ReleaseOutputDevices();
}

struct FPlaygroundReleaseGuard
{
    explicit FPlaygroundReleaseGuard(FPlaygroundLoop& InLoop)
        : Loop(InLoop)
    {
    }

    ~FPlaygroundReleaseGuard()
    {
        Loop.Release();
    }

    FPlaygroundLoop& Loop;
};

int32 PlaygroundMain(const CHAR* Args[], int32 NumArgs)
{
    FPlaygroundLoop           Loop;
    FPlaygroundReleaseGuard   ReleaseGuard(Loop);

    int32 ErrorCode = Loop.PreInit(Args, NumArgs);
    if (ErrorCode != 0)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FPlaygroundLoop::PreInit failed");
        return ErrorCode;
    }

    ErrorCode = Loop.Init();
    if (ErrorCode != 0)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FPlaygroundLoop::Init failed");
        return ErrorCode;
    }

    while (!IsEngineExitRequested())
    {
        Loop.Tick();
    }

    return 0;
}
