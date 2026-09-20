#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Core/Time/ElapsedTime.h"

class FApplicationRenderer;
class FWindow;

class FEngineLoop
{
public:
    FEngineLoop();
    ~FEngineLoop();

    /* Initialize engine systems needed by other systems */
    int32 PreInit(const CHAR** Args, int32 NumArgs);

    /* Initialize engine systems */
    int32 Init();

    /* Load up modules always needed */
    bool LoadCoreModules();

    /* Advance the engine a frame */
    void Tick();

    /* Release engine systems */
    void Release();

private:
    bool CreateApplicationRenderer();
    void RedrawWindowDuringResize(const TSharedPtr<FWindow>& Window);

    TSharedPtr<FApplicationRenderer> UIRenderer;
    FElapsedTime                     FrameTimer;
    uint64                           FrameCounter;
    bool                             bIsRedrawingForResize;
};

extern FEngineLoop GEngineLoop;