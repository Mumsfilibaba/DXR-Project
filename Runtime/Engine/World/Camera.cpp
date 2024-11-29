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
    , Forward(0.0f, 0.0f, 1.0f)
    , Right(-1.0f, 0.0f, 0.0f)
    , Up(0.0f, 1.0f, 0.0f)
{
    UpdateMatrices();
}

void FCamera::Move(float x, float y, float z)
{
    const FVector3 TempRight   = Right * x;
    const FVector3 TempUp      = Up * y;
    const FVector3 TempForward = Forward * z;
    Position = Position + TempRight + TempUp + TempForward;
}

void FCamera::Rotate(float Pitch, float Yaw, float Roll)
{
    Rotation.x += Pitch;
    Rotation.x  = FMath::Clamp(Rotation.x, FMath::ToRadians(-89.0f), FMath::ToRadians(89.0f));

    Rotation.y += Yaw;
    Rotation.z += Roll;

    FMatrix4 RotationMatrix = FMatrix4::RotationRollPitchYaw(Rotation);

    FVector3 TempForward(0.0f, 0.0f, 1.0f);
    Forward = RotationMatrix.TransformNormal(TempForward);
    Forward.Normalize();

    FVector3 TempUp(0.0f, 1.0f, 0.0f);
    Right = Forward.CrossProduct(TempUp);
    Right.Normalize();
    
    Up = Right.CrossProduct(Forward);
    Up.Normalize();
}

void FCamera::UpdateMatrices()
{
    FOV    = FMath::ToRadians(80.0f);
    Width  = 1920.0f;
    Height = 1080.0f;

    Projection  = FMatrix4::PerspectiveProjection(FOV, Width, Height, NearPlane, FarPlane);
    View        = FMatrix4::LookTo(Position, Forward, Up);
    ViewInverse = View.GetInverse();

    FMatrix3 View3x3 = View.GetRotationAndScale();
    ProjectionInverse     = Projection.GetInverse();
    ViewProjection        = View * Projection;
    ViewProjectionInverse = ViewProjection.GetInverse();

    ViewProjectionNoTranslation.SetIdentity();
    ViewProjectionNoTranslation.SetRotationAndScale(View3x3);
    ViewProjectionNoTranslation = ViewProjectionNoTranslation * Projection;

    View                        = View.GetTranspose();
    ViewInverse                 = ViewInverse.GetTranspose();
    ProjectionInverse           = ProjectionInverse.GetTranspose();
    ViewProjection              = ViewProjection.GetTranspose();
    ViewProjectionInverse       = ViewProjectionInverse.GetTranspose();
    ViewProjectionNoTranslation = ViewProjectionNoTranslation.GetTranspose();
}
