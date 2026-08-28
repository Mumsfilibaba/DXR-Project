#pragma once
#include <Core/CoreTypes.h>
#include <Core/Containers/Array.h>
#include <Core/Containers/SharedPtr.h>
#include <Core/Time/ElapsedTime.h>
#include <RHI/RHICommandList.h>
#include <RHI/RHIResources.h>

#include "PlaygroundScene.h"

class FApplicationRenderer;
class FMenuInputHandler;
class FPlaygroundShell;
class FWindow;
struct FConsoleToggleHandler;

struct FPlaygroundSurface
{
    FPlaygroundSurface()
        : Window(nullptr)
        , SwapChain(nullptr)
        , Size()
    {
    }

    TSharedPtr<FWindow> Window;
    FRHISwapChainRef    SwapChain;
    IntVector2          Size;
};

class FPlaygroundLoop
{
public:
    FPlaygroundLoop();
    ~FPlaygroundLoop();


    int32 PreInit(const CHAR** Args, int32 NumArgs);
    int32 Init();
    void Tick();
    void Release();

private:
    bool LoadFonts();
    bool CreateMainWindow();
    void AttachConsole();
    void SyncSurfaces();
    void SyncSurfaceSize(FPlaygroundSurface& Surface);

    NODISCARD FRHISwapChainRef CreateSwapChain(const TSharedPtr<FWindow>& InWindow) const;

    void RenderSurface(FPlaygroundSurface& Surface);
    void OnMainWindowClosed();

    FElapsedTime                      FrameTimer;
    FRHICommandList                   CommandList;
    FPlaygroundFonts                  Fonts;
    TArray<FPlaygroundScene>          Scenes;
    TArray<FPlaygroundSurface>        Surfaces;
    TSharedPtr<FWindow>               MainWindow;
    TSharedPtr<FPlaygroundShell>      Shell;
    TSharedPtr<FApplicationRenderer>  Renderer;
    TSharedPtr<FMenuInputHandler>     MenuInputHandler;
    TSharedPtr<FConsoleToggleHandler> ConsoleToggleHandler;
    bool                              bIsRHIInitialized;
};

int32 PlaygroundMain(const CHAR* Args[], int32 NumArgs);
