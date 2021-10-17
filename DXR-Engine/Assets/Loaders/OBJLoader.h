#pragma once
#include "Core/CoreAPI.h"
#include "Core/Containers/String.h"

#include "Assets/SceneData.h"

class CORE_API COBJLoader
{
public:
    static bool LoadFile( const CString& Filename, SSceneData& OutScene, bool ReverseHandedness = false );
};