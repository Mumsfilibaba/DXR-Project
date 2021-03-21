#include "EngineGlobals.h"

#include "Main/EngineLoop.h"

#include "Debug/Profiler.h"
#include "Debug/Console.h"

#include "Rendering/Renderer.h"

#include "RenderLayer/CommandList.h"

class GenericWindow* gMainWindow  = nullptr;

class EventDispatcher* gEventDispatcher = nullptr;

class Game* gGame = nullptr;

class GenericRenderLayer* gRenderLayer    = nullptr;
class IShaderCompiler*    gShaderCompiler = nullptr;
