#pragma once
#include "Core/Math/Matrix4.h"
#include "Core/Math/Vector3.h"
#include "Engine/World/Components/SceneComponent.h"

class ENGINE_API FCameraComponent : public FSceneComponent
{
public:
    FOBJECT_DECLARE_CLASS(FCameraComponent, FSceneComponent);

    FCameraComponent(const FObjectInitializer& ObjectInitializer);
    ~FCameraComponent();

    void AddLocalMovement(float x, float y, float z);
    void AddRotation(float Pitch, float Yaw, float Roll);

    void SetFieldOfView(float InFieldOfView);
    void SetNearPlane(float InNearPlane);
    void SetFarPlane(float InFarPlane);
    void SetPosition(float x, float y, float z);
    void SetPosition(const Vector3& InPosition);
    void SetRotation(float Pitch, float Yaw, float Roll);
    void SetRotation(const Vector3& InRotation);

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

    const Vector3& GetPosition() const;
    const Vector3& GetRotation() const;

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
    Vector3 ForwardVector;
    Vector3 RightVector;
    Vector3 UpVector;
};
