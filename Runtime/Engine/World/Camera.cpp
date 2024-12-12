#include "Camera.h"

FCamera::FCamera()
    : View()
    , Projection()
    , ViewProjection()
    , ViewProjectionInverse()
    , NearPlane(0.01f)
    , FarPlane(200.0f)
    , AspectRatio()
    , Position(0.0f, 0.0f, -2.0f)
    , Rotation(0.0f, 0.0f, 0.0f)
    , ForwardVector(0.0f, 0.0f, 1.0f)
    , RightVector(-1.0f, 0.0f, 0.0f)
    , UpVector(0.0f, 1.0f, 0.0f)
{
    UpdateMatrices();
}

FCamera::~FCamera()
{
}

void FCamera::Move(float x, float y, float z)
{
    const FVector3 TempRight   = RightVector * x;
    const FVector3 TempUp      = UpVector * y;
    const FVector3 TempForward = ForwardVector * z;
    Position = Position + TempRight + TempUp + TempForward;
}

void FCamera::Rotate(float Pitch, float Yaw, float Roll)
{
    Rotation.X += Pitch;
    Rotation.X  = FMath::Clamp(Rotation.X, FMath::ToRadians(-89.0f), FMath::ToRadians(89.0f));

    Rotation.Y += Yaw;
    Rotation.Z += Roll;

    UpdateDirectionVectors();
}

void FCamera::SetPosition(float x, float y, float z)
{
    Position = FVector3(x, y, z);
}

void FCamera::SetRotation(float Pitch, float Yaw, float Roll)
{
    Rotation.X = FMath::Clamp(Pitch, FMath::ToRadians(-89.0f), FMath::ToRadians(89.0f));
    Rotation.Y = Yaw;
    Rotation.Z = Roll;
    
    UpdateDirectionVectors();
}

void FCamera::UpdateDirectionVectors()
{
    FMatrix4 RotationMatrix = FMatrix4::RotationRollPitchYaw(Rotation);

    FVector3 TempForward(0.0f, 0.0f, 1.0f);
    ForwardVector = RotationMatrix.TransformNormal(TempForward);
    ForwardVector.Normalize();

    FVector3 TempUp(0.0f, 1.0f, 0.0f);
    RightVector = ForwardVector.CrossProduct(TempUp);
    RightVector.Normalize();
    
    UpVector = RightVector.CrossProduct(ForwardVector);
    UpVector.Normalize();
}

void FCamera::UpdateProjectionMatrix(float InFieldOfView, float InViewportWidth, float InViewportHeight)
{
    // Convert the field-of-view into radians instead of degrees
    const float FieldOfViewRadians = FMath::ToRadians(InFieldOfView);

    // Create the matrix
    Projection        = FMatrix4::PerspectiveProjection(FieldOfViewRadians, InViewportWidth, InViewportHeight, NearPlane, FarPlane);
    ProjectionInverse = Projection.GetInverse();

    // Cache the size of the viewport
    ViewportWidth  = InViewportWidth;
    ViewportHeight = InViewportHeight;
    FieldOfView    = InFieldOfView;
}

void FCamera::UpdateViewMatrix()
{
    View        = FMatrix4::LookTo(Position, ForwardVector, UpVector);
    ViewInverse = View.GetInverse();
}

void FCamera::UpdateMatrices()
{
    // Create Projection Matrix
    UpdateProjectionMatrix(90.0f, 1920.0f, 1080.0f);

    // Create the view matrix
    UpdateViewMatrix();

    // Create all other matrices that are dependent on these
    ViewProjection        = View * Projection;
    ViewProjectionInverse = ViewProjection.GetInverse();

    FMatrix3 View3x3 = View.GetRotationAndScale();
    ViewProjectionNoTranslation.SetIdentity();
    ViewProjectionNoTranslation.SetRotationAndScale(View3x3);
    ViewProjectionNoTranslation = ViewProjectionNoTranslation * Projection;
}
