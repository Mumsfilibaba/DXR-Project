#include "PlaygroundScene.h"

void CreatePlaygroundScenes(const FPlaygroundFonts& Fonts, TArray<FPlaygroundScene>& OutScenes)
{
    OutScenes.Clear();
    OutScenes.Add(CreateFoundationsScene(Fonts));
    OutScenes.Add(CreateVectorsScene(Fonts));
    OutScenes.Add(CreateControlsScene(Fonts));
    OutScenes.Add(CreateMenusScene(Fonts));
    OutScenes.Add(CreateWindowsScene(Fonts));
    OutScenes.Add(CreateDockingScene(Fonts));
    OutScenes.Add(CreateOutputLogScene(Fonts));
    OutScenes.Add(CreateToolBarScene(Fonts));
    OutScenes.Add(CreateNodeGraphScene(Fonts));
    OutScenes.Add(CreateGizmoScene(Fonts));
    OutScenes.Add(CreateConsoleScene(Fonts));
    OutScenes.Add(CreateTextScene(Fonts));
}
