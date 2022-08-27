#pragma once
#include "Engine/EngineModule.h"

#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FCamera

class ENGINE_API FCamera
{
public:
    FCamera();
    ~FCamera() = default;

    void Move(float x, float y, float z);

    void Rotate(float Pitch, float Yaw, float Roll);

    void UpdateMatrices();

    const FMatrix4& GetViewMatrix() const
    {
        return View;
    }

    const FMatrix4& GetViewInverseMatrix() const
    {
        return ViewInverse;
    }

    const FMatrix4& GetProjectionMatrix() const
    {
        return Projection;
    }

    const FMatrix4& GetProjectionInverseMatrix() const
    {
        return ProjectionInverse;
    }

    const FMatrix4& GetViewProjectionMatrix() const
    {
        return ViewProjection;
    }

    const FMatrix4& GetViewProjectionInverseMatrix() const
    {
        return ViewProjectionInverse;
    }

    const FMatrix4& GetViewProjectionWitoutTranslateMatrix() const
    {
        return ViewProjectionNoTranslation;
    }

    FVector3 GetPosition() const
    {
        return Position;
    }

    FVector3 GetForward() const
    {
        return Forward;
    }

    FVector3 GetUp() const
    {
        return Up;
    }

    FVector3 GetRight() const
    {
        return Right;
    }

    float GetNearPlane() const
    {
        return NearPlane;
    }

    float GetFarPlane() const
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
    FMatrix4 View;
    FMatrix4 ViewInverse;
    FMatrix4 Projection;
    FMatrix4 ProjectionInverse;
    FMatrix4 ViewProjection;
    FMatrix4 ViewProjectionInverse;
    FMatrix4 ViewProjectionNoTranslation;

    float NearPlane;
    float FarPlane;
    float AspectRatio;
    float Width;
    float Height;
    float FOV;

    FVector3 Position;
    FVector3 Rotation;
    FVector3 Forward;
    FVector3 Right;
    FVector3 Up;
};