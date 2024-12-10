#pragma once
#include "Engine/EngineModule.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"

class ENGINE_API FCamera
{
public:
    FCamera();
    ~FCamera();

    void Move(float x, float y, float z);
    void Rotate(float Pitch, float Yaw, float Roll);

    // Note: FieldOfView in degrees 
    void UpdateProjectionMatrix(float InFieldOfView, float InViewportWidth, float InViewportHeight);
    void UpdateViewMatrix();

    void UpdateMatrices();

    FORCEINLINE const FMatrix4& GetViewMatrix() const
    {
        return View;
    }

    FORCEINLINE const FMatrix4& GetViewInverseMatrix() const
    {
        return ViewInverse;
    }

    FORCEINLINE const FMatrix4& GetProjectionMatrix() const
    {
        return Projection;
    }

    FORCEINLINE const FMatrix4& GetProjectionInverseMatrix() const
    {
        return ProjectionInverse;
    }

    FORCEINLINE const FMatrix4& GetViewProjectionMatrix() const
    {
        return ViewProjection;
    }

    FORCEINLINE const FMatrix4& GetViewProjectionInverseMatrix() const
    {
        return ViewProjectionInverse;
    }

    FORCEINLINE const FMatrix4& GetViewProjectionWitoutTranslateMatrix() const
    {
        return ViewProjectionNoTranslation;
    }

    FORCEINLINE const FVector3& GetPosition() const
    {
        return Position;
    }

    FORCEINLINE const FVector3& GetForwardVector() const
    {
        return ForwardVector;
    }

    FORCEINLINE const FVector3& GetUpVector() const
    {
        return UpVector;
    }

    FORCEINLINE const FVector3& GetRightVector() const
    {
        return RightVector;
    }

    FORCEINLINE float GetNearPlane() const
    {
        return NearPlane;
    }

    FORCEINLINE float GetFarPlane() const
    {
        return FarPlane;
    }

    FORCEINLINE float GetAspectRatio() const
    {
        return AspectRatio;
    }

    FORCEINLINE float GetWidth() const
    {
        return ViewportWidth;
    }

    FORCEINLINE float GetHeight() const
    {
        return ViewportHeight;
    }

    FORCEINLINE float GetFieldOfView() const
    {
        return FieldOfView;
    }

private:
    FMatrix4 View;
    FMatrix4 ViewInverse;
    FMatrix4 Projection;
    FMatrix4 ProjectionInverse;
    FMatrix4 ViewProjection;
    FMatrix4 ViewProjectionInverse;
    FMatrix4 ViewProjectionNoTranslation;

    float    NearPlane;
    float    FarPlane;
    float    AspectRatio;
    float    ViewportWidth;
    float    ViewportHeight;
    float    FieldOfView;

    FVector3 Position;
    FVector3 Rotation;
    FVector3 ForwardVector;
    FVector3 RightVector;
    FVector3 UpVector;
};