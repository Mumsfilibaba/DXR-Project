#pragma once
#include "Core.h"

#include "Assets/SceneData.h"

/* Flags for loading */
enum EFBXFlags : uint8
{
    FBXFlags_None = 0,
    FBXFlags_ApplyScaleFactor = BIT( 1 ),
    FBXFlags_EnsureLeftHanded = BIT( 2 ),

    FBXFlags_Default = FBXFlags_ApplyScaleFactor | FBXFlags_EnsureLeftHanded
};

// TODO: Extend to save as well? 
class CFBXLoader
{
public:
    static bool LoadFile( const String& Filename, SSceneData& OutScene, uint32 Flags = EFBXFlags::FBXFlags_Default ) noexcept;
};
