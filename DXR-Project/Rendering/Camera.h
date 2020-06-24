#pragma once
#include "Types.h"

class Camera
{
public:
	Camera()
		: ViewProjection()
		, ViewProjectionInverse()
		, Position(0.0f, 0.0f, -2.0f)
		, Right(1.0f, 0.0f, 0.0f)
		, Up(0.0f, 1.0f, 0.0f)
		, Forward(0.0f, 0.0f, 1.0f)
		, Rotation()
	{
		UpdateMatrices();
	}

	void Move(Float32 X, Float32 Y, Float32 Z)
	{
		XMVECTOR XmPosition	= XMLoadFloat3(&Position);
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

	void Rotate(Float32 Pitch, Float32 Yaw, Float32 Roll)
	{
		XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(Pitch, Yaw, Roll);

		XMVECTOR XmForward	= XMLoadFloat3(&Forward);
		XmForward			= XMVector3Transform(XmForward, RotationMatrix);

		XMVECTOR XmUp	= XMLoadFloat3(&Up);
		XmUp			= XMVector3Transform(XmUp, RotationMatrix);

		XMVECTOR XmRight	= XMLoadFloat3(&Right);
		XmRight				= XMVector3Transform(XmRight, RotationMatrix);

		XMStoreFloat3(&Forward, XmForward);
		XMStoreFloat3(&Up, XmUp);
		XMStoreFloat3(&Right, XmRight);

		Rotation.x += Pitch;
		Rotation.y += Yaw;
		Rotation.z += Roll;
	}

	void UpdateMatrices()
	{
		Float32 Fov			= XMConvertToRadians(90.0f);
		XMMATRIX Projection = XMMatrixPerspectiveFovRH(Fov, 1920.0f / 1080.0f, 0.01f, 100.0f);

		XMVECTOR XmPosition = XMLoadFloat3(&Position);
		XMVECTOR XmForward	= XMLoadFloat3(&Forward);
		XMVECTOR XmUp		= XMLoadFloat3(&Up);
		XMVECTOR At			= XMVectorAdd(XmPosition, XmForward);
		XMMATRIX View		= XMMatrixLookAtRH(XmPosition, At, XmUp);

		XMMATRIX XmViewProjection			= XMMatrixMultiply(View, Projection);
		XMMATRIX XmViewProjectionInverse	= XMMatrixInverse(nullptr, XmViewProjection);

		XMStoreFloat4x4(&ViewProjection, XMMatrixTranspose(XmViewProjection));
		XMStoreFloat4x4(&ViewProjectionInverse, XMMatrixTranspose(XmViewProjectionInverse));
	}

public:
	XMFLOAT4X4	ViewProjection;
	XMFLOAT4X4	ViewProjectionInverse;
	XMFLOAT3	Position;
	XMFLOAT3	Forward;
	XMFLOAT3	Right;
	XMFLOAT3	Up;
	XMFLOAT3	Rotation;
};