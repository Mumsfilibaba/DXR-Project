#include "Camera.h"

#include <algorithm>

Camera::Camera()
    : View()
    , Projection()
    , ViewProjection()
    , ViewProjectionInverse()
    , Position(0.0f, 0.0f, -2.0f)
    , Right(-1.0f, 0.0f, 0.0f)
    , Up(0.0f, 1.0f, 0.0f)
    , Forward(0.0f, 0.0f, 1.0f)
    , Rotation(0.0f, 0.0f, 0.0f)
    , NearPlane(0.01f)
    , FarPlane(1000.0f)
    , AspectRatio()
{
    UpdateMatrices();
}

void Camera::Move(float X, float Y, float Z)
{
    XMVECTOR XmPosition = XMLoadFloat3(&Position);
    XMVECTOR XmRight    = XMLoadFloat3(&Right);
    XMVECTOR XmUp       = XMLoadFloat3(&Up);
    XMVECTOR XmForward  = XMLoadFloat3(&Forward);
    XmRight             = XMVectorScale(XmRight, X);
    XmUp                = XMVectorScale(XmUp, Y);
    XmForward           = XMVectorScale(XmForward, Z);
    XmPosition          = XMVectorAdd(XmPosition, XmRight);
    XmPosition          = XMVectorAdd(XmPosition, XmUp);
    XmPosition          = XMVectorAdd(XmPosition, XmForward);

    XMStoreFloat3(&Position, XmPosition);
}

void Camera::Rotate(float Pitch, float Yaw, float Roll)
{
    Rotation.x += Pitch;
    Rotation.x = std::max<float>(XMConvertToRadians(-89.0f), std::min<float>(XMConvertToRadians(89.0f), Rotation.x));
    
    Rotation.y += Yaw;
    Rotation.z += Roll;

    XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z);
    XMVECTOR XmForward      = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    XmForward               = XMVector3Normalize(XMVector3Transform(XmForward, RotationMatrix));

    XMVECTOR XmUp    = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR XmRight = XMVector3Normalize(XMVector3Cross(XmForward, XmUp));
    XmUp             = XMVector3Normalize(XMVector3Cross(XmRight, XmForward));

    XMStoreFloat3(&Forward, XmForward);
    XMStoreFloat3(&Up, XmUp);
    XMStoreFloat3(&Right, XmRight);
}

void Camera::UpdateMatrices()
{
    float Fov = XMConvertToRadians(90.0f);
    AspectRatio = 1920.0f / 1080.0f;

    XMMATRIX XmProjection = XMMatrixPerspectiveFovLH(Fov, AspectRatio, NearPlane, FarPlane);
    XMStoreFloat4x4(&Projection, XmProjection);

    XMVECTOR XmPosition = XMLoadFloat3(&Position);
    XMVECTOR XmForward  = XMLoadFloat3(&Forward);
    XMVECTOR XmUp       = XMLoadFloat3(&Up);
    XMVECTOR XmAt       = XMVectorAdd(XmPosition, XmForward);

    XMMATRIX XmView = XMMatrixLookAtLH(XmPosition, XmAt, XmUp);
    XMStoreFloat4x4(&View, XMMatrixTranspose(XmView));

    XMMATRIX XmViewInv = XMMatrixInverse(nullptr, XmView);
    XMStoreFloat4x4(&ViewInverse, XMMatrixTranspose(XmViewInv));

    XMFLOAT3X3 TempView3x3;
    XMStoreFloat3x3(&TempView3x3, XmView);
    XMMATRIX XmView3x3 = XMLoadFloat3x3(&TempView3x3);

    XMMATRIX XmProjectionInverse = XMMatrixInverse(nullptr, XmProjection);
    XMStoreFloat4x4(&ProjectionInverse, XMMatrixTranspose(XmProjectionInverse));

    XMMATRIX XmViewProjection = XMMatrixMultiply(XmView, XmProjection);
    XMStoreFloat4x4(&ViewProjection, XMMatrixTranspose(XmViewProjection));

    XMMATRIX XmViewProjectionInverse = XMMatrixInverse(nullptr, XmViewProjection);
    XMStoreFloat4x4(&ViewProjectionInverse, XMMatrixTranspose(XmViewProjectionInverse));
    
    XMMATRIX XmViewProjectionNoTranslation    = XMMatrixMultiply(XmView3x3, XmProjection);
    XMStoreFloat4x4(&ViewProjectionNoTranslation, XMMatrixTranspose(XmViewProjectionNoTranslation));
}
