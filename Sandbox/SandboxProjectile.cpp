#include "SandboxProjectile.h"
#include <Core/Math/Random.h>
#include <Engine/Engine.h>
#include <Engine/Assets/MeshFactory.h>
#include <Engine/Resources/Material.h>
#include <Engine/Resources/Model.h>
#include <Engine/World/Actors/Actor.h>
#include <Engine/World/Components/CameraComponent.h>
#include <Engine/World/Components/KinematicMovementComponent.h>
#include <Engine/World/Components/StaticMeshComponent.h>
#include <Engine/World/World.h>

constexpr float SpawnDistance = 2.0f;
constexpr float LaunchSpeed   = 15.0f;
constexpr float Gravity       = -20.0f;
constexpr float DespawnBelowY = -50.0f;
constexpr float RoughnessMin  = 0.05f;
constexpr float RoughnessMax  = 0.95f;

static TSharedPtr<FMesh> GProjectileSphereMesh;
static FRandom           GProjectileRandom;

void ReleaseProjectileMeshCache()
{
    GProjectileSphereMesh.Reset();
}

FActor* SpawnProjectileSphere(FWorld* World, FCameraComponent* Camera)
{
    if (!World || !Camera)
    {
        return nullptr;
    }

    if (!GProjectileSphereMesh)
    {
        GProjectileSphereMesh = FMesh::Create(MeshFactory::CreateSphere(3));
    }

    FActor* Actor = World->CreateActor();
    if (!Actor)
    {
        return nullptr;
    }

    Actor->GetTransform().SetTranslation(Camera->GetViewLocationAtDistance(SpawnDistance));

    FMaterialInfo MaterialInfo;
    MaterialInfo.Albedo           = FFloatColor::White;
    MaterialInfo.Metallic         = 1.0f;
    MaterialInfo.Roughness        = GProjectileRandom.RandFloat(RoughnessMin, RoughnessMax);
    MaterialInfo.AmbientOcclusion = 1.0f;
    MaterialInfo.MaterialFlags    = EMaterialFlags::None;

    TSharedPtr<FMaterial> Material = MakeSharedPtr<FMaterial>(MaterialInfo);
    Material->SetTexture(EMaterialTextureSlot::BaseColor, FEngine::Get()->BaseTexture);
    Material->SetTexture(EMaterialTextureSlot::MaskA, FEngine::Get()->BaseTexture);
    Material->Initialize();

    FStaticMeshComponent* MeshComp = NewObject<FStaticMeshComponent>();
    MeshComp->SetMesh(GProjectileSphereMesh);
    MeshComp->SetMaterial(Material);
    Actor->AddComponent(MeshComp);

    FKinematicMovementComponent* MoveComp = NewObject<FKinematicMovementComponent>();
    MoveComp->SetGravity(Gravity);
    MoveComp->SetDespawnBelowY(DespawnBelowY);
    MoveComp->SetInitialVelocity(Camera->GetForwardVector() * LaunchSpeed);
    Actor->AddComponent(MoveComp);

    return Actor;
}
