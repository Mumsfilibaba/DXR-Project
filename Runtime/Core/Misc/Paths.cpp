#include "Core/Misc/Paths.h"
#include "Core/Platform/PlatformFile.h"

String Paths::GetEngineDir()
{
    return String(ENGINE_LOCATION);
}

String Paths::GetAssetDir()
{
    return Paths::GetEngineDir() + String("/Assets");
}

String Paths::GetProjectDir()
{
    return String(PROJECT_LOCATION);
}

String Paths::GetProjectName()
{
    return String(PROJECT_NAME);
}

String Paths::GetProjectModuleName()
{
    return String(PROJECT_NAME);
}

String Paths::GetUserSettingsDir()
{
    const String BaseDirectory = FPlatformFile::GetUserSettingsDirectory();
    return BaseDirectory.IsEmpty() ? String() : BaseDirectory + String("/DXR-Engine");
}
