#include "Core/Math/Math.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/CameraComponent.h"

FOBJECT_IMPLEMENT_CLASS(FCameraComponent);

FCameraComponent::FCameraComponent(const FObjectInitializer& ObjectInitializer)
    : FSceneComponent(ObjectInitializer)
    , View()
    , ViewInverse()
    , Projection()
    , ProjectionInverse()
    , ViewProjection()
    , ViewProjectionInverse()
    , ViewProjectionNoTranslation()
    , NearPlane(0.01f)
    , FarPlane(200.0f)
    , AspectRatio(0.0f)
    , ViewportWidth(0.0f)
    , ViewportHeight(0.0f)
    , FieldOfView(90.0f)
    , ForwardVector(Vector3::Forward)
    , RightVector(-1.0f, 0.0f, 0.0f)
    , UpVector(Vector3::Up)
{
}

FCameraComponent::~FCameraComponent()
{
}

void FCameraComponent::AddLocalMovement(float x, float y, float z)
{
    UpdateDirectionVectors();

    FActorTransform& Transform = GetActorOwner()->GetTransform();

    const Vector3 LocalRight   = RightVector * x;
    const Vector3 LocalUp      = UpVector * y;
    const Vector3 LocalForward = ForwardVector * z;
    const Vector3 Translation  = Transform.GetTranslation() + LocalRight + LocalUp + LocalForward;

    Transform.SetTranslation(Translation);
}

void FCameraComponent::AddRotation(float Pitch, float Yaw, float Roll)
{
    const Vector3 CurrentRotation = GetActorOwner()->GetTransform().GetRotation();
    SetRotation(CurrentRotation.X + Pitch, CurrentRotation.Y + Yaw, CurrentRotation.Z + Roll);
}

void FCameraComponent::SetFieldOfView(float InFieldOfView)
{
    FieldOfView = InFieldOfView;
}

void FCameraComponent::SetNearPlane(float InNearPlane)
{
    NearPlane = InNearPlane;
    UpdateProjectionMatrix(ViewportWidth, ViewportHeight);
}

void FCameraComponent::SetFarPlane(float InFarPlane)
{
    FarPlane = InFarPlane;
    UpdateProjectionMatrix(ViewportWidth, ViewportHeight);
}

void FCameraComponent::SetPosition(float x, float y, float z)
{
    SetPosition(Vector3(x, y, z));
}

void FCameraComponent::SetPosition(const Vector3& InPosition)
{
    GetActorOwner()->GetTransform().SetTranslation(InPosition);
}

void FCameraComponent::SetRotation(float Pitch, float Yaw, float Roll)
{
    const float ClampedPitch = Math::Clamp(Math::FMod(Pitch, Math::Constants::TwoPI), Math::DegreesToRadians(-89.0f), Math::DegreesToRadians(89.0f));
    GetActorOwner()->GetTransform().SetRotation(ClampedPitch, Yaw, Roll);

    UpdateDirectionVectors();
}

void FCameraComponent::SetRotation(const Vector3& InRotation)
{
    SetRotation(InRotation.X, InRotation.Y, InRotation.Z);
}

void FCameraComponent::UpdateDirectionVectors()
{
    CHECK(GetActorOwner() != nullptr);

    const Vector3& Rotation = GetActorOwner()->GetWorldTransform().GetRotation();
    
    const Matrix4 RotationMatrix = Matrix4::RotationRollPitchYaw(Rotation);
    ForwardVector = RotationMatrix.TransformNormal(Vector3::Forward);
    ForwardVector.Normalize();

    RightVector = ForwardVector.CrossProduct(Vector3::Up);
    RightVector.Normalize();

    UpVector = RightVector.CrossProduct(ForwardVector);
    UpVector.Normalize();
}

void FCameraComponent::UpdateProjectionMatrix(float InViewportWidth, float InViewportHeight)
{
    if (InViewportWidth <= 0.0f || InViewportHeight <= 0.0f)
    {
        return;
    }

    const float FieldOfViewRadians = Math::DegreesToRadians(FieldOfView);

    Projection        = Matrix4::PerspectiveProjection(FieldOfViewRadians, InViewportWidth, InViewportHeight, NearPlane, FarPlane);
    ProjectionInverse = Projection.GetInverse();
    AspectRatio       = InViewportWidth / InViewportHeight;
    ViewportWidth     = InViewportWidth;
    ViewportHeight    = InViewportHeight;
}

void FCameraComponent::UpdateViewMatrix()
{
    CHECK(GetActorOwner() != nullptr);

    UpdateDirectionVectors();

    const Vector3& Position = GetActorOwner()->GetWorldTransform().GetTranslation();
    View        = Matrix4::LookTo(Position, ForwardVector, UpVector);
    ViewInverse = View.GetInverse();
}

void FCameraComponent::UpdateWorldToClipSpaceMatrices()
{
    ViewProjection        = View * Projection;
    ViewProjectionInverse = ViewProjection.GetInverse();

    const Matrix3 View3x3 = View.GetRotationAndScale();
    ViewProjectionNoTranslation.SetIdentity();
    ViewProjectionNoTranslation.SetRotationAndScale(View3x3);
    ViewProjectionNoTranslation = ViewProjectionNoTranslation * Projection;
}

const Vector3& FCameraComponent::GetPosition() const
{
    CHECK(GetActorOwner() != nullptr);
    return GetActorOwner()->GetWorldTransform().GetTranslation();
}

const Vector3& FCameraComponent::GetRotation() const
{
    CHECK(GetActorOwner() != nullptr);
    return GetActorOwner()->GetWorldTransform().GetRotation();
}
