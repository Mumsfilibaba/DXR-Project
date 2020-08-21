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

	FORCEINLINE XMFLOAT4X4 GetViewProjection() const
	{
		return ViewProjection;
	}

	FORCEINLINE XMFLOAT4X4 GetViewProjectionInverse() const
	{
		return ViewProjectionInverse;
	}

	FORCEINLINE XMFLOAT4X4 GetViewProjectionWitoutTranslate() const
	{
		return ViewProjectionNoTranslation;
	}

	FORCEINLINE XMFLOAT3 GetPosition() const
	{
		return Position;
	}

private:
	XMFLOAT4X4	ViewProjection;
	XMFLOAT4X4	ViewProjectionInverse;
	XMFLOAT4X4	ViewProjectionNoTranslation;
	XMFLOAT3	Position;
	XMFLOAT3	Rotation;
	XMFLOAT3	Forward;
	XMFLOAT3	Right;
	XMFLOAT3	Up;
};