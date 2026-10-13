#include "AssetCore/AssetType.h"

static constexpr EAssetType NamedTypes[] =
{
    EAssetType::Texture,
    EAssetType::Mesh,
    EAssetType::Material,
    EAssetType::Model,
    EAssetType::Scene,
};

const CHAR_T* AssetTypes::GetName(EAssetType Type)
{
    switch (Type)
    {
        case EAssetType::Texture:  return TEXT("Texture");
        case EAssetType::Mesh:     return TEXT("Mesh");
        case EAssetType::Material: return TEXT("Material");
        case EAssetType::Model:    return TEXT("Model");
        case EAssetType::Scene:    return TEXT("Scene");
        default:                   return TEXT("Unknown");
    }
}

EAssetType AssetTypes::FindByName(const FString& Name)
{
    for (EAssetType Type : NamedTypes)
    {
        if (Name.Equals(GetName(Type)))
        {
            return Type;
        }
    }

    return EAssetType::Unknown;
}

const CHAR_T* AssetTypes::GetTextExtension(EAssetType Type)
{
    switch (Type)
    {
        case EAssetType::Material: return MaterialExtension;
        case EAssetType::Model:    return ModelExtension;
        case EAssetType::Scene:    return SceneExtension;
        default:                   return nullptr;
    }
}

EAssetType AssetTypes::FindByTextExtension(const FString& Extension)
{
    if (Extension.Equals(MaterialExtension, EStringCaseType::NoCase))
    {
        return EAssetType::Material;
    }

    if (Extension.Equals(ModelExtension, EStringCaseType::NoCase))
    {
        return EAssetType::Model;
    }

    if (Extension.Equals(SceneExtension, EStringCaseType::NoCase))
    {
        return EAssetType::Scene;
    }

    return EAssetType::Unknown;
}
