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
    , ForwardVector(Vector3::Forward)
    , RightVector(-1.0f, 0.0f, 0.0f)
    , UpVector(Vector3::Up)
{
}

FCamera::~FCamera()
{
}

void FCamera::Move(float x, float y, float z)
{
    Vector3 LocalRight   = RightVector * x;
    Vector3 LocalUp      = UpVector * y;
    Vector3 LocalForward = ForwardVector * z;

    Position = Position + LocalRight + LocalUp + LocalForward;
}

void FCamera::Rotate(float Pitch, float Yaw, float Roll)
{
    Rotation.X += Pitch;
    Rotation.Y += Yaw;
    Rotation.Z += Roll;

    Rotation.X = Math::Clamp(Math::FMod(Rotation.X, Math::Constants::TwoPI), Math::DegreesToRadians(-89.0f), Math::DegreesToRadians(89.0f));

    UpdateDirectionVectors();
}

void FCamera::SetFieldOfView(float InFieldOfView)
{
    FieldOfView = InFieldOfView;
}

void FCamera::SetNearPlane(float InNearPlane)
{
    NearPlane = InNearPlane;
    UpdateProjectionMatrix(ViewportWidth, ViewportHeight);
}

void FCamera::SetFarPlane(float InFarPlane)
{
    FarPlane = InFarPlane;
    UpdateProjectionMatrix(ViewportWidth, ViewportHeight);
}

void FCamera::SetPosition(float x, float y, float z)
{
    Position = Vector3(x, y, z);
}

void FCamera::SetRotation(float Pitch, float Yaw, float Roll)
{
    Rotation.X = Math::Clamp(Math::FMod(Pitch, Math::Constants::TwoPI), Math::DegreesToRadians(-89.0f), Math::DegreesToRadians(89.0f));
    Rotation.Y = Yaw;
    Rotation.Z = Roll;

    UpdateDirectionVectors();
}

void FCamera::UpdateDirectionVectors()
{
    Matrix4 RotationMatrix = Matrix4::RotationRollPitchYaw(Rotation);

    ForwardVector = RotationMatrix.TransformNormal(Vector3::Forward);
    ForwardVector.Normalize();

    RightVector = ForwardVector.CrossProduct(Vector3::Up);
    RightVector.Normalize();

    UpVector = RightVector.CrossProduct(ForwardVector);
    UpVector.Normalize();
}

void FCamera::UpdateProjectionMatrix(float InViewportWidth, float InViewportHeight)
{
    // Convert the field-of-view into radians instead of degrees
    const float FieldOfViewRadians = Math::DegreesToRadians(FieldOfView);

    // Create the matrix
    Projection        = Matrix4::PerspectiveProjection(FieldOfViewRadians, InViewportWidth, InViewportHeight, NearPlane, FarPlane);
    ProjectionInverse = Projection.GetInverse();

    // Cache the size of the viewport
    ViewportWidth  = InViewportWidth;
    ViewportHeight = InViewportHeight;
}

void FCamera::UpdateViewMatrix()
{
    View        = Matrix4::LookTo(Position, ForwardVector, UpVector);
    ViewInverse = View.GetInverse();
}

void FCamera::UpdateWorldToClipSpaceMatrices()
{
    // Create all other matrices that are dependent on these
    ViewProjection        = View * Projection;
    ViewProjectionInverse = ViewProjection.GetInverse();

    Matrix3 View3x3 = View.GetRotationAndScale();
    ViewProjectionNoTranslation.SetIdentity();
    ViewProjectionNoTranslation.SetRotationAndScale(View3x3);
    ViewProjectionNoTranslation = ViewProjectionNoTranslation * Projection;
}
