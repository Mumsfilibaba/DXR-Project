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

    for (uint32 i = 0; i < NUM_CASCADES; i++)
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

    //const float Offset = 35.0f;
    //XMMATRIX XmProjection = XMMatrixOrthographicOffCenterLH(-Offset, Offset, -Offset, Offset, ShadowNearPlane, ShadowFarPlane);
    //XMMATRIX XmView = XMMatrixLookAtLH(XmPosition, XmLookAt, XmUp);

    const float Scale = (ShadowFarPlane - ShadowNearPlane) / 2.0f;
    XmOffset = XMVectorScale(XmOffset, -Scale);

    XMVECTOR XmLookAt = XMLoadFloat3(&LookAt);
    XMVECTOR XmPosition = XMVectorAdd(XmLookAt, XmOffset);
    XMStoreFloat3(&Position, XmPosition);

    // TODO: Should not be the done in the renderer
    //XMFLOAT3 CameraPosition = Camera->GetPosition();
    //XMFLOAT3 CameraForward  = Camera->GetForward();
    //XMFLOAT4X4 LightView = CurrentLight->GetViewMatrix();

    XMFLOAT4X4 InvCamera = Camera.GetViewProjectionInverseMatrix();

    float NearPlane = GetShadowNearPlane();
    float FarPlane  = GetShadowFarPlane();
    float ClipRange = FarPlane - NearPlane;

    float MinZ = NearPlane;
    float MaxZ = FarPlane;

    float Range = ClipRange;
    float Ratio = MaxZ / MinZ;

    // Calculate split depths based on view camera frustum
    // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
    for (uint32 i = 0; i < 4; i++)
    {
        float p = (i + 1) / static_cast<float>(NUM_CASCADES);
        float Log = MinZ * std::pow(Ratio, p);
        float Uniform = MinZ + Range * p;
        float d = CascadeSplitLambda * (Log - Uniform) + Uniform;
        CascadeSplits[i] = (d - NearPlane) / ClipRange;
    }

    float LastSplitDist = 0.0f;
    for (uint32 i = 0; i < 4; i++)
    {
        float SplitDist = CascadeSplits[i];

        XMFLOAT4 FrustumCorners[8] =
        {
            XMFLOAT4(-1.0f,  1.0f, -1.0f, 1.0f),
            XMFLOAT4( 1.0f,  1.0f, -1.0f, 1.0f),
            XMFLOAT4( 1.0f, -1.0f, -1.0f, 1.0f),
            XMFLOAT4(-1.0f, -1.0f, -1.0f, 1.0f),
            XMFLOAT4(-1.0f,  1.0f,  1.0f, 1.0f),
            XMFLOAT4( 1.0f,  1.0f,  1.0f, 1.0f),
            XMFLOAT4( 1.0f, -1.0f,  1.0f, 1.0f),
            XMFLOAT4(-1.0f, -1.0f,  1.0f, 1.0f),
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
        Center = Center / 8.0f;

        float Radius = 0.0f;
        for (uint32 j = 0; j < 8; j++)
        {
            float Distance = Length(FrustumCorners[j] - Center);
            Radius = Math::Max(Radius, Distance);
        }
        Radius = std::ceil(Radius * 16.0f) / 16.0f;

        XMFLOAT3 MaxExtents = XMFLOAT3(Radius, Radius, Radius);
        XMFLOAT3 MinExtents = -MaxExtents;

        XMFLOAT3 CascadePosition = XMFLOAT3(Center.x, Center.y, Center.z) - Direction * -MinExtents.z;

        XMVECTOR EyePosition = XMVectorSet(CascadePosition.x, CascadePosition.y, CascadePosition.z, 1.0f);
        XMVECTOR LookPosition = XMVectorSet(Center.x, Center.y, Center.z, 0.0f);

        XMMATRIX XmViewMatrix = XMMatrixLookAtLH(EyePosition, LookPosition, XmUp);
        XMMATRIX XmOrtoMatrix = XMMatrixOrthographicOffCenterLH(MinExtents.x, MaxExtents.x, MinExtents.y, MaxExtents.y, 0.0f, MaxExtents.z - MinExtents.z);

        XMStoreFloat4x4(&ViewMatrices[i], XMMatrixTranspose(XmViewMatrix));
        XMStoreFloat4x4(&ProjectionMatrices[i], XMMatrixTranspose(XmOrtoMatrix));
        XMStoreFloat4x4(&Matrices[i], XMMatrixMultiplyTranspose(XmViewMatrix, XmOrtoMatrix));

        LastSplitDist = SplitDist;
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

void DirectionalLight::SetShadowNearPlane(float InShadowNearPlane)
{
    if (InShadowNearPlane > 0.0f)
    {
        if (Math::Abs(ShadowFarPlane - InShadowNearPlane) >= 0.1f)
        {
            ShadowNearPlane = InShadowNearPlane;
        }
    }
}

void DirectionalLight::SetShadowFarPlane(float InShadowFarPlane)
{
    if (InShadowFarPlane > 0.0f)
    {
        if (Math::Abs(InShadowFarPlane - ShadowNearPlane) >= 0.1f)
        {
            ShadowFarPlane = InShadowFarPlane;
        }
    }
}
