#include "EngineGlobals.h"

#include "Main/EngineLoop.h"

#include "Debug/Profiler.h"
#include "Debug/Console.h"

#include "Rendering/Renderer.h"

#include "RenderLayer/CommandList.h"

EngineLoop GlobalEngineLoop;
Renderer   GlobalRenderer;

class GenericWindow*       GlobalMainWindow          = nullptr;
class GenericApplication*  GlobalPlatformApplication = nullptr;
class GenericOutputDevice* GlobalConsoleOutput       = nullptr;

class EventDispatcher* GlobalEventDispatcher = nullptr;

class Game* GlobalGame = nullptr;

class GenericRenderLayer* GlobalRenderLayer    = nullptr;
class IShaderCompiler*    GlobalShaderCompiler = nullptr;

CommandListExecutor GlobalCmdListExecutor;

Profiler GlobalProfiler;
Console  GlobalConsole;
