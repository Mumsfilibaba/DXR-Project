#include "DirectionalLight.h"

DirectionalLight::DirectionalLight()
    : Light()
    , Direction(0.0f, -1.0f, 0.0f)
    , Rotation(0.0f, 0.0f, 0.0f)
    , ShadowMapPosition(0.0f, 0.0f, 0.0f)
    , LookAt(0.0f, 0.0f, 0.0f)
    , Matrix()
{
    CORE_OBJECT_INIT();

    XMStoreFloat4x4(&Matrix, XMMatrixIdentity());
}

DirectionalLight::~DirectionalLight()
{
}

void DirectionalLight::SetRotation(const XMFLOAT3& InRotation)
{
    Rotation = InRotation;
    CalculateMatrix();
}

void DirectionalLight::SetRotation(float x, float y, float z)
{
    SetRotation(XMFLOAT3(x, y, z));
}

void DirectionalLight::SetLookAt(const XMFLOAT3& InLookAt)
{
    LookAt = InLookAt;
    CalculateMatrix();
}

void DirectionalLight::SetLookAt(float x, float y, float z)
{
    SetLookAt(XMFLOAT3(x, y, z));
}

void DirectionalLight::SetShadowNearPlane(float InShadowNearPlane)
{
    if (InShadowNearPlane > 0.0f)
    {
        if (abs(ShadowFarPlane - InShadowNearPlane) >= 0.1f)
        {
            ShadowNearPlane = InShadowNearPlane;
            CalculateMatrix();
        }
    }
}

void DirectionalLight::SetShadowFarPlane(float InShadowFarPlane)
{
    if (InShadowFarPlane > 0.0f)
    {
        if (abs(InShadowFarPlane - ShadowNearPlane) >= 0.1f)
        {
            ShadowFarPlane = InShadowFarPlane;
            CalculateMatrix();
        }
    }
}

void DirectionalLight::CalculateMatrix()
{
    XMVECTOR XmDirection = XMVectorSet(0.0, -1.0f, 0.0f, 0.0f);;
    XMMATRIX XmRotation  = XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z);
    XMVECTOR XmOffset    = XMVector3Transform(XmDirection, XmRotation);
    XmDirection	= XMVector3Normalize(XmOffset);
    XMStoreFloat3(&Direction, XmDirection);

    const float Scale = (ShadowFarPlane - ShadowNearPlane) / 2.0f;
    XmOffset = XMVectorScale(XmOffset, -Scale);

    XMVECTOR XmLookAt   = XMLoadFloat3(&LookAt);
    XMVECTOR XmPosition = XMVectorAdd(XmLookAt, XmOffset);
    XMStoreFloat3(&ShadowMapPosition, XmPosition);

    XMVECTOR XmUp = XMVectorSet(0.0, 0.0f, 1.0f, 0.0f);
    XmUp = XMVector3Normalize(XMVector3Transform(XmUp, XmRotation));

    const float Offset    = 35.0f;
    XMMATRIX XmProjection = XMMatrixOrthographicOffCenterLH(-Offset, Offset, -Offset, Offset, ShadowNearPlane, ShadowFarPlane);
    XMMATRIX XmView       = XMMatrixLookAtLH(XmPosition, XmLookAt, XmUp);

    XMStoreFloat4x4(&Matrix, XMMatrixMultiplyTranspose(XmView, XmProjection));
}
