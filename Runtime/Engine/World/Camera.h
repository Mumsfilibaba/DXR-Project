#pragma once
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"

class ENGINE_API FCamera
{
public:
    FCamera();
    ~FCamera();

    void Move(float x, float y, float z);
    void Rotate(float Pitch, float Yaw, float Roll);
    
    // NOTE: FieldOfView in degrees 
    void SetFieldOfView(float InFieldOfView);
    void SetNearPlane(float InNearPlane);
    void SetFarPlane(float InFarPlane);
    void SetPosition(float x, float y, float z);
    void SetRotation(float Pitch, float Yaw, float Roll);

    void UpdateProjectionMatrix(float InViewportWidth, float InViewportHeight);
    void UpdateViewMatrix();
    void UpdateWorldToClipSpaceMatrices();

    FORCEINLINE const Matrix4& GetViewMatrix() const
    {
        return View;
    }

    FORCEINLINE const Matrix4& GetViewInverseMatrix() const
    {
        return ViewInverse;
    }

    FORCEINLINE const Matrix4& GetProjectionMatrix() const
    {
        return Projection;
    }

    FORCEINLINE const Matrix4& GetProjectionInverseMatrix() const
    {
        return ProjectionInverse;
    }

    FORCEINLINE const Matrix4& GetViewProjectionMatrix() const
    {
        return ViewProjection;
    }

    FORCEINLINE const Matrix4& GetViewProjectionInverseMatrix() const
    {
        return ViewProjectionInverse;
    }

    FORCEINLINE const Matrix4& GetViewProjectionWitoutTranslateMatrix() const
    {
        return ViewProjectionNoTranslation;
    }

    FORCEINLINE const Vector3& GetPosition() const
    {
        return Position;
    }

    FORCEINLINE const Vector3& GetRotation() const
    {
        return Rotation;
    }

    FORCEINLINE const Vector3& GetForwardVector() const
    {
        return ForwardVector;
    }

    FORCEINLINE const Vector3& GetUpVector() const
    {
        return UpVector;
    }

    FORCEINLINE const Vector3& GetRightVector() const
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
    void UpdateDirectionVectors();

    Matrix4 View;
    Matrix4 ViewInverse;
    Matrix4 Projection;
    Matrix4 ProjectionInverse;
    Matrix4 ViewProjection;
    Matrix4 ViewProjectionInverse;
    Matrix4 ViewProjectionNoTranslation;
    float   NearPlane;
    float   FarPlane;
    float   AspectRatio;
    float   ViewportWidth;
    float   ViewportHeight;
    float   FieldOfView;
    Vector3 Position;
    Vector3 Rotation;
    Vector3 ForwardVector;
    Vector3 RightVector;
    Vector3 UpVector;
};
