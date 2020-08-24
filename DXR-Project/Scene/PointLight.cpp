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

PointLight::~PointLight()
{
}

void PointLight::SetPosition(const XMFLOAT3& InPosition)
{
	Position = InPosition;
	CalculateMatrices();
}

void PointLight::SetPosition(Float32 X, Float32 Y, Float32 Z)
{
	Position = XMFLOAT3(X, Y, Z);
	CalculateMatrices();
}

void PointLight::SetShadowNearPlane(Float32 InShadowNearPlane)
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

void PointLight::SetShadowFarPlane(Float32 InShadowFarPlane)
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
	for (Uint32 i = 0; i < 6; i++)
	{
		XMVECTOR LightDirection	= XMLoadFloat3(&Directions[i]);
		XMVECTOR LightUp		= XMLoadFloat3(&UpVectors[i]);

		XMMATRIX LightProjection	= XMMatrixPerspectiveFovLH(XM_PI / 2.0f, 1.0f, ShadowNearPlane, ShadowFarPlane);
		XMMATRIX LightView			= XMMatrixLookToLH(LightPosition, LightDirection, LightUp);
		XMStoreFloat4x4(&Matrices[i], XMMatrixMultiplyTranspose(LightView, LightProjection));
	}
}
