#pragma once
#include "Core/Containers/UniquePtr.h"
#include "Core/Math/IntVector2.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"

class FActor;
class FCameraActor;
class FCameraComponent;

struct FEditorCameraInputState
{
    Vector3 MoveAxis;
    Vector2 LookDelta;
    Vector2 RotationAxis;
    Vector2 PanDelta;
    float   WheelDelta       = 0.0f;
    bool    bLeftMouseDown   = false;
    bool    bRightMouseDown  = false;
    bool    bMiddleMouseDown = false;
    bool    bAltDown         = false;
    bool    bCmdDown         = false;
    bool    bBoost           = false;
    bool    bFlyActive       = false;
    bool    bFocusPressed    = false;
    bool    bResetPressed    = false;
};

class FEditorCameraController
{
public:
    FEditorCameraController();
    ~FEditorCameraController();

    void Tick(float DeltaTime, const FEditorCameraInputState& Input, FActor* SelectedActor);
    void UpdateProjection(const IntVector2& ViewportSize);

    FCameraComponent* GetCamera() const;

    void Reset();
    void FocusOn(FActor* Actor);
    void AttachTo(FActor* Actor);
    void Detach();
    void OnActorRemoved(FActor* Actor);

    bool ConsumeCameraCut();

    float GetMoveSpeed() const { return MoveSpeed; }
    float GetRotationSpeed() const { return RotationSpeed; }
    float GetMouseSensitivity() const { return MouseSensitivity; }
    float GetPanSpeed() const { return PanSpeed; }
    float GetFieldOfView() const;
    float GetNearPlane() const;
    float GetFarPlane() const;

    void SetMoveSpeed(float Value);
    void SetRotationSpeed(float Value);
    void SetMouseSensitivity(float Value);
    void SetPanSpeed(float Value);
    void SetFieldOfView(float Value);
    void SetNearPlane(float Value);
    void SetFarPlane(float Value);

    bool IsAttached() const { return AttachedActor != nullptr; }

private:
    void UpdateAttachedCamera();
    void HandleFlyMovement(float DeltaTime, const FEditorCameraInputState& Input);
    void HandleMouse(float DeltaTime, const FEditorCameraInputState& Input);
    void AnchorOrbitPivot();
    void Orbit(const Vector2& Delta);
    void Pan(const Vector2& Delta);
    void Dolly(float Steps);
    void ZoomForward(float Steps);

    void MarkCameraCut()
    {
        bCameraCutPending = true;
    }

    TUniquePtr<FCameraActor> CameraActor;
    FActor*                  AttachedActor;
    Vector3                  AttachLocalOffset;
    Vector3                  AttachRotationOffset;
    Vector3                  Velocity;
    Vector3                  OrbitPivot;
    Vector3                  ResetPosition;
    Vector3                  ResetRotation;
    IntVector2               ViewportSize;
    float                    OrbitDistance;
    float                    MoveSpeed;
    float                    RotationSpeed;
    float                    MouseSensitivity;
    float                    PanSpeed;
    float                    ZoomSpeed;
    float                    DragDollySpeed;
    float                    SpeedAdjustRate;
    bool                     bCameraCutPending;
};
