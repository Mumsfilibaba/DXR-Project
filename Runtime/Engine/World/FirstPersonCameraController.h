#pragma once
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"

class FCameraComponent;

struct FFirstPersonCameraInput
{
    /** Right, up and forward, each in the -1..1 range */
    FVector3 MoveAxis;

    /** Mouse movement since the last frame, in pixels */
    FVector2 LookDelta;

    /** Gamepad look stick, each axis in the -1..1 range, applied per second rather than per pixel */
    FVector2 StickLook;

    bool    bBoost = false;
};

class ENGINE_API FFirstPersonCameraController
{
public:
    static constexpr float MinMoveSpeed     = 0.1f;
    static constexpr float MaxMoveSpeed     = 200.0f;
    static constexpr float MaxPitchDegrees  = 89.0f;

public:
    FFirstPersonCameraController();
    ~FFirstPersonCameraController();

    /**
     * @brief Choose the camera to drive, adopting its current orientation so the view does not jump
     *
     * @param InCamera Camera to drive, or nullptr to drive nothing
     */
    void SetCamera(FCameraComponent* InCamera);

    /**
     * @brief Apply a frame of input to the camera
     *
     * @param DeltaTime Time since the last call in seconds
     * @param Input Look and movement gathered for this frame
     */
    void Tick(float DeltaTime, const FFirstPersonCameraInput& Input);

    /**
     * @brief Adopt the camera's current orientation, use after moving the camera from outside the controller
     */
    void SyncFromCamera();

    /**
     * @brief Drop any accumulated movement, leaving the camera where it stands
     */
    void StopMovement()
    {
        Velocity = FVector3();
    }

    FCameraComponent* GetCamera() const
    {
        return Camera;
    }

    float GetMoveSpeed() const
    {
        return MoveSpeed;
    }

    float GetBoostMultiplier() const
    {
        return BoostMultiplier;
    }

    float GetStickLookSpeed() const
    {
        return StickLookSpeed;
    }

    void SetMoveSpeed(float InMoveSpeed);

    void SetBoostMultiplier(float InBoostMultiplier)
    {
        BoostMultiplier = InBoostMultiplier;
    }

    void SetStickLookSpeed(float InStickLookSpeed)
    {
        StickLookSpeed = InStickLookSpeed;
    }

private:
    void ApplyLook(float DeltaTime, const FFirstPersonCameraInput& Input);
    void ApplyMovement(float DeltaTime, const FFirstPersonCameraInput& Input);

    FCameraComponent* Camera;
    FVector3          Velocity;
    float             Pitch;
    float             Yaw;
    float             MoveSpeed;
    float             BoostMultiplier;
    float             StickLookSpeed;
};
