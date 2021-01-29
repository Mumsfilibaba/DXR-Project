#pragma once
#include <Containers/Types.h>

extern class EngineLoop gEngineLoop;
extern class Renderer   gRenderer;

extern class GenericWindow*       gMainWindow;
extern class GenericApplication*  gApplication;
extern class GenericOutputDevice* gConsoleOutput;
extern class EventDispatcher*     gEventDispatcher;

extern class Game* gGame;

extern class GenericRenderLayer* gRenderLayer;
extern class IShaderCompiler*    gShaderCompiler;
extern class CommandListExecutor gCmdListExecutor;

extern class Profiler gProfiler;
extern class Console  gConsole;