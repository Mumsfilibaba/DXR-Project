#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Engine/Assets/SceneData.h"

enum class EFBXFlags : uint8
{
    None             = 0,
    ApplyScaleFactor = BIT(1),
    EnsureLeftHanded = BIT(2),

    Default = EnsureLeftHanded
};

ENUM_CLASS_OPERATORS(EFBXFlags);

// TODO: Extend to save as well? 
struct ENGINE_API FFBXLoader
{
    static TSharedRef<FSceneData> LoadFile(const FString& Filename, EFBXFlags Flags = EFBXFlags::Default) noexcept;
};
