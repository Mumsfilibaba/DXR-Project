#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"

#include "Engine/Assets/SceneData.h"

enum EFBXFlags : uint8
{
    FBXFlags_None             = 0,
    FBXFlags_ApplyScaleFactor = BIT(1),
    FBXFlags_EnsureLeftHanded = BIT(2),

    FBXFlags_Default = FBXFlags_EnsureLeftHanded
};

// TODO: Extend to save as well? 
class ENGINE_API FFBXLoader
{
public:
    static bool LoadFile(const FString& Filename, FSceneData& OutScene, uint32 Flags = EFBXFlags::FBXFlags_Default) noexcept;
};