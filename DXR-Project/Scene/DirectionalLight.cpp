#include "DirectionalLight.h"

DirectionalLight::DirectionalLight()
	: Light()
	, Direction(0.0f, -1.0f, 0.0f)
	, Rotation(0.0f, 0.0f, 0.0f)
	, ShadowMapPosition(0.0f, 30.0f, 0.0f)
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

void DirectionalLight::SetRotation(Float32 X, Float32 Y, Float32 Z)
{
	Rotation = XMFLOAT3(X, Y, Z);
	CalculateMatrix();
}

void DirectionalLight::SetDirection(const XMFLOAT3& InDirection)
{
	XMVECTOR XmDir = XMVectorSet(InDirection.x, InDirection.y, InDirection.z, 0.0f);
	XmDir = XMVector3Normalize(XmDir);
	XMStoreFloat3(&Direction, XmDir);

	CalculateMatrix();
}

void DirectionalLight::SetDirection(Float32 X, Float32 Y, Float32 Z)
{
	XMVECTOR XmDir = XMVectorSet(X, Y, Z, 0.0f);
	XmDir = XMVector3Normalize(XmDir);
	XMStoreFloat3(&Direction, XmDir);

	CalculateMatrix();
}

void DirectionalLight::SetShadowMapPosition(const XMFLOAT3& InPosition)
{
	ShadowMapPosition = InPosition;
	CalculateMatrix();
}

void DirectionalLight::SetShadowMapPosition(Float32 X, Float32 Y, Float32 Z)
{
	ShadowMapPosition = XMFLOAT3(X, Y, Z);
	CalculateMatrix();
}

void DirectionalLight::SetShadowNearPlane(Float32 InShadowNearPlane)
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

void DirectionalLight::SetShadowFarPlane(Float32 InShadowFarPlane)
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
	XMFLOAT3 StartDirection = XMFLOAT3(0.0f, -1.0f, 0.0f);
	XMVECTOR LightDirection = XMLoadFloat3(&StartDirection);
	XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z);
	LightDirection = XMVector3Normalize(XMVector3Transform(LightDirection, RotationMatrix));
	XMStoreFloat3(&Direction, LightDirection);

	XMVECTOR LightPosition	= XMLoadFloat3(&ShadowMapPosition);
	XMVECTOR LightUp		= XMVectorSet(0.0, 0.0f, 1.0f, 0.0f);

	const Float32 Offset		= 30.0f;
	XMMATRIX LightProjection	= XMMatrixOrthographicOffCenterLH(-Offset, Offset, -Offset, Offset, ShadowNearPlane, ShadowFarPlane);
	XMMATRIX LightView			= XMMatrixLookToLH(LightPosition, LightDirection, LightUp);

	XMStoreFloat4x4(&Matrix, XMMatrixMultiplyTranspose(LightView, LightProjection));
}
