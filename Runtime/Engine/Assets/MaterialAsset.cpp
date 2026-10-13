#include "AssetCore/AssetType.h"
#include "Core/Json/Json.h"
#include "Core/Json/JsonGuid.h"
#include "Core/Json/JsonMath.h"
#include "Core/Json/JsonSerializer.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/MaterialAsset.h"
#include "Engine/Engine.h"

struct FMaterialFlagName
{
    EMaterialFlags Flag;
    const CHAR_T*  Name;
};

static constexpr FMaterialFlagName MaterialFlagNames[] =
{
    { EMaterialFlags::EnableHeight,           TEXT("EnableHeight") },
    { EMaterialFlags::EnableAlpha,            TEXT("EnableAlpha") },
    { EMaterialFlags::EnableNormalMapping,    TEXT("EnableNormalMapping") },
    { EMaterialFlags::EnableParallaxClipping, TEXT("EnableParallaxClipping") },
    { EMaterialFlags::NormalMapPositiveY,     TEXT("NormalMapPositiveY") },
    { EMaterialFlags::DoubleSided,            TEXT("DoubleSided") },
    { EMaterialFlags::ForceForwardPass,       TEXT("ForceForwardPass") },
    { EMaterialFlags::Translucent,            TEXT("Translucent") },
    { EMaterialFlags::EnableRefraction,       TEXT("EnableRefraction") },
};

static constexpr const CHAR_T* TextureSlotNames[] =
{
    TEXT("BaseColor"), TEXT("Normal"), TEXT("Height"), TEXT("MaskA"), TEXT("MaskB"), TEXT("MaskC"), TEXT("MaskD"),
};

static constexpr const CHAR_T* ScalarNames[] =
{
    TEXT("Roughness"), TEXT("Metallic"), TEXT("Occlusion"), TEXT("Opacity"),
};

static_assert(ARRAY_COUNT(TextureSlotNames) == EMaterialTextureSlot::Count, "Every texture slot needs a name");
static_assert(ARRAY_COUNT(ScalarNames) == EMaterialScalar::Count, "Every scalar needs a name");

JSON_ENUM_NAMES_BEGIN(ETextureChannel)
    JSON_ENUM_NAME(ETextureChannel, R)
    JSON_ENUM_NAME(ETextureChannel, G)
    JSON_ENUM_NAME(ETextureChannel, B)
    JSON_ENUM_NAME(ETextureChannel, A)
JSON_ENUM_NAMES_END()

JSON_ENUM_NAMES_BEGIN(EMaterialTextureSlot::Type)
    JSON_ENUM_NAME(EMaterialTextureSlot::Type, BaseColor)
    JSON_ENUM_NAME(EMaterialTextureSlot::Type, Normal)
    JSON_ENUM_NAME(EMaterialTextureSlot::Type, Height)
    JSON_ENUM_NAME(EMaterialTextureSlot::Type, MaskA)
    JSON_ENUM_NAME(EMaterialTextureSlot::Type, MaskB)
    JSON_ENUM_NAME(EMaterialTextureSlot::Type, MaskC)
    JSON_ENUM_NAME(EMaterialTextureSlot::Type, MaskD)
    { EMaterialTextureSlot::Count, TEXT("None") },
JSON_ENUM_NAMES_END()

template<>
struct TJsonSerializer<EMaterialFlags>
{
    static void Save(FJsonValue& OutValue, const EMaterialFlags& InValue)
    {
        OutValue = FJsonValue::MakeArray();
        for (const FMaterialFlagName& Entry : MaterialFlagNames)
        {
            if (IsEnumFlagSet(InValue, Entry.Flag))
            {
                OutValue.Add(FJsonValue(Entry.Name));
            }
        }
    }

    static bool Load(const FJsonValue& InValue, EMaterialFlags& OutValue, FJsonArchive& Archive)
    {
        if (!InValue.IsArray())
        {
            Archive.AddTypeError(InValue, TEXT("an array of flag names"));
            return false;
        }

        EMaterialFlags Flags = EMaterialFlags::None;
        for (int32 Index = 0; Index < InValue.Num(); ++Index)
        {
            FString Name;
            if (!InValue[Index].TryGetString(Name))
            {
                Archive.AddTypeError(InValue[Index], TEXT("a flag name"));
                return false;
            }

            bool bFound = false;
            for (const FMaterialFlagName& Entry : MaterialFlagNames)
            {
                if (Name.Equals(Entry.Name))
                {
                    Flags |= Entry.Flag;
                    bFound = true;
                }
            }

            if (!bFound)
            {
                Archive.AddError(TEXT("'%s' is not a material flag"), Name.Data());
                return false;
            }
        }

        OutValue = Flags;
        return true;
    }
};

static void SerializeRoute(FJsonArchive& Archive, FMaterialTextureRoute& Route)
{
    Archive.Field(TEXT("Slot"), Route.Slot);
    Archive.Field(TEXT("Channel"), Route.Channel, ETextureChannel::R);
    Archive.Field(TEXT("Invert"), Route.bInvert, false);
}

template<>
struct TJsonSerializer<FMaterialTextureRoute>
{
    static void Save(FJsonValue& OutValue, const FMaterialTextureRoute& InValue)
    {
        FMaterialTextureRoute Route   = InValue;
        FJsonArchive          Archive = FJsonArchive::Saver(OutValue);
        SerializeRoute(Archive, Route);
    }

    static bool Load(const FJsonValue& InValue, FMaterialTextureRoute& OutValue, FJsonArchive& Archive)
    {
        if (!InValue.IsObject())
        {
            Archive.AddTypeError(InValue, TEXT("an object"));
            return false;
        }

        FMaterialTextureRoute Route;
        FJsonArchive          Nested = FJsonArchive::Loader(InValue);
        SerializeRoute(Nested, Route);
        Archive.AppendErrors(Nested);

        OutValue = Route;
        return !Nested.HasErrors();
    }
};

void FMaterialAssetData::Serialize(FJsonArchive& Archive)
{
    const FMaterialInfo Defaults;

    if (Archive.IsSaving())
    {
        Archive.SetVersion(MaterialAssets::CurrentVersion);
    }

    FString Type = AssetTypes::GetName(EAssetType::Material);
    Archive.Field(TEXT("Guid"), Guid);
    Archive.Field(TEXT("Type"), Type);

    if (Archive.IsLoading() && !Type.Equals(AssetTypes::GetName(EAssetType::Material)))
    {
        Archive.AddError(TEXT("Type is '%s', not Material"), Type.Data());
    }

    Archive.Field(TEXT("Albedo"), Info.Albedo, Defaults.Albedo);
    Archive.Field(TEXT("Roughness"), Info.Roughness, Defaults.Roughness);
    Archive.Field(TEXT("Metallic"), Info.Metallic, Defaults.Metallic);
    Archive.Field(TEXT("AmbientOcclusion"), Info.AmbientOcclusion, Defaults.AmbientOcclusion);
    Archive.Field(TEXT("Opacity"), Info.Opacity, Defaults.Opacity);
    Archive.Field(TEXT("Flags"), Info.MaterialFlags, Defaults.MaterialFlags);
    Archive.Field(TEXT("ParallaxHeightScale"), Info.ParallaxHeightScale, Defaults.ParallaxHeightScale);
    Archive.Field(TEXT("ParallaxMinLayers"), Info.ParallaxMinLayers, Defaults.ParallaxMinLayers);
    Archive.Field(TEXT("ParallaxMaxLayers"), Info.ParallaxMaxLayers, Defaults.ParallaxMaxLayers);
    Archive.Field(TEXT("IndexOfRefraction"), Info.IndexOfRefraction, Defaults.IndexOfRefraction);
    Archive.Field(TEXT("RefractionStrength"), Info.RefractionStrength, Defaults.RefractionStrength);

    bool bHasRoutes = Archive.IsLoading();
    for (int32 Scalar = 0; Scalar < EMaterialScalar::Count; ++Scalar)
    {
        bHasRoutes = bHasRoutes || !(Info.Routes[Scalar] == Defaults.Routes[Scalar]);
    }

    if (bHasRoutes)
    {
        FJsonArchive::FObjectScope Routes(Archive, TEXT("Routes"));
        for (int32 Scalar = 0; Scalar < EMaterialScalar::Count; ++Scalar)
        {
            Archive.Field(ScalarNames[Scalar], Info.Routes[Scalar], Defaults.Routes[Scalar]);
        }
    }

    bool bHasTextures = Archive.IsLoading();
    for (const TAssetRef<FTexture>& Texture : Textures)
    {
        bHasTextures = bHasTextures || Texture.IsSet();
    }

    if (bHasTextures)
    {
        FJsonArchive::FObjectScope TexturesScope(Archive, TEXT("Textures"));
        for (int32 Slot = 0; Slot < EMaterialTextureSlot::Count; ++Slot)
        {
            Archive.Field(TextureSlotNames[Slot], Textures[Slot]);
        }
    }

    Archive.Field(TEXT("EmissiveColor"), EmissiveColor, FFloatColor(0.0f, 0.0f, 0.0f, 1.0f));
    Archive.Field(TEXT("EmissiveIntensity"), EmissiveIntensity, 1.0f);
    Archive.Field(TEXT("EmissiveTexture"), EmissiveTexture);
    Archive.Field(TEXT("MultiplyVertexColor"), bMultiplyVertexColor, false);

    if (Archive.IsSaving())
    {
        TArray<FGuid> Dependencies;
        for (const TAssetRef<FTexture>& Texture : Textures)
        {
            if (Texture.IsSet())
            {
                Dependencies.AddUnique(Texture.GetGuid());
            }
        }

        if (EmissiveTexture.IsSet())
        {
            Dependencies.AddUnique(EmissiveTexture.GetGuid());
        }

        Archive.Field(TEXT("Dependencies"), Dependencies, TArray<FGuid>());
    }
}

FJsonValue MaterialAssets::ToJson(const FMaterialAssetData& Material)
{
    FJsonValue Document;
    TJsonSerializer<FMaterialAssetData>::Save(Document, Material);
    return Document;
}

bool MaterialAssets::FromJson(const FJsonValue& Value, FMaterialAssetData& OutMaterial, FString& OutError)
{
    if (!Value.IsObject())
    {
        OutError = FString(TEXT("it isn't a JSON object"));
        return false;
    }

    FJsonArchive Loader = FJsonArchive::Loader(Value);

    const int32 Version = Loader.GetVersion();
    if (Version < 1)
    {
        OutError = FString(TEXT("it has no Version"));
        return false;
    }

    if (Version > CurrentVersion)
    {
        OutError = FString::Printf(TEXT("it is version %d, but this engine reads up to version %d"), Version, CurrentVersion);
        return false;
    }

    FMaterialAssetData Material;
    Material.Serialize(Loader);

    if (Loader.HasErrors())
    {
        OutError = Loader.GetErrorString();
        return false;
    }

    if (!Material.Guid.IsValid())
    {
        OutError = FString(TEXT("it has no Guid"));
        return false;
    }

    OutMaterial = ::Move(Material);
    return true;
}

bool MaterialAssets::SaveToFile(const FString& Filename, const FMaterialAssetData& Material, FString& OutError)
{
    if (!Json::SaveToFile(Filename, ToJson(Material)))
    {
        OutError = FString::Printf(TEXT("'%s' couldn't be written"), *Filename);
        return false;
    }

    return true;
}

bool MaterialAssets::LoadFromFile(const FString& Filename, FMaterialAssetData& OutMaterial, FString& OutError)
{
    FJsonValue Document;
    FJsonError ParseError;

    if (!Json::LoadFromFile(Filename, Document, &ParseError))
    {
        OutError = FString::Printf(TEXT("'%s' isn't valid JSON: %s"), *Filename, *ParseError.ToString());
        return false;
    }

    if (!FromJson(Document, OutMaterial, OutError))
    {
        OutError = FString::Printf(TEXT("'%s' isn't a valid material: %s"), *Filename, *OutError);
        return false;
    }

    return true;
}

TSharedPtr<FMaterial> MaterialAssets::Load(const FAssetData& Asset, const FString& DiskPath, FString& OutError)
{
    FMaterialAssetData Data;
    if (!LoadFromFile(DiskPath, Data, OutError))
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
        TSharedPtr<FTexture> Texture   = FAssetManager::Get().GetAssetLoader().Load<FTexture>(Data.Textures[Slot], &Error);
        FTexture2D*          Texture2D = Texture ? Texture->GetTexture2D() : nullptr;

        if (!Texture2D)
        {
            LOG_WARNING("[MaterialAssets]: The %s texture of '%s' is left out: %s", TextureSlotNames[Slot], *Asset.Path, Texture ? TEXT("it isn't a 2D texture") : *Error);
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
