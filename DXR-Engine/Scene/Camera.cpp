#include "Camera.h"

#include <algorithm>

Camera::Camera()
    : View()
    , Projection()
    , ViewProjection()
    , ViewProjectionInverse()
	, NearPlane( 0.01f )
	, FarPlane( 100.0f )
	, AspectRatio()
    , Position( 0.0f, 0.0f, -2.0f )
	, Rotation( 0.0f, 0.0f, 0.0f )
	, Forward( 0.0f, 0.0f, 1.0f )
    , Right( -1.0f, 0.0f, 0.0f )
    , Up( 0.0f, 1.0f, 0.0f )
{
    UpdateMatrices();
}

void Camera::Move( float x, float y, float z )
{
    const CVector3 TempRight = Right * x;
    const CVector3 TempUp = Up * y;
    const CVector3 TempForward = Forward * z;
    Position = Position + TempRight + TempUp + TempForward;
}

void Camera::Rotate( float Pitch, float Yaw, float Roll )
{
    Rotation.x += Pitch;
    Rotation.x = NMath::Max<float>( NMath::ToRadians( -89.0f ), NMath::Min<float>( NMath::ToRadians( 89.0f ), Rotation.x ) );

    Rotation.y += Yaw;
    Rotation.z += Roll;

    CMatrix4 RotationMatrix = CMatrix4::RotationRollPitchYaw( Rotation );
    CVector3 TempForward( 0.0f, 0.0f, 1.0f );
    Forward = RotationMatrix.TransformDirection( TempForward );
    Forward.Normalize();

    CVector3 TempUp( 0.0f, 1.0f, 0.0f );
    Right = Forward.CrossProduct( TempUp );
    Right.Normalize();
    Up = Right.CrossProduct( Forward );
    Up.Normalize();
}

void Camera::UpdateMatrices()
{
    FOV = NMath::ToRadians( 80.0f );
    Width = 1920.0f;
    Height = 1080.0f;

    Projection = CMatrix4::PerspectiveProjection( FOV, Width, Height, NearPlane, FarPlane );
    View = CMatrix4::LookTo( Position, Forward, Up );
    ViewInverse = View.Invert();

    CMatrix3 View3x3 = View.GetRotationAndScale();
    ProjectionInverse = Projection.Invert();
    ViewProjection = View * Projection;
    ViewProjectionInverse = ViewProjection.Invert();

    ViewProjectionNoTranslation.SetIdentity();
    ViewProjectionNoTranslation.SetRotationAndScale( View3x3 );
    ViewProjectionNoTranslation = ViewProjectionNoTranslation * Projection;

    View = View.Transpose();
    ViewInverse = ViewInverse.Transpose();
    ProjectionInverse = ProjectionInverse.Transpose();
    ViewProjection = ViewProjection.Transpose();
    ViewProjectionInverse = ViewProjectionInverse.Transpose();
    ViewProjectionNoTranslation = ViewProjectionNoTranslation.Transpose();
}
