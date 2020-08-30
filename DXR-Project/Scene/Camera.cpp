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
{
	UpdateMatrices();
}

void Camera::Move(Float32 X, Float32 Y, Float32 Z)
{
	XMVECTOR XmPosition = XMLoadFloat3(&Position);
	XMVECTOR XmRight	= XMLoadFloat3(&Right);
	XMVECTOR XmUp		= XMLoadFloat3(&Up);
	XMVECTOR XmForward	= XMLoadFloat3(&Forward);
	XmRight				= XMVectorScale(XmRight, X);
	XmUp				= XMVectorScale(XmUp, Y);
	XmForward			= XMVectorScale(XmForward, Z);
	XmPosition			= XMVectorAdd(XmPosition, XmRight);
	XmPosition			= XMVectorAdd(XmPosition, XmUp);
	XmPosition			= XMVectorAdd(XmPosition, XmForward);

	XMStoreFloat3(&Position, XmPosition);
}

void Camera::Rotate(Float32 Pitch, Float32 Yaw, Float32 Roll)
{
	Rotation.x += Pitch;
	Rotation.x = std::max<Float32>(XMConvertToRadians(-89.0f), std::min<Float32>(XMConvertToRadians(89.0f), Rotation.x));
	
	Rotation.y += Yaw;
	Rotation.z += Roll;

	XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z);
	XMVECTOR XmForward		= XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XmForward				= XMVector3Normalize(XMVector3Transform(XmForward, RotationMatrix));

	XMVECTOR XmUp		= XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR XmRight	= XMVector3Normalize(XMVector3Cross(XmForward, XmUp));
	XmUp				= XMVector3Normalize(XMVector3Cross(XmRight, XmForward));

	XMStoreFloat3(&Forward, XmForward);
	XMStoreFloat3(&Up, XmUp);
	XMStoreFloat3(&Right, XmRight);
}

void Camera::UpdateMatrices()
{
	Float32 Fov	= XMConvertToRadians(90.0f);
	XMMATRIX XmProjection = XMMatrixPerspectiveFovLH(Fov, 1920.0f / 1080.0f, NearPlane, FarPlane);
	XMStoreFloat4x4(&Projection, XmProjection);

	XMVECTOR XmPosition = XMLoadFloat3(&Position);
	XMVECTOR XmForward	= XMLoadFloat3(&Forward);
	XMVECTOR XmUp		= XMLoadFloat3(&Up);
	XMVECTOR XmAt		= XMVectorAdd(XmPosition, XmForward);
	XMMATRIX XmView		= XMMatrixLookAtLH(XmPosition, XmAt, XmUp);
	XMStoreFloat4x4(&View, XmView);

	XMFLOAT3X3 TempView3x3;
	XMStoreFloat3x3(&TempView3x3, XmView);
	XMMATRIX XmView3x3 = XMLoadFloat3x3(&TempView3x3);

	XMMATRIX XmViewProjection				= XMMatrixMultiply(XmView, XmProjection);
	XMMATRIX XmViewProjectionInverse		= XMMatrixInverse(nullptr, XmViewProjection);
	XMMATRIX XmViewProjectionNoTranslation	= XMMatrixMultiply(XmView3x3, XmProjection);

	XMStoreFloat4x4(&ViewProjection, XMMatrixTranspose(XmViewProjection));
	XMStoreFloat4x4(&ViewProjectionInverse, XMMatrixTranspose(XmViewProjectionInverse));
	XMStoreFloat4x4(&ViewProjectionNoTranslation, XMMatrixTranspose(XmViewProjectionNoTranslation));
}
