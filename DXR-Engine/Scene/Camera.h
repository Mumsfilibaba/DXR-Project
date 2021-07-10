#pragma once
#include "Core.h"

class Camera
{
public:
    Camera();
    ~Camera() = default;

    void Move( float X, float Y, float Z );

    void Rotate( float Pitch, float Yaw, float Roll );

    void UpdateMatrices();

    const XMFLOAT4X4& GetViewMatrix() const
    {
        return View;
    }
    const XMFLOAT4X4& GetViewInverseMatrix() const
    {
        return ViewInverse;
    }

    const XMFLOAT4X4& GetProjectionMatrix() const
    {
        return Projection;
    }
    const XMFLOAT4X4& GetProjectionInverseMatrix() const
    {
        return ProjectionInverse;
    }

    const XMFLOAT4X4& GetViewProjectionMatrix() const
    {
        return ViewProjection;
    }
    const XMFLOAT4X4& GetViewProjectionInverseMatrix() const
    {
        return ViewProjectionInverse;
    }
    const XMFLOAT4X4& GetViewProjectionWitoutTranslateMatrix() const
    {
        return ViewProjectionNoTranslation;
    }

    XMFLOAT3 GetPosition() const
    {
        return Position;
    }

    XMFLOAT3 GetForward() const
    {
        return Forward;
    }
    XMFLOAT3 GetUp() const
    {
        return Up;
    }
    XMFLOAT3 GetRight() const
    {
        return Right;
    }

    float GetNearPlane() const
    {
        return NearPlane;
    }
    float GetFarPlane()  const
    {
        return FarPlane;
    }

    float GetAspectRatio() const
    {
        return AspectRatio;
    }

    float GetWidth() const
    {
        return Width;
    }
    float GetHeight() const
    {
        return Height;
    }

    float GetFOV() const
    {
        return FOV;
    }

private:
    XMFLOAT4X4 View;
    XMFLOAT4X4 ViewInverse;
    XMFLOAT4X4 Projection;
    XMFLOAT4X4 ProjectionInverse;
    XMFLOAT4X4 ViewProjection;
    XMFLOAT4X4 ViewProjectionInverse;
    XMFLOAT4X4 ViewProjectionNoTranslation;

    float NearPlane;
    float FarPlane;
    float AspectRatio;
    float Width;
    float Height;
    float FOV;

    XMFLOAT3 Position;
    XMFLOAT3 Rotation;
    XMFLOAT3 Forward;
    XMFLOAT3 Right;
    XMFLOAT3 Up;
};