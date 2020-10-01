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

	void Move(Float32 X, Float32 Y, Float32 Z);

	void Rotate(Float32 Pitch, Float32 Yaw, Float32 Roll);

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

	FORCEINLINE Float32 GetNearPlane() const
	{
		return NearPlane;
	}

	FORCEINLINE Float32 GetFarPlane() const
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
	Float32		NearPlane;
	Float32		FarPlane;
};