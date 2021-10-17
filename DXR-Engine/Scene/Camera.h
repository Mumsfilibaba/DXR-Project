#pragma once
#include "Core.h"

#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"

class CORE_API CCamera
{
public:
    CCamera();
    ~CCamera() = default;

    void Move( float x, float y, float z );

    void Rotate( float Pitch, float Yaw, float Roll );

    void UpdateMatrices();

    const CMatrix4& GetViewMatrix() const
    {
        return View;
    }
    const CMatrix4& GetViewInverseMatrix() const
    {
        return ViewInverse;
    }

    const CMatrix4& GetProjectionMatrix() const
    {
        return Projection;
    }
    const CMatrix4& GetProjectionInverseMatrix() const
    {
        return ProjectionInverse;
    }

    const CMatrix4& GetViewProjectionMatrix() const
    {
        return ViewProjection;
    }
    const CMatrix4& GetViewProjectionInverseMatrix() const
    {
        return ViewProjectionInverse;
    }
    const CMatrix4& GetViewProjectionWitoutTranslateMatrix() const
    {
        return ViewProjectionNoTranslation;
    }

    CVector3 GetPosition() const
    {
        return Position;
    }

    CVector3 GetForward() const
    {
        return Forward;
    }
    CVector3 GetUp() const
    {
        return Up;
    }
    CVector3 GetRight() const
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
    CMatrix4 View;
    CMatrix4 ViewInverse;
    CMatrix4 Projection;
    CMatrix4 ProjectionInverse;
    CMatrix4 ViewProjection;
    CMatrix4 ViewProjectionInverse;
    CMatrix4 ViewProjectionNoTranslation;

    float NearPlane;
    float FarPlane;
    float AspectRatio;
    float Width;
    float Height;
    float FOV;

    CVector3 Position;
    CVector3 Rotation;
    CVector3 Forward;
    CVector3 Right;
    CVector3 Up;
};