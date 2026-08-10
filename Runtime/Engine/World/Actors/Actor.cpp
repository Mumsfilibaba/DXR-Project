#include "Core/Math/Quaternion.h"
#include "Engine/World/World.h"
#include "Engine/World/Components/ActorComponent.h"
#include "Engine/World/Actors/Actor.h"

FOBJECT_IMPLEMENT_CLASS(FActor);

FActorTransform::FActorTransform()
    : Translation(0.0f, 0.0f, 0.0f)
    , Scale(1.0f, 1.0f, 1.0f)
    , Rotation(0.0f, 0.0f, 0.0f)
    , TransformMatrix()
    , Version(0)
{
    CalculateMatrix();
}

void FActorTransform::SetTranslation(float x, float y, float z)
{
    SetTranslation(Vector3(x, y, z));
}

void FActorTransform::SetTranslation(const Vector3& InPosition)
{
    Translation = InPosition;
    CalculateMatrix();
}

void FActorTransform::SetScale(float x, float y, float z)
{
    SetScale(Vector3(x, y, z));
}

void FActorTransform::SetScale(const Vector3& InScale)
{
    Scale = InScale;
    CalculateMatrix();
}

void FActorTransform::SetRotation(float x, float y, float z)
{
    SetRotation(Vector3(x, y, z));
}

void FActorTransform::SetRotation(const Vector3& InRotation)
{
    Rotation = InRotation;
    CalculateMatrix();
}

void FActorTransform::SetFromMatrix(const Matrix4& InMatrix)
{
    const Vector3 AxisX(InMatrix.M[0][0], InMatrix.M[0][1], InMatrix.M[0][2]);
    const Vector3 AxisY(InMatrix.M[1][0], InMatrix.M[1][1], InMatrix.M[1][2]);
    const Vector3 AxisZ(InMatrix.M[2][0], InMatrix.M[2][1], InMatrix.M[2][2]);

    Scale       = Vector3(AxisX.GetLength(), AxisY.GetLength(), AxisZ.GetLength());
    Translation = InMatrix.GetTranslation();

    Matrix4 OrthoNormalMatrix = InMatrix;
    OrthoNormalMatrix.OrthoNormalize();

    const Quaternion RotationQuaternion = Quaternion::FromRotationMatrix(OrthoNormalMatrix.GetRotationAndScale());
    Rotation = RotationQuaternion.ToEuler();

    // Store the matrix as-is instead of recomposing it, since a decomposed transform cannot 
    // represent the shear that a chain of rotated and non-uniformly scaled transforms may produce.
    TransformMatrix = InMatrix;
    Version++;
}

void FActorTransform::CalculateMatrix()
{
    Matrix4 ScaleMatrix       = Matrix4::Scale(Scale);
    Matrix4 RotationMatrix    = Matrix4::RotationRollPitchYaw(Rotation);
    Matrix4 TranslationMatrix = Matrix4::Translation(Translation);

    TransformMatrix = (ScaleMatrix * RotationMatrix) * TranslationMatrix;
    Version++;
}

FActor::FActor(const FObjectInitializer& ObjectInitializer)
    : FObject(ObjectInitializer)
    , Name()
    , World(nullptr)
#if EDITOR_BUILD
    , Filter(nullptr)
#endif
    , Transform()
    , Components()
    , ParentActor(nullptr)
    , ChildActors()
    , CachedWorldTransform()
    , CachedLocalVersion(0)
    , CachedParentVersion(0)
    , bIsStartable(true)
    , bIsTickable(true)
    , bTickInEditor(false)
{
}

FActor::~FActor()
{
    for (FActorComponent* CurrentComponent : Components)
    {
        SAFE_DELETE(CurrentComponent);
    }

    Components.Clear();
}

void FActor::Initialize()
{
}

void FActor::Start()
{
    for (FActorComponent* Component : Components)
    {
        if (Component->IsStartable())
        {
            Component->Start();
        }
    }
}

void FActor::EndPlay()
{
    for (FActorComponent* Component : Components)
    {
        Component->EndPlay();
    }
}

void FActor::Tick(float DeltaTime)
{
    for (FActorComponent* Component : Components)
    {
        if (Component->IsTickable())
        {
            Component->Tick(DeltaTime);
        }
    }
}

void FActor::AddComponent(FActorComponent* InComponent)
{
    CHECK(InComponent != nullptr);
    CHECK(InComponent->GetActorOwner() == nullptr);

    // Set this actor as the owner
    InComponent->SetActorOwner(this);
    Components.Emplace(InComponent);

    if (FSceneComponent* SceneComponent = Cast<FSceneComponent>(InComponent))
    {
        if (World)
        {
            World->AddSceneComponent(SceneComponent);
        }
    }
}

void FActor::RemoveComponent(FActorComponent* InComponent)
{
    CHECK(InComponent != nullptr);
    CHECK(InComponent->GetActorOwner() == this);

    if (FSceneComponent* SceneComponent = Cast<FSceneComponent>(InComponent))
    {
        if (World)
        {
            World->RemoveSceneComponent(SceneComponent);
        }
    }

    Components.Remove(InComponent);
    InComponent->SetActorOwner(nullptr);
    SAFE_DELETE(InComponent);
}

bool FActor::AttachToActor(FActor* NewParent, EAttachmentRule Rule)
{
    if (!NewParent)
    {
        DetachFromParent(Rule);
        return true;
    }

    if (NewParent == ParentActor)
    {
        return true;
    }

    if (NewParent == this)
    {
        LOG_WARNING("Cannot attach actor '%s' to itself", *Name);
        return false;
    }

    if (NewParent->IsAttachedTo(this))
    {
        LOG_WARNING("Cannot attach actor '%s' to '%s' since that would create a cycle", *Name, *NewParent->GetName());
        return false;
    }

    if (NewParent->GetWorld() != World)
    {
        LOG_WARNING("Cannot attach actor '%s' to '%s' since they belong to different worlds", *Name, *NewParent->GetName());
        return false;
    }

    const Matrix4 WorldMatrix = GetWorldTransform().GetTransformMatrix();

    UnlinkFromParent();

    ParentActor = NewParent;
    NewParent->ChildActors.Emplace(this);

    InvalidateWorldTransformCache();

    if (Rule == EAttachmentRule::KeepWorld)
    {
        SetWorldTransformMatrix(WorldMatrix);
    }

    return true;
}

void FActor::DetachFromParent(EAttachmentRule Rule)
{
    if (!ParentActor)
    {
        return;
    }

    const Matrix4 WorldMatrix = GetWorldTransform().GetTransformMatrix();

    UnlinkFromParent();
    InvalidateWorldTransformCache();

    if (Rule == EAttachmentRule::KeepWorld)
    {
        SetWorldTransformMatrix(WorldMatrix);
    }
}

void FActor::DetachAllChildren(EAttachmentRule Rule)
{
    while (!ChildActors.IsEmpty())
    {
        ChildActors.Last()->DetachFromParent(Rule);
    }
}

bool FActor::IsAttachedTo(const FActor* PossibleParent) const
{
    if (!PossibleParent)
    {
        return false;
    }

    for (const FActor* CurrentParent = ParentActor; CurrentParent; CurrentParent = CurrentParent->ParentActor)
    {
        if (CurrentParent == PossibleParent)
        {
            return true;
        }
    }

    return false;
}

const FActorTransform& FActor::GetWorldTransform() const
{
    if (!ParentActor)
    {
        return Transform;
    }

    const FActorTransform& ParentWorldTransform = ParentActor->GetWorldTransform();
    if (CachedLocalVersion != Transform.GetVersion() || CachedParentVersion != ParentWorldTransform.GetVersion())
    {
        CachedWorldTransform.SetFromMatrix(Transform.GetTransformMatrix() * ParentWorldTransform.GetTransformMatrix());
        CachedLocalVersion  = Transform.GetVersion();
        CachedParentVersion = ParentWorldTransform.GetVersion();
    }

    return CachedWorldTransform;
}

void FActor::SetWorldTransform(const FActorTransform& InTransform)
{
    SetWorldTransformMatrix(InTransform.GetTransformMatrix());
}

void FActor::SetWorldTransformMatrix(const Matrix4& InMatrix)
{
    Transform.SetFromMatrix(ConvertWorldToRelativeMatrix(InMatrix));
}

Matrix4 FActor::ConvertWorldToRelativeMatrix(const Matrix4& InWorldMatrix) const
{
    if (!ParentActor)
    {
        return InWorldMatrix;
    }

    return InWorldMatrix * ParentActor->GetWorldTransform().GetTransformMatrixInverse();
}

FActorTransform FActor::ConvertWorldToRelativeTransform(const FActorTransform& InWorldTransform) const
{
    if (!ParentActor)
    {
        return InWorldTransform;
    }

    FActorTransform RelativeTransform;
    RelativeTransform.SetFromMatrix(ConvertWorldToRelativeMatrix(InWorldTransform.GetTransformMatrix()));
    return RelativeTransform;
}

void FActor::UnlinkFromParent()
{
    if (ParentActor)
    {
        ParentActor->ChildActors.Remove(this);
        ParentActor = nullptr;
    }
}

void FActor::ClearAttachments()
{
    ParentActor = nullptr;
    ChildActors.Clear();
}

void FActor::InvalidateWorldTransformCache() const
{
    CachedLocalVersion  = 0;
    CachedParentVersion = 0;

    for (const FActor* Child : ChildActors)
    {
        Child->InvalidateWorldTransformCache();
    }
}

void FActor::SetName(const String& InName)
{
    Name = InName;
}

bool FActor::HasComponentOfClass(class FObjectClass* ComponentClass) const
{
    for (FActorComponent* Component : Components)
    {
        if (IsSubClassOf(Component, ComponentClass))
        {
            return true;
        }
    }

    return false;
}

FActorComponent* FActor::GetComponentOfClass(class FObjectClass* ComponentClass) const
{
    for (FActorComponent* Component : Components)
    {
        if (IsSubClassOf(Component, ComponentClass))
        {
            return Component;
        }
    }

    return nullptr;
}
