#include "PointLight.h"

#include "Rendering/Renderer.h"

PointLight::PointLight()
	: Light()
	, Position(0.0f, 0.0f, 0.0f)
	, Matrices()
{
	CORE_OBJECT_INIT();
	CalculateMatrices();
}

void PointLight::SetPosition(const XMFLOAT3& InPosition)
{
	Position = InPosition;
	CalculateMatrices();
}

void PointLight::SetPosition(Float x, Float y, Float z)
{
	SetPosition(XMFLOAT3(x, y, z));
}

void PointLight::SetShadowNearPlane(Float InShadowNearPlane)
{
	if (InShadowNearPlane > 0.0f)
	{
		if (abs(ShadowFarPlane - InShadowNearPlane) >= 0.1f)
		{
			ShadowNearPlane = InShadowNearPlane;
			CalculateMatrices();
		}
	}
}

void PointLight::SetShadowFarPlane(Float InShadowFarPlane)
{
	if (InShadowFarPlane > 0.0f)
	{
		if (abs(InShadowFarPlane - ShadowNearPlane) >= 0.1f)
		{
			ShadowFarPlane = InShadowFarPlane;
			CalculateMatrices();
		}
	}
}

void PointLight::CalculateMatrices()
{
	if (!ShadowCaster)
	{
		return;
	}

	XMFLOAT3 Directions[6] = 
	{
		{  1.0f,  0.0f,  0.0f },
		{ -1.0f,  0.0f,  0.0f },
		{  0.0f,  1.0f,  0.0f },
		{  0.0f, -1.0f,  0.0f },
		{  0.0f,  0.0f,  1.0f },
		{  0.0f,  0.0f, -1.0f },
	};

	XMFLOAT3 UpVectors[6] = 
	{
		{ 0.0f, 1.0f,  0.0f },
		{ 0.0f, 1.0f,  0.0f },
		{ 0.0f, 0.0f, -1.0f },
		{ 0.0f, 0.0f,  1.0f },
		{ 0.0f, 1.0f,  0.0f },
		{ 0.0f, 1.0f,  0.0f },
	};

	XMVECTOR LightPosition = XMLoadFloat3(&Position);
	for (UInt32 i = 0; i < 6; i++)
	{
		XMVECTOR LightDirection	= XMLoadFloat3(&Directions[i]);
		XMVECTOR LightUp		= XMLoadFloat3(&UpVectors[i]);

		XMMATRIX LightProjection = XMMatrixPerspectiveFovLH(XM_PI / 2.0f, 1.0f, ShadowNearPlane, ShadowFarPlane);
		XMStoreFloat4x4(&ProjMatrices[i], LightProjection);

		XMMATRIX LightView = XMMatrixLookToLH(LightPosition, LightDirection, LightUp);
		XMStoreFloat4x4(&ViewMatrices[i], XMMatrixTranspose(LightView));
		XMStoreFloat4x4(&Matrices[i], XMMatrixMultiplyTranspose(LightView, LightProjection));
	}
}
