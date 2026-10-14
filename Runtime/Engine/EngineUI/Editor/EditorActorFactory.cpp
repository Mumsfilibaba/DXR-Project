#include "Engine/EngineUI/Editor/EditorActorFactory.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Filesystem/File.h"
#include "Core/Math/Math.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Misc/Paths.h"
#include "Core/Templates/CString.h"
#include "Engine/Engine.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/EngineContent.h"
#include "Engine/Assets/EnvironmentAssetFactory.h"
#include "Engine/Resources/Model.h"
#include "Engine/World/World.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Actors/CameraActor.h"
#include "Engine/World/Actors/DirectionalLightActor.h"
#include "Engine/World/Actors/PointLightActor.h"
#include "Engine/World/Actors/SkyActor.h"
#include "Engine/World/Actors/SkyLightActor.h"
#include "Engine/World/Actors/SpotLightActor.h"
#include "Engine/World/Components/SkyLightComponent.h"
#include "Engine/World/Components/SkyboxComponent.h"
#include "Engine/World/Components/PrimitiveShapeComponent.h"
#include "Engine/World/Components/StaticMeshComponent.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"

// Placed upright: the plane is built facing -Z, so it is turned to lie flat
static const FVector3 GPrimitiveRotations[PrimitiveShapes::NumShapes] =
{
    FVector3(0.0f, 0.0f, 0.0f),
    FVector3(0.0f, 0.0f, 0.0f),
    FVector3(Math::Constants::HalfPI, 0.0f, 0.0f),
    FVector3(0.0f, 0.0f, 0.0f),
    FVector3(0.0f, 0.0f, 0.0f),
    FVector3(0.0f, 0.0f, 0.0f),
    FVector3(0.0f, 0.0f, 0.0f),
    FVector3(0.0f, 0.0f, 0.0f),
    FVector3(0.0f, 0.0f, 0.0f),
};

static const EEditorLightType GLightTypes[] =
{
    EEditorLightType::Point,
    EEditorLightType::Spot,
    EEditorLightType::Directional,
    EEditorLightType::SkyLight,
    EEditorLightType::Sky,
};

static TSharedPtr<FEnvironment> GDefaultSky;

static float ComputeGroundOffset(const FAABB& Bounds, const FMatrix4& OrientationMatrix)
{
    float LowestY = FLT_MAX;

    for (int32 Corner = 0; Corner < 8; ++Corner)
    {
        const FVector3 LocalCorner = FVector3(
            (Corner & 1) ? Bounds.Max.X : Bounds.Min.X,
            (Corner & 2) ? Bounds.Max.Y : Bounds.Min.Y,
            (Corner & 4) ? Bounds.Max.Z : Bounds.Min.Z);

        LowestY = Math::Min(LowestY, OrientationMatrix.TransformCoord(LocalCorner).Y);
    }

    return (LowestY < FLT_MAX) ? -LowestY : 0.0f;
}

// The scene lights with one sky light
static bool HasSkyLight(FWorld* World)
{
    if (World)
    {
        for (FActor* Actor : World->GetActors())
        {
            if (Actor && Actor->GetComponentOfType<FSkyLightComponent>())
            {
                return true;
            }
        }
    }

    return false;
}

static bool HasSkybox(FWorld* World)
{
    if (World)
    {
        for (FActor* Actor : World->GetActors())
        {
            if (Actor && Actor->GetComponentOfType<FSkyboxComponent>())
            {
                return true;
            }
        }
    }

    return false;
}

static TSharedPtr<FEnvironment> GetDefaultSky()
{
    if (!GDefaultSky)
    {
        GDefaultSky = FAssetManager::Get().LoadAsset<FEnvironment>(EngineContent::DefaultSky);
    }

    if (!GDefaultSky)
    {
        const FString Filename = File::CombinePath(File::CombinePath(Paths::GetEngineDir(), FAssetImporter::ContentSourcesFolder), EngineContent::DefaultSkySource);
        LOG_WARNING("[EditorActorFactory]: The engine content's DefaultSky is missing, so it is baked from '%s'; run Engine.ImportEngineContent to create the engine content", *Filename);

        FString Error;
        GDefaultSky = FEnvironmentAssetFactory::CreateFromPanoramaFile(Filename, 1024, Error);
        if (!GDefaultSky)
        {
            LOG_ERROR("[EditorActorFactory]: The default sky '%s' couldn't be baked: %s", *Filename, *Error);
        }
    }

    return GDefaultSky;
}

FActor* EditorActorFactory::SpawnPrimitive(FWorld* World, const FPrimitiveShape& Shape, const FVector3& Location)
{
    if (!World)
    {
        return nullptr;
    }

    FActor* NewActor = World->CreateActor();
    if (!NewActor)
    {
        return nullptr;
    }

    NewActor->SetName(CreateUniqueActorName(World, GetPrimitiveName(Shape.Type)));
    NewActor->GetTransform().SetRotation(GPrimitiveRotations[static_cast<int32>(Shape.Type)]);
    NewActor->GetTransform().SetTranslation(Location);

    FPrimitiveShapeComponent* NewComponent = NewObject<FPrimitiveShapeComponent>();
    if (NewComponent)
    {
        NewComponent->SetShape(Shape);
        NewActor->AddComponent(NewComponent);

        if (NewComponent->GetMesh())
        {
            RestOnGround(NewActor, Location);
        }
        else
        {
            TSharedPtr<bool> bGrounded = MakeSharedPtr<bool>(false);
            NewComponent->GetOnMeshChanged().AddLambda([NewActor, Location, bGrounded]()
            {
                if (!*bGrounded)
                {
                    *bGrounded = true;
                    RestOnGround(NewActor, Location);
                }
            });
        }
    }

    return NewActor;
}

void EditorActorFactory::RestOnGround(FActor* Actor, const FVector3& Location)
{
    FStaticMeshComponent* Component = Actor ? Actor->GetComponentOfType<FStaticMeshComponent>() : nullptr;
    if (!Component || !Component->GetMesh())
    {
        return;
    }

    FActorTransform& Transform = Actor->GetTransform();
    Transform.SetTranslation(Location);

    const float GroundOffset = ComputeGroundOffset(Component->GetMesh()->GetAABB(), Transform.GetTransformMatrix());
    Transform.SetTranslation(Location + FVector3(0.0f, GroundOffset, 0.0f));
}

FActor* EditorActorFactory::SpawnLight(FWorld* World, EEditorLightType Type, const FVector3& Location)
{
    if (!World)
    {
        return nullptr;
    }

    const FString ActorName = CreateUniqueActorName(World, GetLightName(Type));

    FActor* NewActor = nullptr;
    switch (Type)
    {
        case EEditorLightType::Point:
        {
            NewActor = World->SpawnActor<FPointLightActor>(Location, true);
            break;
        }

        case EEditorLightType::Spot:
        {
            NewActor = World->SpawnActor<FSpotLightActor>();
            if (NewActor)
            {
                NewActor->GetTransform().SetTranslation(Location);
            }

            break;
        }

        case EEditorLightType::Directional:
        {
            NewActor = World->SpawnActor<FDirectionalLightActor>(FVector3(Math::DegreesToRadians(35.0f), Math::DegreesToRadians(135.0f), 0.0f));
            break;
        }

        case EEditorLightType::SkyLight:
        {
            if (!HasSkyLight(World))
            {
                NewActor = World->SpawnActor<FSkyLightActor>();
            }

            break;
        }

        case EEditorLightType::Sky:
        {
            if (!HasSkyLight(World) && !HasSkybox(World))
            {
                NewActor = World->SpawnActor<FSkyActor>(GetDefaultSky());
            }

            break;
        }
    }

    if (NewActor)
    {
        NewActor->SetName(ActorName);
    }

    return NewActor;
}

FActor* EditorActorFactory::SpawnCamera(FWorld* World, const FVector3& Location)
{
    if (!World)
    {
        return nullptr;
    }

    const FString ActorName = CreateUniqueActorName(World, TEXT("Camera"));

    FCameraActor* NewActor = World->SpawnActor<FCameraActor>(Location, FVector3(0.0f, 0.0f, 0.0f));
    if (NewActor)
    {
        NewActor->SetName(ActorName);
    }

    return NewActor;
}

bool EditorActorFactory::CanSpawnLight(FWorld* World, EEditorLightType Type)
{
    switch (Type)
    {
        case EEditorLightType::Spot:
        {
            return false;
        }

        case EEditorLightType::SkyLight:
        {
            return World && !HasSkyLight(World);
        }

        case EEditorLightType::Sky:
        {
            return World && !HasSkyLight(World) && !HasSkybox(World);
        }

        default:
        {
            return true;
        }
    }
}

FActor* EditorActorFactory::DrawPlaceActorMenu(FWorld* World, const FVector3& Location)
{
    FActor* SpawnedActor = nullptr;

    {
        FSubMenuState MeshSubMenu;
        if (EditorWidgets::BeginSubMenu(MeshSubMenu, TEXT("##PlaceActorMeshMenu"), TEXT("Mesh")))
        {
            for (int32 Index = 0; Index < PrimitiveShapes::NumShapes; ++Index)
            {
                const EPrimitiveShape Type = static_cast<EPrimitiveShape>(Index);
                if (EditorWidgets::MenuItem(GetPrimitiveName(Type)))
                {
                    SpawnedActor = SpawnPrimitive(World, PrimitiveShapes::MakeDefault(Type), Location);
                }
            }

            EditorWidgets::EndSubMenu(MeshSubMenu);
        }
    }

    {
        FSubMenuState LightSubMenu;
        if (EditorWidgets::BeginSubMenu(LightSubMenu, TEXT("##PlaceActorLightMenu"), TEXT("Light")))
        {
            for (const EEditorLightType Type : GLightTypes)
            {
                const bool bEnabled = CanSpawnLight(World, Type);
                if (EditorWidgets::MenuItem(GetLightName(Type), nullptr, false, bEnabled))
                {
                    SpawnedActor = SpawnLight(World, Type, Location);
                }
            }

            EditorWidgets::EndSubMenu(LightSubMenu);
        }
    }

    if (EditorWidgets::MenuItem(TEXT("Camera")))
    {
        SpawnedActor = SpawnCamera(World, Location);
    }

    return SpawnedActor;
}

FString EditorActorFactory::CreateUniqueActorName(FWorld* World, const CHAR_T* BaseName)
{
    FString Name = BaseName;
    if (!World)
    {
        return Name;
    }

    auto IsNameTaken = [World](const FString& Candidate) -> bool
    {
        for (FActor* Actor : World->GetActors())
        {
            if (Actor && Actor->GetName() == Candidate)
            {
                return true;
            }
        }

        return false;
    };

    int32 Suffix = 0;
    TStaticArray<CHAR_T, 128> Buffer{};

    while (IsNameTaken(Name))
    {
        ++Suffix;
        CString::Snprintf(Buffer.Data(), static_cast<int32>(Buffer.Size()), TEXT("%s (%d)"), BaseName, Suffix);
        Name = Buffer.Data();
    }

    return Name;
}

void EditorActorFactory::ReleaseCachedResources()
{
    GDefaultSky.Reset();
}

const CHAR_T* EditorActorFactory::GetPrimitiveName(EPrimitiveShape Type)
{
    return PrimitiveShapes::GetDesc(Type).Name;
}

const CHAR_T* EditorActorFactory::GetLightName(EEditorLightType Type)
{
    switch (Type)
    {
        case EEditorLightType::Point:       return TEXT("Point Light");
        case EEditorLightType::Spot:        return TEXT("Spot Light");
        case EEditorLightType::Directional: return TEXT("Directional Light");
        case EEditorLightType::SkyLight:    return TEXT("Sky Light");
        case EEditorLightType::Sky:         return TEXT("Sky");
        default:                            return TEXT("Light");
    }
}
