#pragma once
#include "Core/Math/Matrix4.h"
#include "Core/Math/Vector3.h"
#include "Engine/World/Components/SceneComponent.h"

struct FCameraSnapshot;

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
    void SetPosition(const FVector3& InPosition);
    void SetRotation(float Pitch, float Yaw, float Roll);
    void SetRotation(const FVector3& InRotation);

    void UpdateProjectionMatrix(float InViewportWidth, float InViewportHeight);
    void UpdateViewMatrix();
    void UpdateWorldToClipSpaceMatrices();
    void PrepareSceneViewInfo(FCameraSnapshot& OutCameraSnapshot) const;

    const FVector3& GetPosition() const;
    const FVector3& GetRotation() const;

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

    FORCEINLINE const FVector3& GetForwardVector() const
    {
        return ForwardVector;
    }

    FORCEINLINE FVector3 GetViewLocationAtDistance(float Distance) const
    {
        return GetPosition() + (GetForwardVector() * Distance);
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
    void UpdateDirectionVectors();

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
    FVector3 ForwardVector;
    FVector3 RightVector;
    FVector3 UpVector;
};
