#pragma once
#include "Core.h"

class Camera
{
public:
    Camera();
    ~Camera() = default;

    void Move(Float X, Float Y, Float Z);

    void Rotate(Float Pitch, Float Yaw, Float Roll);

    void UpdateMatrices();

    const XMFLOAT4X4& GetViewMatrix() const { return View; }
    const XMFLOAT4X4& GetViewInverseMatrix() const { return ViewInverse; }

    const XMFLOAT4X4& GetProjectionMatrix() const { return Projection; }
    const XMFLOAT4X4& GetProjectionInverseMatrix() const { return ProjectionInverse; }

    const XMFLOAT4X4& GetViewProjectionMatrix() const { return ViewProjection; }
    const XMFLOAT4X4& GetViewProjectionInverseMatrix() const { return ViewProjectionInverse; }
    const XMFLOAT4X4& GetViewProjectionWitoutTranslateMatrix() const { return ViewProjectionNoTranslation; }

    XMFLOAT3 GetPosition() const { return Position; }

    Float GetNearPlane() const { return NearPlane; }
    Float GetFarPlane() const { return FarPlane; }

    Float GetAspectRatio() const { return AspectRatio; }

private:
    XMFLOAT4X4 View;
    XMFLOAT4X4 ViewInverse;
    XMFLOAT4X4 Projection;
    XMFLOAT4X4 ProjectionInverse;
    XMFLOAT4X4 ViewProjection;
    XMFLOAT4X4 ViewProjectionInverse;
    XMFLOAT4X4 ViewProjectionNoTranslation;
    Float NearPlane;
    Float FarPlane;
    Float AspectRatio;
    XMFLOAT3 Position;
    XMFLOAT3 Rotation;
    XMFLOAT3 Forward;
    XMFLOAT3 Right;
    XMFLOAT3 Up;
};