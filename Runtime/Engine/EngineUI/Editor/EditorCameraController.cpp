#include "Core/Math/Math.h"
#include "Engine/EngineUI/Editor/EditorCameraController.h"
#include "Engine/Resources/Model.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Actors/CameraActor.h"
#include "Engine/World/Components/CameraComponent.h"
#include "Engine/World/Components/StaticMeshComponent.h"

FEditorCameraController::FEditorCameraController()
    : CameraActor(nullptr)
    , AttachedActor(nullptr)
    , AttachLocalOffset()
    , AttachRotationOffset()
    , Velocity()
    , OrbitPivot()
    , ResetPosition(0.0f, 0.0f, -2.0f)
    , ResetRotation()
    , ViewportSize(1920, 1080)
    , OrbitDistance(10.0f)
    , MoveSpeed(15.0f)
    , RotationSpeed(45.0f)
    , MouseSensitivity(0.15f)
    , PanSpeed(0.002f)
    , ZoomSpeed(0.15f)
    , DragDollySpeed(0.033f)
    , SpeedAdjustRate(0.05f)
    , bCameraCutPending(true)
{
    CameraActor = NewObject<FCameraActor>();
    CHECK(CameraActor != nullptr);

    if (FCameraComponent* Camera = GetCamera())
    {
        ResetPosition = Camera->GetPosition();
        ResetRotation = Camera->GetRotation();

        Camera->UpdateProjectionMatrix(float(ViewportSize.X), float(ViewportSize.Y));
        Camera->UpdateViewMatrix();
        Camera->UpdateWorldToClipSpaceMatrices();
        
        OrbitPivot = Camera->GetPosition() + Camera->GetForwardVector() * OrbitDistance;
    }
}

FEditorCameraController::~FEditorCameraController() = default;

void FEditorCameraController::Tick(float DeltaTime, const FEditorCameraInputState& Input, FActor* SelectedActor)
{
    FCameraComponent* Camera = GetCamera();
    if (!Camera)
    {
        return;
    }

    if (Input.bResetPressed)
    {
        Reset();
    }
    else if (Input.bFocusPressed && SelectedActor)
    {
        FocusOn(SelectedActor);
    }

    if (AttachedActor)
    {
        UpdateAttachedCamera();
    }
    else
    {
        HandleMouse(DeltaTime, Input);
        HandleFlyMovement(DeltaTime, Input);
    }

    Camera->UpdateViewMatrix();
    Camera->UpdateWorldToClipSpaceMatrices();
}

void FEditorCameraController::UpdateProjection(const IntVector2& InViewportSize)
{
    if (InViewportSize.X <= 0 || InViewportSize.Y <= 0)
    {
        return;
    }

    ViewportSize = InViewportSize;
    if (FCameraComponent* Camera = GetCamera())
    {
        Camera->UpdateProjectionMatrix(float(ViewportSize.X), float(ViewportSize.Y));
    }
}

FCameraComponent* FEditorCameraController::GetCamera() const
{
    return CameraActor ? CameraActor->GetCameraComponent() : nullptr;
}

void FEditorCameraController::Reset()
{
    Detach();
    Velocity = Vector3(0.0f);

    if (FCameraComponent* Camera = GetCamera())
    {
        Camera->SetPosition(ResetPosition);
        Camera->SetRotation(ResetRotation);
        OrbitPivot = ResetPosition + Camera->GetForwardVector() * OrbitDistance;
    }

    MarkCameraCut();
}

void FEditorCameraController::FocusOn(FActor* Actor)
{
    FCameraComponent* Camera = GetCamera();
    if (!Camera || !Actor)
    {
        return;
    }

    Detach();

    const FActorTransform& Transform = Actor->GetTransform();
    Vector3 FocusPoint = Transform.GetTranslation();
    float Radius = 1.0f;

    if (FStaticMeshComponent* MeshComponent = Actor->GetComponentOfType<FStaticMeshComponent>())
    {
        const TSharedPtr<FMesh> Mesh = MeshComponent->GetMesh();
        if (Mesh && Mesh->GetVertexCount() > 0)
        {
            const FAABB& Bounds = Mesh->GetAABB();
            FocusPoint = Transform.GetTransformMatrix().Transform(Bounds.GetCenter());

            const Vector3 BoundsSize = Bounds.GetSize();
            const Vector3 Scale      = Transform.GetScale();
            const float   MaxSize    = Math::Max(BoundsSize.X * Math::Abs(Scale.X), Math::Max(BoundsSize.Y * Math::Abs(Scale.Y), BoundsSize.Z * Math::Abs(Scale.Z)));

            Radius = Math::Max(MaxSize * 0.5f, 0.5f);
        }
    }

    OrbitPivot    = FocusPoint;
    OrbitDistance = Math::Max(Radius * 2.5f, 1.0f);
    Camera->SetPosition(OrbitPivot - Camera->GetForwardVector() * OrbitDistance);

    Velocity = Vector3(0.0f);
    
    MarkCameraCut();
}

void FEditorCameraController::AttachTo(FActor* Actor)
{
    FCameraComponent* Camera = GetCamera();
    if (!Camera || !Actor)
    {
        return;
    }

    AttachedActor = Actor;

    const FActorTransform& Transform = AttachedActor->GetTransform();
    AttachLocalOffset    = Transform.GetTransformMatrixInverse().Transform(Camera->GetPosition());
    AttachRotationOffset = Camera->GetRotation() - Transform.GetRotation();
    Velocity             = Vector3(0.0f);

    MarkCameraCut();
}

void FEditorCameraController::Detach()
{
    if (AttachedActor)
    {
        AttachedActor = nullptr;
        Velocity      = Vector3(0.0f);
        MarkCameraCut();
    }
}

void FEditorCameraController::OnActorRemoved(FActor* Actor)
{
    if (AttachedActor == Actor)
    {
        Detach();
    }
}

bool FEditorCameraController::ConsumeCameraCut()
{
    const bool bWasPending = bCameraCutPending;
    bCameraCutPending = false;
    return bWasPending;
}

float FEditorCameraController::GetFieldOfView() const
{
    const FCameraComponent* Camera = GetCamera();
    return Camera ? Camera->GetFieldOfView() : 90.0f;
}

float FEditorCameraController::GetNearPlane() const
{
    const FCameraComponent* Camera = GetCamera();
    return Camera ? Camera->GetNearPlane() : 0.01f;
}

float FEditorCameraController::GetFarPlane() const
{
    const FCameraComponent* Camera = GetCamera();
    return Camera ? Camera->GetFarPlane() : 200.0f;
}

void FEditorCameraController::SetMoveSpeed(float Value)
{
    MoveSpeed = Math::Clamp(Value, 0.1f, 200.0f);
}

void FEditorCameraController::SetRotationSpeed(float Value)
{
    RotationSpeed = Math::Clamp(Value, 1.0f, 360.0f);
}

void FEditorCameraController::SetMouseSensitivity(float Value)
{
    MouseSensitivity = Math::Clamp(Value, 0.01f, 2.0f);
}

void FEditorCameraController::SetPanSpeed(float Value)
{
    PanSpeed = Math::Clamp(Value, 0.0002f, 0.02f);
}

void FEditorCameraController::SetFieldOfView(float Value)
{
    if (FCameraComponent* Camera = GetCamera())
    {
        Camera->SetFieldOfView(Math::Clamp(Value, 10.0f, 170.0f));
        Camera->UpdateProjectionMatrix(float(ViewportSize.X), float(ViewportSize.Y));

        MarkCameraCut();
    }
}

void FEditorCameraController::SetNearPlane(float Value)
{
    if (FCameraComponent* Camera = GetCamera())
    {
        Camera->SetNearPlane(Math::Clamp(Value, 0.001f, Math::Max(0.001f, Camera->GetFarPlane() - 0.001f)));
        MarkCameraCut();
    }
}

void FEditorCameraController::SetFarPlane(float Value)
{
    if (FCameraComponent* Camera = GetCamera())
    {
        Camera->SetFarPlane(Math::Max(Value, Camera->GetNearPlane() + 0.001f));
        MarkCameraCut();
    }
}

void FEditorCameraController::UpdateAttachedCamera()
{
    FCameraComponent* Camera = GetCamera();
    if (!Camera || !AttachedActor)
    {
        return;
    }

    const FActorTransform& Transform = AttachedActor->GetTransform();
    Camera->SetPosition(Transform.GetTransformMatrix().Transform(AttachLocalOffset));
    Camera->SetRotation(Transform.GetRotation() + AttachRotationOffset);
    OrbitPivot = Transform.GetTranslation();
}

void FEditorCameraController::HandleFlyMovement(float DeltaTime, const FEditorCameraInputState& Input)
{
    FCameraComponent* Camera = GetCamera();
    if (!Camera)
    {
        return;
    }

    if (!Input.bFlyActive)
    {
        Velocity = Vector3(0.0f);
        return;
    }

    const float   Acceleration       = MoveSpeed * (Input.bBoost ? 3.0f : 1.0f);
    const Vector3 CameraAcceleration = Input.MoveAxis * Acceleration;
    const float   DampingFactor      = Math::Exp(-5.0f * DeltaTime);

    Velocity = Velocity * DampingFactor;
    Velocity = Velocity + CameraAcceleration * DeltaTime;

    const Vector3 Movement = Velocity * DeltaTime;
    Camera->AddLocalMovement(Movement.X, Movement.Y, Movement.Z);

    if (Movement.GetLengthSquared() > 0.0f)
    {
        AnchorOrbitPivot();
    }
}

void FEditorCameraController::HandleMouse(float DeltaTime, const FEditorCameraInputState& Input)
{
    FCameraComponent* Camera = GetCamera();
    if (!Camera)
    {
        return;
    }

    const bool bPanChord = Input.bAltDown && (Input.bMiddleMouseDown || (Input.bCmdDown && Input.bLeftMouseDown));
    if (bPanChord)
    {
        Pan(Input.PanDelta);
    }
    else if (Input.bAltDown && Input.bLeftMouseDown)
    {
        Orbit(Input.LookDelta);
    }
    else if (Input.bAltDown && Input.bRightMouseDown)
    {
        Dolly(-Input.LookDelta.Y * DragDollySpeed);
    }
    else if (Input.bRightMouseDown)
    {
        const float Pitch = Math::DegreesToRadians(Input.LookDelta.Y * MouseSensitivity);
        const float Yaw   = Math::DegreesToRadians(Input.LookDelta.X * MouseSensitivity);
        Camera->AddRotation(Pitch, Yaw, 0.0f);

        AnchorOrbitPivot();
    }

    if (Input.RotationAxis.GetLengthSquared() > 0.0f)
    {
        const float Pitch = Math::DegreesToRadians(Input.RotationAxis.Y * RotationSpeed * DeltaTime);
        const float Yaw   = Math::DegreesToRadians(Input.RotationAxis.X * RotationSpeed * DeltaTime);
        Camera->AddRotation(Pitch, Yaw, 0.0f);

        AnchorOrbitPivot();
    }

    if (Input.WheelDelta != 0.0f)
    {
        if (Input.bAltDown)
        {
            SetMoveSpeed(MoveSpeed * Math::Exp(Input.WheelDelta * SpeedAdjustRate));
        }
        else
        {
            ZoomForward(Input.WheelDelta);
        }
    }
}

void FEditorCameraController::AnchorOrbitPivot()
{
    if (FCameraComponent* Camera = GetCamera())
    {
        OrbitPivot = Camera->GetPosition() + Camera->GetForwardVector() * OrbitDistance;
    }
}

void FEditorCameraController::Orbit(const Vector2& Delta)
{
    FCameraComponent* Camera = GetCamera();
    if (!Camera)
    {
        return;
    }

    const float Pitch = Math::DegreesToRadians(Delta.Y * MouseSensitivity);
    const float Yaw   = Math::DegreesToRadians(Delta.X * MouseSensitivity);
    Camera->AddRotation(Pitch, Yaw, 0.0f);
    Camera->SetPosition(OrbitPivot - Camera->GetForwardVector() * OrbitDistance);
    Velocity = Vector3(0.0f);
}

void FEditorCameraController::Pan(const Vector2& Delta)
{
    FCameraComponent* Camera = GetCamera();
    if (!Camera)
    {
        return;
    }

    const float Scale = PanSpeed * Math::Max(OrbitDistance, 1.0f);

    // Both axes drag the scene with the cursor. The X sign looks wrong for that, but RightVector is
    // Forward x Up and so points to the camera's left.
    const Vector3 PreviousPosition = Camera->GetPosition();
    Camera->AddLocalMovement(Delta.X * Scale, Delta.Y * Scale, 0.0f);
    OrbitPivot += Camera->GetPosition() - PreviousPosition;
    Velocity = Vector3(0.0f);
}

void FEditorCameraController::Dolly(float Steps)
{
    FCameraComponent* Camera = GetCamera();
    if (!Camera)
    {
        return;
    }

    OrbitDistance = Math::Clamp(OrbitDistance * Math::Exp(-Steps * ZoomSpeed), 0.1f, 100000.0f);
    Camera->SetPosition(OrbitPivot - Camera->GetForwardVector() * OrbitDistance);
    Velocity = Vector3(0.0f);
}

void FEditorCameraController::ZoomForward(float Steps)
{
    FCameraComponent* Camera = GetCamera();
    if (!Camera)
    {
        return;
    }

    const float Distance = Math::Max(OrbitDistance, 1.0f) * (1.0f - Math::Exp(-Steps * ZoomSpeed));
    Camera->AddLocalMovement(0.0f, 0.0f, Distance);
    AnchorOrbitPivot();
}

