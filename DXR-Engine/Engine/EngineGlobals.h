#pragma once
#include "Core/Types.h"

extern class Renderer gRenderer;

extern class GenericWindow*       gMainWindow;
extern class GenericApplication*  gApplication;
extern class GenericOutputDevice* gConsoleOutput;

extern class EventDispatcher* gEventDispatcher;

extern class Game* gGame;

extern class GenericRenderLayer* gRenderLayer;
extern class IShaderCompiler*    gShaderCompiler;
extern class CommandListExecutor gCmdListExecutor;

extern class Console gConsole;