#include "DirectionalLight.h"

#include "Scene/Camera.h"

#include "Math/Math.h"

DirectionalLight::DirectionalLight()
    : Light()
    , Direction(0.0f, -1.0f, 0.0f)
    , Rotation(0.0f, 0.0f, 0.0f)
    , Position(0.0f, 0.0f, 0.0f)
    , LookAt(0.0f, 0.0f, 0.0f)
    , Matrices()
{
    CORE_OBJECT_INIT();

    for (uint32 i = 0; i < NUM_SHADOW_CASCADES; i++)
    {
        XMStoreFloat4x4(&Matrices[i], XMMatrixIdentity());
        XMStoreFloat4x4(&ViewMatrices[i], XMMatrixIdentity());
        XMStoreFloat4x4(&ProjectionMatrices[i], XMMatrixIdentity());
    }
}

DirectionalLight::~DirectionalLight()
{
    // Empty for now
}

void DirectionalLight::UpdateCascades(Camera& Camera)
{
    XMVECTOR XmDirection = XMVectorSet(0.0, -1.0f, 0.0f, 0.0f);;
    XMMATRIX XmRotation  = XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z);
    XMVECTOR XmOffset    = XMVector3Transform(XmDirection, XmRotation);
    XmDirection = XMVector3Normalize(XmOffset);
    XMStoreFloat3(&Direction, XmDirection);

    XMVECTOR XmUp = XMVectorSet(0.0, 0.0f, 1.0f, 0.0f);
    XmUp = XMVector3Normalize(XMVector3Transform(XmUp, XmRotation));
    XMStoreFloat3(&Up, XmUp);

    XMFLOAT4X4 InvCamera = Camera.GetViewProjectionInverseMatrix();

    float NearPlane = Camera.GetNearPlane();
    float FarPlane  = Math::Min<float>(Camera.GetFarPlane(), 100.0f); // TODO: Should be a setting
    float ClipRange = FarPlane - NearPlane;

    ShadowNearPlane = NearPlane;
    ShadowFarPlane  = FarPlane;

    float MinZ = NearPlane;
    float MaxZ = FarPlane;

    float Range = ClipRange;
    float Ratio = MaxZ / MinZ;

    float LocalCascadeSplits[NUM_SHADOW_CASCADES];

    // Calculate split depths based on view camera frustum
    // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
    for (uint32 i = 0; i < 4; i++)
    {
        float p = (i + 1) / static_cast<float>(NUM_SHADOW_CASCADES);
        float Log = MinZ * std::pow(Ratio, p);
        float Uniform = MinZ + Range * p;
        float d = CascadeSplitLambda * (Log - Uniform) + Uniform;
        LocalCascadeSplits[i] = (d - NearPlane) / ClipRange;
    }

    // TODO: This has to be moved so we do not duplicate it
    const float CascadeSizes[NUM_SHADOW_CASCADES] =
    {
        2048.0f, 2048.0f, 4096.0f, 4096.0f
    };

    float LastSplitDist = 0.0f;
    for (uint32 i = 0; i < 4; i++)
    {
        float SplitDist = LocalCascadeSplits[i];

        XMFLOAT4 FrustumCorners[8] =
        {
            XMFLOAT4(-1.0f,  1.0f, 0.0f, 1.0f),
            XMFLOAT4( 1.0f,  1.0f, 0.0f, 1.0f),
            XMFLOAT4( 1.0f, -1.0f, 0.0f, 1.0f),
            XMFLOAT4(-1.0f, -1.0f, 0.0f, 1.0f),
            XMFLOAT4(-1.0f,  1.0f, 1.0f, 1.0f),
            XMFLOAT4( 1.0f,  1.0f, 1.0f, 1.0f),
            XMFLOAT4( 1.0f, -1.0f, 1.0f, 1.0f),
            XMFLOAT4(-1.0f, -1.0f, 1.0f, 1.0f),
        };

        // Calculate position of light frustum
        XMMATRIX XmInvCamera = XMMatrixTranspose(XMLoadFloat4x4(&InvCamera));

        for (uint32 j = 0; j < 8; j++)
        {
            XMVECTOR XmCorner = XMLoadFloat4(&FrustumCorners[j]);
            XmCorner = XMVector4Transform(XmCorner, XmInvCamera);
            XMStoreFloat4(&FrustumCorners[j], XmCorner);

            FrustumCorners[j] = FrustumCorners[j] / FrustumCorners[j].w;
        }

        for (uint32 j = 0; j < 4; j++)
        {
            const XMFLOAT4 Distance = FrustumCorners[j + 4] - FrustumCorners[j];
            FrustumCorners[j + 4] = FrustumCorners[j] + (Distance * SplitDist);
            FrustumCorners[j]     = FrustumCorners[j] + (Distance * LastSplitDist);
        }

        // Calc frustum center
        XMFLOAT4 Center = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
        for (uint32 j = 0; j < 8; j++)
        {
            Center = Center + FrustumCorners[j];
        }
        Center = Center * (1.0f / 8.0f);

        float Radius = 0.0f;
        for (uint32 j = 0; j < 8; j++)
        {
            float Distance = ceil(Length(FrustumCorners[j] - Center));
            Radius = Math::Min(Math::Max(Radius, Distance), 80.0f); // This should be dynamic
        }

        // Make sure we only move cascades with whole pixels
        float TexelsPerUnit = CascadeSizes[i] / (Radius * 2.0f);
        XMMATRIX Scale = XMMatrixScaling(TexelsPerUnit, TexelsPerUnit, TexelsPerUnit);

        XMVECTOR EyePosition  = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        XMVECTOR LookPosition = XMVectorSet(-Direction.x, -Direction.y, -Direction.z, 0.0f);
        
        XMMATRIX LookAtMat = XMMatrixLookAtLH(EyePosition, LookPosition, XmUp);
        LookAtMat = XMMatrixMultiply(Scale, LookAtMat);

        XMMATRIX LookAtMatInverse = XMMatrixInverse(nullptr, LookAtMat);

        XMVECTOR XmCenter = XMLoadFloat4(&Center);
        XMVector3Transform(XmCenter, LookAtMat);
        XMStoreFloat4(&Center, XmCenter);

        Center.x = floor(Center.x);
        Center.y = floor(Center.y);
        Center.z = floor(Center.z);

        XmCenter = XMLoadFloat4(&Center);
        XMVector3Transform(XmCenter, LookAtMatInverse);
        XMStoreFloat4(&Center, XmCenter);

        XMFLOAT3 CascadePosition = XMFLOAT3(Center.x, Center.y, Center.z) - (Direction * Radius * 6.0f);
        
        EyePosition  = XMLoadFloat3(&CascadePosition);
        LookPosition = XMLoadFloat4(&Center);

        XMMATRIX XmViewMatrix = XMMatrixLookAtLH(EyePosition, LookPosition, XmUp);
        XMMATRIX XmOrtoMatrix = XMMatrixOrthographicOffCenterLH(-Radius, Radius, -Radius, Radius, 0.01f, Radius * 12.0f);

        XMStoreFloat4x4(&ViewMatrices[i], XMMatrixTranspose(XmViewMatrix));
        XMStoreFloat4x4(&ProjectionMatrices[i], XMMatrixTranspose(XmOrtoMatrix));
        XMStoreFloat4x4(&Matrices[i], XMMatrixMultiplyTranspose(XmViewMatrix, XmOrtoMatrix));

        LastSplitDist = SplitDist;

        CascadeSplits[i] = (NearPlane + SplitDist * ClipRange);
        CascadeRadius[i] = Radius;

        if (i == 0)
        {
            LookAt   = XMFLOAT3(Center.x, Center.y, Center.z);
            Position = CascadePosition;
        }
    }
}

void DirectionalLight::SetRotation(const XMFLOAT3& InRotation)
{
    Rotation = InRotation;
}

void DirectionalLight::SetRotation(float x, float y, float z)
{
    SetRotation(XMFLOAT3(x, y, z));
}

void DirectionalLight::SetLookAt(const XMFLOAT3& InLookAt)
{
    LookAt = InLookAt;
}

void DirectionalLight::SetLookAt(float x, float y, float z)
{
    SetLookAt(XMFLOAT3(x, y, z));
}
