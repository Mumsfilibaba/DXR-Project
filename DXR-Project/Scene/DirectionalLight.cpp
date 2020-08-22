#include "DirectionalLight.h"

DirectionalLight::DirectionalLight()
	: Light()
	, Direction(0.0f, -1.0f, 0.0f)
	, ShadowBias(0.005f)
	, ShadowMapPosition(0.0f, 30.0f, 0.0f)
	, Matrix()
{
	CORE_OBJECT_INIT();

	XMStoreFloat4x4(&Matrix, XMMatrixIdentity());
}

DirectionalLight::~DirectionalLight()
{
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

void DirectionalLight::CalculateMatrix()
{
	XMVECTOR LightDirection = XMVector3Normalize(XMLoadFloat3(&Direction));
	XMVECTOR LightPosition	= XMLoadFloat3(&ShadowMapPosition);
	XMVECTOR LightUp		= XMVectorSet(0.0, 0.0f, 1.0f, 0.0f);

	const Float32 Offset		= 30.0f;
	XMMATRIX LightProjection	= XMMatrixOrthographicOffCenterLH(-Offset, Offset, -Offset, Offset, 1.0f, 30.0f);
	XMMATRIX LightView			= XMMatrixLookToLH(LightPosition, LightDirection, LightUp);

	XMStoreFloat4x4(&Matrix, XMMatrixMultiplyTranspose(LightView, LightProjection));
}
