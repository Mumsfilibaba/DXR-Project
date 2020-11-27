#pragma once
#include "Types.h"

/*
* Camera
*/

class Camera
{
public:
	Camera();
	~Camera() = default;

	void Move(Float X, Float Y, Float Z);

	void Rotate(Float Pitch, Float Yaw, Float Roll);

	void UpdateMatrices();

	FORCEINLINE const XMFLOAT4X4& GetViewMatrix() const
	{
		return View;
	}

	FORCEINLINE const XMFLOAT4X4& GetProjectionMatrix() const
	{
		return Projection;
	}

	FORCEINLINE const XMFLOAT4X4& GetViewProjectionMatrix() const
	{
		return ViewProjection;
	}

	FORCEINLINE const XMFLOAT4X4& GetViewProjectionInverseMatrix() const
	{
		return ViewProjectionInverse;
	}

	FORCEINLINE const XMFLOAT4X4& GetViewProjectionWitoutTranslateMatrix() const
	{
		return ViewProjectionNoTranslation;
	}

	FORCEINLINE XMFLOAT3 GetPosition() const
	{
		return Position;
	}

	FORCEINLINE Float GetNearPlane() const
	{
		return NearPlane;
	}

	FORCEINLINE Float GetFarPlane() const
	{
		return FarPlane;
	}

private:
	XMFLOAT4X4	View;
	XMFLOAT4X4	Projection;
	XMFLOAT4X4	ViewProjection;
	XMFLOAT4X4	ViewProjectionInverse;
	XMFLOAT4X4	ViewProjectionNoTranslation;
	XMFLOAT3	Position;
	XMFLOAT3	Rotation;
	XMFLOAT3	Forward;
	XMFLOAT3	Right;
	XMFLOAT3	Up;
	Float		NearPlane;
	Float		FarPlane;
};