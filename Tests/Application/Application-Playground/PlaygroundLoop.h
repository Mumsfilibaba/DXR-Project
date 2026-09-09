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
    void OnMainWindowClosed();

    FElapsedTime                      FrameTimer;
    FPlaygroundFonts                  Fonts;
    TArray<FPlaygroundScene>          Scenes;
    TSharedPtr<FWindow>               MainWindow;
    TSharedPtr<FPlaygroundShell>      Shell;
    TSharedPtr<FApplicationRenderer>  Renderer;
    TSharedPtr<FMenuInputHandler>     MenuInputHandler;
    TSharedPtr<FConsoleToggleHandler> ConsoleToggleHandler;
    bool                              bIsRHIInitialized;
};

int32 PlaygroundMain(const CHAR* Args[], int32 NumArgs);
