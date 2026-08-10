#include "Engine/World/FirstPersonCameraController.h"
#include "Core/Math/Math.h"
#include "Core/Misc/ConsoleManager.h"
#include "Engine/World/Components/CameraComponent.h"

static TAutoConsoleVariable<float> CVarMouseSensitivity(
    "Game.Camera.MouseSensitivity",
    "Degrees the first-person camera turns per pixel of mouse movement",
    0.15f,
    0.001f,
    10.0f,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<bool> CVarInvertLookY(
    "Game.Camera.InvertLookY",
    "Invert the vertical axis of the first-person camera",
    false,
    EConsoleVariableFlags::Default);

constexpr float MOVEMENT_DAMPING_RATE = 5.0f;
constexpr float MOVEMENT_REST_SPEED   = 1.0e-3f;

FFirstPersonCameraController::FFirstPersonCameraController()
    : Camera(nullptr)
    , Velocity()
    , Pitch(0.0f)
    , Yaw(0.0f)
    , MoveSpeed(15.0f)
    , BoostMultiplier(3.0f)
    , StickLookSpeed(45.0f)
{
}

FFirstPersonCameraController::~FFirstPersonCameraController() = default;

void FFirstPersonCameraController::SetCamera(FCameraComponent* InCamera)
{
    if (Camera == InCamera)
    {
        return;
    }

    Camera   = InCamera;
    Velocity = Vector3();

    SyncFromCamera();
}

void FFirstPersonCameraController::SyncFromCamera()
{
    if (!Camera)
    {
        Pitch = 0.0f;
        Yaw   = 0.0f;
        return;
    }

    const Vector3& Rotation = Camera->GetRotation();
    Pitch = Math::RadiansToDegrees(Rotation.X);
    Yaw   = Math::RadiansToDegrees(Rotation.Y);
}

void FFirstPersonCameraController::SetMoveSpeed(float InMoveSpeed)
{
    MoveSpeed = Math::Clamp(InMoveSpeed, MinMoveSpeed, MaxMoveSpeed);
}

void FFirstPersonCameraController::Tick(float DeltaTime, const FFirstPersonCameraInput& Input)
{
    if (!Camera)
    {
        return;
    }

    ApplyLook(DeltaTime, Input);
    ApplyMovement(DeltaTime, Input);
}

void FFirstPersonCameraController::ApplyLook(float DeltaTime, const FFirstPersonCameraInput& Input)
{
    const float Sensitivity = CVarMouseSensitivity.GetValue();
    const float LookSign    = CVarInvertLookY.GetValue() ? -1.0f : 1.0f;

    Yaw   += Input.LookDelta.X * Sensitivity;
    Pitch += Input.LookDelta.Y * Sensitivity * LookSign;

    Yaw   += Input.StickLook.X * StickLookSpeed * DeltaTime;
    Pitch += Input.StickLook.Y * StickLookSpeed * DeltaTime * LookSign;

    Pitch = Math::Clamp(Pitch, -MaxPitchDegrees, MaxPitchDegrees);
    Yaw   = Math::FMod(Yaw, 360.0f);

    Camera->SetRotation(Math::DegreesToRadians(Pitch), Math::DegreesToRadians(Yaw), 0.0f);
}

void FFirstPersonCameraController::ApplyMovement(float DeltaTime, const FFirstPersonCameraInput& Input)
{
    const float   Acceleration       = MoveSpeed * (Input.bBoost ? BoostMultiplier : 1.0f);
    const Vector3 CameraAcceleration = Input.MoveAxis * Acceleration;
    const float   DampingFactor      = Math::Exp(-MOVEMENT_DAMPING_RATE * DeltaTime);

    Velocity = Velocity * DampingFactor;
    Velocity = Velocity + (CameraAcceleration * DeltaTime);

    if ((CameraAcceleration.GetLengthSquared() <= 0.0f) && (Velocity.GetLengthSquared() <= (MOVEMENT_REST_SPEED * MOVEMENT_REST_SPEED)))
    {
        Velocity = Vector3();
    }

    if (Velocity.GetLengthSquared() > 0.0f)
    {
        const Vector3 Movement = Velocity * DeltaTime;
        Camera->AddLocalMovement(Movement.X, Movement.Y, Movement.Z);
    }
}
