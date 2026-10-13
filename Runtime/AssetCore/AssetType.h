#pragma once
#include "Core/Containers/String.h"

enum class EAssetType : uint32
{
    Unknown  = 0,
    Texture  = 1,
    Mesh     = 2,
    Material = 3,
    Model    = 4,
    Scene    = 5,
};

struct ASSETCORE_API AssetTypes
{
    static constexpr const CHAR_T* MaterialExtension = TEXT(".dxrmaterial");
    static constexpr const CHAR_T* ModelExtension    = TEXT(".dxrmodel");
    static constexpr const CHAR_T* SceneExtension    = TEXT(".dxrscene");

    /** @return Returns the type's name, as text assets write it, such as "Material" */
    NODISCARD static const CHAR_T* GetName(EAssetType Type);

    /** @return Returns the type with Name, or EAssetType::Unknown */
    NODISCARD static EAssetType FindByName(const FString& Name);

    /** @return Returns the type a text asset with Extension holds, such as Material for ".dxrmaterial", or EAssetType::Unknown */
    NODISCARD static EAssetType FindByTextExtension(const FString& Extension);

    /** @return Returns the extension of the text files assets of Type are, such as ".dxrmaterial", or nullptr when they are .dxrasset files */
    NODISCARD static const CHAR_T* GetTextExtension(EAssetType Type);
};
