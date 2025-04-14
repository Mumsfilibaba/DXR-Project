#include "Engine/World/Camera.h"

FCamera::FCamera()
    : View()
    , Projection()
    , ViewProjection()
    , ViewProjectionInverse()
    , NearPlane(0.01f)
    , FarPlane(200.0f)
    , FieldOfView(90.0f)
    , AspectRatio()
    , Position(0.0f, 0.0f, -2.0f)
    , Rotation(0.0f, 0.0f, 0.0f)
    , ForwardVector(FVector3::Forward)
    , RightVector(-1.0f, 0.0f, 0.0f)
    , UpVector(FVector3::Up)
{
}

FCamera::~FCamera()
{
}

void FCamera::Move(float x, float y, float z)
{
    FVector3 LocalRight   = RightVector * x;
    FVector3 LocalUp      = UpVector * y;
    FVector3 LocalForward = ForwardVector * z;

    Position = Position + LocalRight + LocalUp + LocalForward;
}

void FCamera::Rotate(float Pitch, float Yaw, float Roll)
{
    Rotation.X += Pitch;
    Rotation.Y += Yaw;
    Rotation.Z += Roll;

    Rotation.X = FMath::Clamp(FMath::FMod(Rotation.X, FMath::TwoPI_Float), FMath::DegreesToRadians(-89.0f), FMath::DegreesToRadians(89.0f));

    UpdateDirectionVectors();
}

void FCamera::SetFieldOfView(float InFieldOfView)
{
    FieldOfView = InFieldOfView;
}

void FCamera::SetPosition(float x, float y, float z)
{
    Position = FVector3(x, y, z);
}

void FCamera::SetRotation(float Pitch, float Yaw, float Roll)
{
    Rotation.X = FMath::Clamp(FMath::FMod(Pitch, FMath::TwoPI_Float), FMath::DegreesToRadians(-89.0f), FMath::DegreesToRadians(89.0f));
    Rotation.Y = Yaw;
    Rotation.Z = Roll;

    UpdateDirectionVectors();
}

void FCamera::UpdateDirectionVectors()
{
    FMatrix4 RotationMatrix = FMatrix4::RotationRollPitchYaw(Rotation);

    ForwardVector = RotationMatrix.TransformNormal(FVector3::Forward);
    ForwardVector.Normalize();

    RightVector = ForwardVector.CrossProduct(FVector3::Up);
    RightVector.Normalize();

    UpVector = RightVector.CrossProduct(ForwardVector);
    UpVector.Normalize();
}

void FCamera::UpdateProjectionMatrix(float InViewportWidth, float InViewportHeight)
{
    // Convert the field-of-view into radians instead of degrees
    const float FieldOfViewRadians = FMath::DegreesToRadians(FieldOfView);

    // Create the matrix
    Projection        = FMatrix4::PerspectiveProjection(FieldOfViewRadians, InViewportWidth, InViewportHeight, NearPlane, FarPlane);
    ProjectionInverse = Projection.GetInverse();

    // Cache the size of the viewport
    ViewportWidth  = InViewportWidth;
    ViewportHeight = InViewportHeight;
}

void FCamera::UpdateViewMatrix()
{
    View        = FMatrix4::LookTo(Position, ForwardVector, UpVector);
    ViewInverse = View.GetInverse();
}

void FCamera::UpdateWorldToClipSpaceMatrices()
{
    // Create all other matrices that are dependent on these
    ViewProjection        = View * Projection;
    ViewProjectionInverse = ViewProjection.GetInverse();

    FMatrix3 View3x3 = View.GetRotationAndScale();
    ViewProjectionNoTranslation.SetIdentity();
    ViewProjectionNoTranslation.SetRotationAndScale(View3x3);
    ViewProjectionNoTranslation = ViewProjectionNoTranslation * Projection;
}
