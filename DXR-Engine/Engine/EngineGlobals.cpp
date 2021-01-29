#include "EngineGlobals.h"

#include "Main/EngineLoop.h"

#include "Debug/Profiler.h"
#include "Debug/Console.h"

#include "Rendering/Renderer.h"

#include "RenderLayer/CommandList.h"

EngineLoop gEngineLoop;
Renderer   gRenderer;

class GenericWindow*       gMainWindow          = nullptr;
class GenericApplication*  gPlatformApplication = nullptr;
class GenericOutputDevice* gConsoleOutput       = nullptr;

class EventDispatcher* gEventDispatcher = nullptr;

class Game* gGame = nullptr;

class GenericRenderLayer* gRenderLayer    = nullptr;
class IShaderCompiler*    gShaderCompiler = nullptr;

CommandListExecutor gCmdListExecutor;

Profiler gProfiler;
Console  gConsole;
