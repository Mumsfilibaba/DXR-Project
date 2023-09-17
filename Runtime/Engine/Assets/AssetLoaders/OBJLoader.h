#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Engine/Assets/SceneData.h"

struct ENGINE_API FOBJLoader
{
    static bool LoadFile(const FString& Filename, FSceneData& OutScene, bool bReverseHandedness = false);
};