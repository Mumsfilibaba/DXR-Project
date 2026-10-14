#pragma once
#include "Core/Containers/String.h"
#include "Core/Misc/Guid.h"
#include "Engine/EngineModule.h"

class FAssetImporter;

struct ENGINE_API EngineContent
{
    static constexpr FGuid BaseTexture  = FGuid(0x5c98d6b1, 0xc9cf457f, 0xad1ac7b2, 0xfa2bd0ca);
    static constexpr FGuid BaseNormal   = FGuid(0xa2a39e90, 0x273f4bdd, 0xb88c9db6, 0x4120c1c0);
    static constexpr FGuid BaseMaterial = FGuid(0x8f219b26, 0xd43b43fd, 0xadcbe3bb, 0x51fc867e);
    static constexpr FGuid DefaultSky   = FGuid(0x4fd82bd8, 0x58ba48be, 0x915610c3, 0xe1fb4116);

    /** @brief The default sky's source, under <Engine>/ContentSources */
    static constexpr const CHAR_T* DefaultSkySource = TEXT("Environments/DefaultSky.hdr");

    /**
     * @brief Write the content made in code, the base textures and the base material, into
     * ContentDirectory, the folder /Engine/ is mounted at
     */
    static bool WriteGeneratedContent(const FString& ContentDirectory, FString& OutError);

    /** @brief Import the default sky from <Engine>/ContentSources into /Engine/Environments with Importer, baking it on the GPU */
    static bool ImportDefaultSky(FAssetImporter& Importer, FString& OutError);
};
