#include "Camera.h"

Camera::Camera()
    : View()
    , Projection()
    , ViewProjection()
    , ViewProjectionInverse()
    , Position(0.0f, 0.0f, -2.0f)
    , Right(-1.0f, 0.0f, 0.0f)
    , Up(0.0f, 1.0f, 0.0f)
    , Forward(0.0f, 0.0f, 1.0f)
    , Rotation(0.0f, 0.0f, 0.0f, 0.0f)
    , NearPlane(0.01f)
    , FarPlane(1000.0f)
    , AspectRatio()
{
    XMStoreFloat4(&Rotation, XMQuaternionIdentity());
    Tick(1920.0f, 1080.0f);
}

void Camera::Move(Float x, Float y, Float z)
{
    XMVECTOR XmPosition = XMLoadFloat3(&Position);
    XMVECTOR XmRight    = XMLoadFloat3(&Right);
    XMVECTOR XmUp       = XMLoadFloat3(&Up);
    XMVECTOR XmForward  = XMLoadFloat3(&Forward);
    XmRight             = XMVectorScale(XmRight, x);
    XmUp                = XMVectorScale(XmUp, y);
    XmForward           = XMVectorScale(XmForward, z);
    XmPosition          = XMVectorAdd(XmPosition, XmRight);
    XmPosition          = XMVectorAdd(XmPosition, XmUp);
    XmPosition          = XMVectorAdd(XmPosition, XmForward);

    XMStoreFloat3(&Position, XmPosition);
}

void Camera::Translate(Float x, Float y, Float z)
{
    Position = Position + XMFLOAT3(x, y, z);
}

void Camera::TranslateForward(Float x, Float y, Float z)
{
    XMVECTOR XMTranslation = XMVectorSet(x, y, z, 0.0f);
    XMVECTOR XMForward     = XMLoadFloat3(&Forward);
    XMForward = XMVectorAdd(XMForward, XMTranslation);

    XMVECTOR XMUp    = XMLoadFloat3(&Up);
    XMVECTOR XMRight = XMLoadFloat3(&Right);

    // Re-orthogonalize the vectors
    XMUp    = XMVector3Normalize(XMVectorSubtract(XMUp, XMVectorMultiply(XMForward, XMVector3Dot(XMUp, XMForward))));
    XMRight = XMVector3Normalize(XMVectorSubtract(XMRight, XMVectorMultiply(XMForward, XMVector3Dot(XMRight, XMForward))));

    XMStoreFloat3(&Forward, XMForward);
    XMStoreFloat3(&Up, XMUp);
    XMStoreFloat3(&Right, XMRight);
}

void Camera::Rotate(Float Pitch, Float Yaw, Float Roll)
{
    XMVECTOR CurrentRotation = XMLoadFloat4(&Rotation);
    XMVECTOR NewRotation = XMQuaternionRotationRollPitchYaw(Pitch, Yaw, Roll);
    
    CurrentRotation = XMQuaternionMultiply(NewRotation, CurrentRotation);

    XMMATRIX RotationMatrix = XMMatrixRotationQuaternion(CurrentRotation);
    XMVECTOR XmForward      = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    XmForward               = XMVector3Normalize(XMVector3Transform(XmForward, RotationMatrix));

    XMVECTOR XmUp    = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR XmRight = XMVector3Normalize(XMVector3Cross(XmForward, XmUp));
    XmUp             = XMVector3Normalize(XMVector3Cross(XmRight, XmForward));

    XMStoreFloat3(&Forward, XmForward);
    XMStoreFloat3(&Up, XmUp);
    XMStoreFloat3(&Right, XmRight);
    XMStoreFloat4(&Rotation, CurrentRotation);
}

void Camera::Tick(Float Width, Float Height)
{
    Frame  = (++Frame) % 8;
    Jitter = Math::Hammersley((UInt32)Frame, 8);
    Jitter = (Jitter * 2.0f) - 1.0f;

    XMFLOAT2 ClipSpaceJitter = Jitter / XMFLOAT2(Width, Height);
    ClipSpaceJitter.y = -ClipSpaceJitter.y;

    Float Fov = XMConvertToRadians(90.0f);
    AspectRatio = Width / Height;

    XMMATRIX XmOffset     = XMMatrixIdentity();// XMMatrixTranslation(ClipSpaceJitter.x, ClipSpaceJitter.y, 0.0f);
    XMMATRIX XmProjection = XMMatrixPerspectiveFovLH(Fov, AspectRatio, NearPlane, FarPlane);
    XmProjection = XMMatrixMultiply(XmOffset, XmProjection);
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
    
    XMMATRIX XmViewProjectionNoTranslation = XMMatrixMultiply(XmView3x3, XmProjection);
    XMStoreFloat4x4(&ViewProjectionNoTranslation, XMMatrixTranspose(XmViewProjectionNoTranslation));
}

void Camera::SetPosition(Float x, Float y, Float z)
{
    SetPosition(XMFLOAT3(x, y, z));
}

void Camera::SetPosition(const XMFLOAT3& InPosition)
{
    Position = InPosition;
}

void Camera::SetRotation(const XMFLOAT4& InRotation)
{
    XMVECTOR XmQuaternion = XMLoadFloat4(&InRotation);

    XMMATRIX XmRotationMatrix = XMMatrixRotationQuaternion(XmQuaternion);

    XMVECTOR XmForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    XmForward = XMVector3Normalize(XMVector3Transform(XmForward, XmRotationMatrix));

    XMVECTOR XmUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR XmRight = XMVector3Normalize(XMVector3Cross(XmForward, XmUp));
    XmUp = XMVector3Normalize(XMVector3Cross(XmRight, XmForward));

    Rotation = InRotation;

    XMStoreFloat3(&Forward, XmForward);
    XMStoreFloat3(&Up, XmUp);
    XMStoreFloat3(&Right, XmRight);
}

// https://stackoverflow.com/questions/60350349/directx-get-pitch-yaw-roll-from-xmmatrix
XMFLOAT3 Camera::GetRotationInEulerAngles() const
{
    XMVECTOR Quaternion = XMLoadFloat4(&Rotation);

    XMFLOAT4X4 Matrix;
    XMStoreFloat4x4(&Matrix, XMMatrixTranspose(XMMatrixRotationQuaternion(Quaternion)));

    XMFLOAT3 Euler;
    Euler.x = (float)asin(-Matrix._23);
    Euler.y = (float)atan2(Matrix._13, Matrix._33);
    Euler.z = (float)atan2(Matrix._21, Matrix._22);

    return Euler;
}
