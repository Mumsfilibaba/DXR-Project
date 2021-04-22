#pragma once
#include "Core.h"

class Camera
{
public:
    Camera();
    ~Camera() = default;

    void Move(Float x, Float y, Float z);

    void Rotate(Float Pitch, Float Yaw, Float Roll);

    void Tick(Float Width, Float Height);

    const XMFLOAT4X4& GetViewMatrix() const { return View; }
    const XMFLOAT4X4& GetViewInverseMatrix() const { return ViewInverse; }

    const XMFLOAT4X4& GetProjectionMatrix() const { return Projection; }
    const XMFLOAT4X4& GetProjectionInverseMatrix() const { return ProjectionInverse; }

    const XMFLOAT4X4& GetViewProjectionMatrix() const { return ViewProjection; }
    const XMFLOAT4X4& GetViewProjectionInverseMatrix() const { return ViewProjectionInverse; }
    const XMFLOAT4X4& GetViewProjectionWitoutTranslateMatrix() const { return ViewProjectionNoTranslation; }

    void SetPosition(Float x, Float y, Float z);
    void SetPosition(const XMFLOAT3& InPosition);

    XMFLOAT3 GetPosition() const { return Position; }
    XMFLOAT3 GetRotation() const { return Rotation; }
    XMFLOAT3 GetForward() const { return Forward; }
    XMFLOAT3 GetUp() const { return Up; }
    XMFLOAT3 GetRight() const { return Right; }

    Float GetNearPlane() const { return NearPlane; }
    Float GetFarPlane() const { return FarPlane; }

    Float GetAspectRatio() const { return AspectRatio; }

    XMFLOAT2 GetJitter() const { return Jitter; }

private:
    UInt64 Frame = 0;

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

    XMFLOAT2 Jitter;
};