#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Engine/Assets/SceneData.h"

struct ENGINE_API FOBJLoader
{
    static TSharedRef<FSceneData> LoadFile(const FString& Filename, bool bReverseHandedness = false);
};
