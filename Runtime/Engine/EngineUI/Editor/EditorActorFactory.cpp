#include "Engine/EngineUI/Editor/EditorActorFactory.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Math/Math.h"
#include "Core/Templates/CString.h"
#include "Engine/Engine.h"
#include "Engine/Assets/MeshFactory.h"
#include "Engine/Resources/Model.h"
#include "Engine/World/World.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Actors/CameraActor.h"
#include "Engine/World/Actors/DirectionalLightActor.h"
#include "Engine/World/Actors/PointLightActor.h"
#include "Engine/World/Actors/SkyLightActor.h"
#include "Engine/World/Actors/SpotLightActor.h"
#include "Engine/World/Components/SkyLightComponent.h"
#include "Engine/World/Components/StaticMeshComponent.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"

constexpr int32 NumPrimitiveTypes = static_cast<int32>(EEditorPrimitiveType::Count);

struct FPrimitiveRecipe
{
    const CHAR* Name;
    Vector3     Rotation;
};

static const FPrimitiveRecipe GPrimitiveRecipes[NumPrimitiveTypes] =
{
    { "Cube",     Vector3(0.0f, 0.0f, 0.0f) },
    { "Sphere",   Vector3(0.0f, 0.0f, 0.0f) },
    { "Plane",    Vector3(Math::Constants::HalfPI, 0.0f, 0.0f) },
    { "Cylinder", Vector3(0.0f, 0.0f, 0.0f) },
    { "Cone",     Vector3(0.0f, 0.0f, 0.0f) },
    { "Torus",    Vector3(0.0f, 0.0f, 0.0f) },
    { "Pyramid",  Vector3(0.0f, 0.0f, 0.0f) },
    { "Teapot",   Vector3(0.0f, 0.0f, 0.0f) },
};

static const EEditorLightType GLightTypes[] =
{
    EEditorLightType::Point,
    EEditorLightType::Spot,
    EEditorLightType::Directional,
    EEditorLightType::Sky,
};

static TStaticArray<TSharedPtr<FMesh>, NumPrimitiveTypes> GPrimitiveMeshCache;

static FMeshData CreatePrimitiveMeshData(EEditorPrimitiveType Type)
{
    switch (Type)
    {
        case EEditorPrimitiveType::Cube:     return MeshFactory::CreateCube();
        case EEditorPrimitiveType::Sphere:   return MeshFactory::CreateSphere(3);
        case EEditorPrimitiveType::Plane:    return MeshFactory::CreatePlane(10, 10);
        case EEditorPrimitiveType::Cylinder: return MeshFactory::CreateCylinder();
        case EEditorPrimitiveType::Cone:     return MeshFactory::CreateCone();
        case EEditorPrimitiveType::Torus:    return MeshFactory::CreateTorus();
        case EEditorPrimitiveType::Pyramid:  return MeshFactory::CreatePyramid();
        case EEditorPrimitiveType::Teapot:   return MeshFactory::CreateTeapot();
        default:                             return FMeshData();
    }
}

static TSharedPtr<FMesh> GetOrCreateMesh(EEditorPrimitiveType Type)
{
    const int32 Index = static_cast<int32>(Type);
    if (Index < 0 || Index >= NumPrimitiveTypes)
    {
        return nullptr;
    }

    if (!GPrimitiveMeshCache[Index])
    {
        GPrimitiveMeshCache[Index] = FMesh::Create(CreatePrimitiveMeshData(Type));
    }

    return GPrimitiveMeshCache[Index];
}

static float ComputeGroundOffset(const FAABB& Bounds, const Matrix4& OrientationMatrix)
{
    float LowestY = FLT_MAX;

    for (int32 Corner = 0; Corner < 8; ++Corner)
    {
        const Vector3 LocalCorner = Vector3(
            (Corner & 1) ? Bounds.Max.X : Bounds.Min.X,
            (Corner & 2) ? Bounds.Max.Y : Bounds.Min.Y,
            (Corner & 4) ? Bounds.Max.Z : Bounds.Min.Z);

        LowestY = Math::Min(LowestY, OrientationMatrix.TransformCoord(LocalCorner).Y);
    }

    return (LowestY < FLT_MAX) ? -LowestY : 0.0f;
}

static FRHITextureRef FindSkyLightCubeMap(FWorld* World)
{
    if (World)
    {
        for (FActor* Actor : World->GetActors())
        {
            if (!Actor)
            {
                continue;
            }

            if (FSkyLightComponent* SkyLight = Actor->GetComponentOfType<FSkyLightComponent>())
            {
                if (SkyLight->GetCubeMap())
                {
                    return SkyLight->GetCubeMap();
                }
            }
        }
    }

    return FRHITextureRef();
}

FActor* EditorActorFactory::SpawnPrimitive(FWorld* World, EEditorPrimitiveType Type, const Vector3& Location)
{
    if (!World)
    {
        return nullptr;
    }

    const TSharedPtr<FMesh> Mesh = GetOrCreateMesh(Type);
    if (!Mesh)
    {
        return nullptr;
    }

    const String ActorName = MakeUniqueActorName(World, GetPrimitiveName(Type));

    FActor* NewActor = World->CreateActor();
    if (!NewActor)
    {
        return nullptr;
    }

    NewActor->SetName(ActorName);

    FActorTransform& Transform = NewActor->GetTransform();
    Transform.SetRotation(GPrimitiveRecipes[static_cast<int32>(Type)].Rotation);

    const float GroundOffset = ComputeGroundOffset(Mesh->GetAABB(), Transform.GetTransformMatrix());
    Transform.SetTranslation(Location + Vector3(0.0f, GroundOffset, 0.0f));

    FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
    if (NewComponent)
    {
        NewComponent->SetMesh(Mesh);
        NewComponent->SetMaterial(FEngine::Get()->BaseMaterial);
        NewActor->AddComponent(NewComponent);
    }

    return NewActor;
}

FActor* EditorActorFactory::SpawnLight(FWorld* World, EEditorLightType Type, const Vector3& Location)
{
    if (!World)
    {
        return nullptr;
    }

    const String ActorName = MakeUniqueActorName(World, GetLightName(Type));

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
            NewActor = World->SpawnActor<FDirectionalLightActor>(Vector3(Math::DegreesToRadians(35.0f), Math::DegreesToRadians(135.0f), 0.0f));
            break;
        }

        case EEditorLightType::Sky:
        {
            const FRHITextureRef CubeMap = FindSkyLightCubeMap(World);
            if (CubeMap)
            {
                NewActor = World->SpawnActor<FSkyLightActor>(CubeMap);
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

FActor* EditorActorFactory::SpawnCamera(FWorld* World, const Vector3& Location)
{
    if (!World)
    {
        return nullptr;
    }

    const String ActorName = MakeUniqueActorName(World, "Camera");

    FCameraActor* NewActor = World->SpawnActor<FCameraActor>(Location, Vector3(0.0f, 0.0f, 0.0f));
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

        case EEditorLightType::Sky:
        {
            return FindSkyLightCubeMap(World).IsValid();
        }

        default:
        {
            return true;
        }
    }
}

FActor* EditorActorFactory::DrawPlaceActorMenu(FWorld* World, const Vector3& Location)
{
    FActor* SpawnedActor = nullptr;

    {
        FSubMenuState MeshSubMenu;
        if (EditorWidgets::BeginSubMenu(MeshSubMenu, "##PlaceActorMeshMenu", "Mesh"))
        {
            for (int32 Index = 0; Index < NumPrimitiveTypes; ++Index)
            {
                const EEditorPrimitiveType Type = static_cast<EEditorPrimitiveType>(Index);
                if (EditorWidgets::MenuItem(GetPrimitiveName(Type)))
                {
                    SpawnedActor = SpawnPrimitive(World, Type, Location);
                }
            }

            EditorWidgets::EndSubMenu(MeshSubMenu);
        }
    }

    {
        FSubMenuState LightSubMenu;
        if (EditorWidgets::BeginSubMenu(LightSubMenu, "##PlaceActorLightMenu", "Light"))
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

    if (EditorWidgets::MenuItem("Camera"))
    {
        SpawnedActor = SpawnCamera(World, Location);
    }

    return SpawnedActor;
}

String EditorActorFactory::MakeUniqueActorName(FWorld* World, const CHAR* BaseName)
{
    String Name = BaseName;
    if (!World)
    {
        return Name;
    }

    auto IsNameTaken = [World](const String& Candidate) -> bool
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
    TStaticArray<CHAR, 128> Buffer{};

    while (IsNameTaken(Name))
    {
        ++Suffix;
        CString::Snprintf(Buffer.Data(), static_cast<int32>(Buffer.Size()), "%s (%d)", BaseName, Suffix);
        Name = Buffer.Data();
    }

    return Name;
}

void EditorActorFactory::ReleaseCachedMeshes()
{
    for (int32 Index = 0; Index < NumPrimitiveTypes; ++Index)
    {
        GPrimitiveMeshCache[Index].Reset();
    }
}

const CHAR* EditorActorFactory::GetPrimitiveName(EEditorPrimitiveType Type)
{
    const int32 Index = static_cast<int32>(Type);
    return (Index >= 0 && Index < NumPrimitiveTypes) ? GPrimitiveRecipes[Index].Name : "Actor";
}

const CHAR* EditorActorFactory::GetLightName(EEditorLightType Type)
{
    switch (Type)
    {
        case EEditorLightType::Point:       return "Point Light";
        case EEditorLightType::Spot:        return "Spot Light";
        case EEditorLightType::Directional: return "Directional Light";
        case EEditorLightType::Sky:         return "Sky Light";
        default:                            return "Light";
    }
}
