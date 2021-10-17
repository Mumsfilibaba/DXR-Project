#pragma once
#include "Assets/SceneData.h"

#include "Core/CoreAPI.h"
#include "Core/Containers/String.h"

class CORE_API CDDSLoader
{
public:
    static TSharedPtr<SImage2D> LoadFile( const CString& Filename );
};
