#include "DirectionalLight.h"

#include "Scene/Camera.h"

#include "Math/Math.h"

DirectionalLight::DirectionalLight()
    : Light()
    , Direction( 0.0f, -1.0f, 0.0f )
    , Rotation( 0.0f, 0.0f, 0.0f )
    , Position( 0.0f, 0.0f, 0.0f )
    , LookAt( 0.0f, 0.0f, 0.0f )
    , Matrices()
{
    CORE_OBJECT_INIT();

    for ( uint32 i = 0; i < NUM_SHADOW_CASCADES; i++ )
    {
        Matrices[i].SetIdentity();
        ViewMatrices[i].SetIdentity();
        ProjectionMatrices[i].SetIdentity();
    }
}

DirectionalLight::~DirectionalLight()
{
    // Empty for now
}

void DirectionalLight::UpdateCascades( Camera& Camera )
{
    //XMVECTOR XmDirection = XMVectorSet( 0.0, -1.0f, 0.0f, 0.0f );
    //XMMATRIX XmRotation = XMMatrixRotationRollPitchYaw( Rotation.x, Rotation.y, Rotation.z );
    //XMVECTOR XmOffset = XMVector3Transform( XmDirection, XmRotation );
    //XmDirection = XMVector3Normalize( XmOffset );
    //XMStoreFloat3( &Direction, XmDirection );

    CMatrix4 RotationMatrix = CMatrix4::RotationRollPitchYaw( Rotation.x, Rotation.y, Rotation.z );
    
    CVector3 StartDirection(0.0f, -1.0f, 0.0f);
    StartDirection = RotationMatrix.TransformDirection(StartDirection);
    StartDirection.Normalize();
    Direction = StartDirection;

    CVector3 StartUp( 0.0, 0.0f, 1.0f );
    StartUp = RotationMatrix.TransformDirection(StartUp);
    StartUp.Normalize();
    Up = StartUp;

    CMatrix4 InvCamera = Camera.GetViewProjectionInverseMatrix();
    InvCamera = InvCamera.Transpose();

    float NearPlane = Camera.GetNearPlane();
    float FarPlane = NMath::Min<float>( Camera.GetFarPlane(), 100.0f ); // TODO: Should be a setting
    float ClipRange = FarPlane - NearPlane;

    ShadowNearPlane = NearPlane;
    ShadowFarPlane = FarPlane;

    float MinZ = NearPlane;
    float MaxZ = FarPlane;

    float Range = ClipRange;
    float Ratio = MaxZ / MinZ;

    float LocalCascadeSplits[NUM_SHADOW_CASCADES];

    // Calculate split depths based on view camera frustum
    // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
    for ( uint32 i = 0; i < 4; i++ )
    {
        float p = (i + 1) / static_cast<float>(NUM_SHADOW_CASCADES);
        float Log = MinZ * std::pow( Ratio, p );
        float Uniform = MinZ + Range * p;
        float d = CascadeSplitLambda * (Log - Uniform) + Uniform;
        LocalCascadeSplits[i] = (d - NearPlane) / ClipRange;
    }

    // TODO: This has to be moved so we do not duplicate it
    const float CascadeSizes[NUM_SHADOW_CASCADES] =
    {
        2048.0f, 2048.0f, 2048.0f, 4096.0f
    };

    float LastSplitDist = 0.0f;
    for ( uint32 i = 0; i < 4; i++ )
    {
        float SplitDist = LocalCascadeSplits[i];

        CVector4 FrustumCorners[8] =
        {
            CVector4( -1.0f,  1.0f, 0.0f, 1.0f ),
            CVector4( 1.0f,  1.0f, 0.0f, 1.0f ),
            CVector4( 1.0f, -1.0f, 0.0f, 1.0f ),
            CVector4( -1.0f, -1.0f, 0.0f, 1.0f ),
            CVector4( -1.0f,  1.0f, 1.0f, 1.0f ),
            CVector4( 1.0f,  1.0f, 1.0f, 1.0f ),
            CVector4( 1.0f, -1.0f, 1.0f, 1.0f ),
            CVector4( -1.0f, -1.0f, 1.0f, 1.0f ),
        };

        // Calculate position of light frustum
        for ( uint32 j = 0; j < 8; j++ )
        {
            CVector4 Corner = InvCamera * FrustumCorners[j];
            FrustumCorners[j] = FrustumCorners[j] / FrustumCorners[j].w;
        }

        for ( uint32 j = 0; j < 4; j++ )
        {
            const CVector4 Distance = FrustumCorners[j + 4] - FrustumCorners[j];
            FrustumCorners[j + 4] = FrustumCorners[j] + (Distance * SplitDist);
            FrustumCorners[j] = FrustumCorners[j] + (Distance * LastSplitDist);
        }

        // Calc frustum center
        CVector4 Center = CVector4( 0.0f );
        for ( uint32 j = 0; j < 8; j++ )
        {
            Center = Center + FrustumCorners[j];
        }
        Center = Center * (1.0f / 8.0f);

        float Radius = 0.0f;
        for ( uint32 j = 0; j < 8; j++ )
        {
            float Distance = ceil( (FrustumCorners[j] - Center).Length() );
            Radius = NMath::Min( NMath::Max( Radius, Distance ), 80.0f ); // This should be dynamic
        }

        // Make sure we only move cascades with whole pixels
        //float TexelsPerUnit = CascadeSizes[i] / (Radius * 2.0f);
        //XMMATRIX Scale = XMMatrixScaling(TexelsPerUnit, TexelsPerUnit, TexelsPerUnit);

        //XMVECTOR EyePosition  = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        //XMVECTOR LookPosition = XMVectorSet(-Direction.x, -Direction.y, -Direction.z, 0.0f);
        //
        //XMMATRIX LookAtMat = XMMatrixLookAtLH(EyePosition, LookPosition, XmUp);
        //LookAtMat = XMMatrixMultiply(Scale, LookAtMat);

        //XMMATRIX LookAtMatInverse = XMMatrixInverse(nullptr, LookAtMat);

        //XMVECTOR XmCenter = XMLoadFloat4(&Center);
        //XMVector3Transform(XmCenter, LookAtMat);
        //XMStoreFloat4(&Center, XmCenter);

        //Center.x = floor(Center.x);
        //Center.y = floor(Center.y);
        //Center.z = floor(Center.z);

        //XmCenter = XMLoadFloat4(&Center);
        //XMVector3Transform(XmCenter, LookAtMatInverse);
        //XMStoreFloat4(&Center, XmCenter);

        CVector3 CascadePosition = CVector3( Center.x, Center.y, Center.z ) - (Direction * Radius * 6.0f);
        CVector3 EyePosition = CascadePosition;
        CVector3 LookPosition = CVector3( Center.x, Center.y, Center.z );

        CMatrix4 View = CMatrix4::LookAt( EyePosition, LookPosition, Up );
        CMatrix4 Projection = CMatrix4::OrtographicProjection( -Radius, Radius, -Radius, Radius, 0.01f, Radius * 12.0f );
        ViewMatrices[i] = View.Transpose();
        ProjectionMatrices[i] = Projection.Transpose();
        Matrices[i] = (View * Projection).Transpose();

        LastSplitDist = SplitDist;

        CascadeSplits[i] = (NearPlane + SplitDist * ClipRange);
        CascadeRadius[i] = Radius;

        if ( i == 0 )
        {
            LookAt = CVector3( Center.x, Center.y, Center.z );
            Position = CascadePosition;
        }
    }

    return;
}

void DirectionalLight::SetRotation( const CVector3& InRotation )
{
    Rotation = InRotation;
}

void DirectionalLight::SetRotation( float x, float y, float z )
{
    SetRotation( CVector3( x, y, z ) );
}

void DirectionalLight::SetLookAt( const CVector3& InLookAt )
{
    LookAt = InLookAt;
}

void DirectionalLight::SetLookAt( float x, float y, float z )
{
    SetLookAt( CVector3( x, y, z ) );
}
