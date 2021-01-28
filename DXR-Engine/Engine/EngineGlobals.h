#pragma once
#include <Containers/Types.h>

extern class EngineLoop GlobalEngineLoop;
extern class Renderer   GlobalRenderer;

extern class GenericWindow*       GlobalMainWindow;
extern class GenericApplication*  GlobalPlatformApplication;
extern class GenericOutputDevice* GlobalConsoleOutput;
extern class EventDispatcher*     GlobalEventDispatcher;

extern class Game* GlobalGame;

extern class GenericRenderLayer* GlobalRenderLayer;
extern class IShaderCompiler*    GlobalShaderCompiler;
extern class CommandListExecutor GlobalCmdListExecutor;

extern class Profiler GlobalProfiler;
extern class Console  GlobalConsole;