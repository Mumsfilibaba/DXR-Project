#include "AssetCore/MaterialAsset.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Engine/Engine.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/MaterialAssetLoader.h"
#include "Engine/Resources/Texture.h"

TSharedPtr<FMaterial> MaterialAssetLoader::Load(const FAssetData& Asset, const FString& DiskPath, FString& OutError)
{
    FMaterialAssetData Data;
    if (!MaterialAssets::LoadFromFile(DiskPath, Data, OutError))
    {
        return nullptr;
    }

    TSharedPtr<FMaterial> Material = MakeSharedPtr<FMaterial>(Data.Info);
    for (int32 Slot = 0; Slot < EMaterialTextureSlot::Count; ++Slot)
    {
        if (!Data.Textures[Slot].IsSet())
        {
            continue;
        }

        FString              Error;
        TSharedPtr<FTexture> Texture   = FAssetManager::Get().GetAssetLoader().Load<FTexture>(TAssetRef<FTexture>(Data.Textures[Slot].GetGuid(), Data.Textures[Slot].GetPath()), &Error);
        FTexture2D*          Texture2D = Texture ? Texture->GetTexture2D() : nullptr;

        if (!Texture2D)
        {
            LOG_WARNING("[MaterialAssetLoader]: The %s texture of '%s' is left out: %s", MaterialAssets::GetTextureSlotName(EMaterialTextureSlot::Type(Slot)), *Asset.Path, Texture ? TEXT("it isn't a 2D texture") : *Error);
            continue;
        }

        Material->SetTexture(EMaterialTextureSlot::Type(Slot), Texture2D->GetRHITexture());
    }

    if (!Material->GetTexture(EMaterialTextureSlot::BaseColor))
    {
        Material->SetTexture(EMaterialTextureSlot::BaseColor, FEngine::Get()->BaseTexture);
    }

    Material->Initialize();
    Material->SetName(Asset.Name);
    return Material;
}
