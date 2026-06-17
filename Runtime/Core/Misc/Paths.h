#pragma once
#include "Core/Containers/String.h"

struct CORE_API Paths
{
    static String GetEngineDir();

    static String GetAssetDir();

    static String GetProjectDir();

    static String GetProjectName();

    static String GetProjectModuleName();
};