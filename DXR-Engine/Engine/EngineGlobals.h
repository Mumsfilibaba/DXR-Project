#pragma once
#include <Containers/Types.h>

extern class EngineLoop GlobalEngineLoop;

extern class GenericWindow*       GlobalMainWindow;
extern class GenericApplication*  GlobalPlatformApplication;
extern class EventDispatcher*     GlobalEventDispatcher;
extern class GenericOutputDevice* GlobalConsoleOutput;

extern class Game* GlobalGame;

extern class Renderer*           GlobalRenderer;
extern class GenericRenderLayer* GlobalRenderLayer;
extern class IShaderCompiler*    GlobalShaderCompiler;
extern class CommandListExecutor GlobalCmdListExecutor;

extern Bool GlobalPrePassEnabled;
extern Bool GlobalDrawAABBs;
extern Bool GlobalVSyncEnabled;
extern Bool GlobalFrustumCullEnabled;
extern Bool GlobalRayTracingEnabled;

extern class Profiler GlobalProfiler;
extern class Console  GlobalConsole;