#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix3x4.h"
#include "Core/Math/Matrix4.h"
#include "Core/Time/Timespan.h"
#include "Engine/Core/Object.h"

class ENGINE_API FActorTransform
{
public:
    FActorTransform();
    ~FActorTransform() = default;

    void SetTranslation(float x, float y, float z);
    void SetTranslation(const FVector3& InPosition);

    void SetScale(float x, float y, float z);
    void SetScale(const FVector3& InScale);

    void SetUniformScale(float InScale)
    {
        SetScale(InScale, InScale, InScale);
    }

    void SetRotation(float x, float y, float z);
    void SetRotation(const FVector3& InRotation);

    const FVector3& GetTranslation() const
    {
        return Translation;
    }

    const FVector3& GetScale() const
    {
        return Scale;
    }

    const FVector3& GetRotation() const
    {
        return Rotation;
    }

    const FMatrix4& GetMatrix() const
    {
        return Matrix;
    }

    FMatrix4 GetMatrixInverse() const
    {
        FMatrix4 MatrixInverse = Matrix.Invert();
        return MatrixInverse.Transpose();
    }

    FMatrix3x4 GetTinyMatrix() const
    {
        return FMatrix3x4(
            Matrix.m00, Matrix.m01, Matrix.m02, Matrix.m03,
            Matrix.m10, Matrix.m11, Matrix.m12, Matrix.m13,
            Matrix.m20, Matrix.m21, Matrix.m22, Matrix.m23);
    }

private:
    void CalculateMatrix();

    FMatrix4 Matrix;
    FVector3 Translation;
    FVector3 Scale;
    FVector3 Rotation;
};


class FScene;
class FComponent;

class ENGINE_API FActor : public FObject
{
public:
    FOBJECT_DECLARE_CLASS(FActor, FObject);

    FActor(const FObjectInitializer& ObjectInitializer);
    ~FActor();
    
    /**
     * @brief - Start actor, called in the beginning of the run, perform initialization here
     */
    virtual void Start();

    /**
     * @brief           - Tick component, should be called once every frame
     * @param DeltaTime - Time since the last call to tick
     */
    virtual void Tick(FTimespan DeltaTime);

    /**
     * @brief             - Add a new component to the actor 
     * @param InComponent - Component to add to the Actor
     */
    void AddComponent(FComponent* InComponent);

    /**
     * @brief        - Set name of the actor 
     * @param InName - Name of the actor
     */
    void SetName(const FString& InName);

    /**
     * @brief                - Check if the actor has a component of the component-class 
     * @param ComponentClass - ClassObject to of the component to retrieve 
     * @return               - Returns true if the actor contains a component of a certain type
     */
    bool HasComponentOfClass(class FObjectClass* ComponentClass) const;

    /**
     * @brief  - Check if the actor has a component of the component-class
     * @return - Returns true if the actor contains a component of a certain type
     */
    template<typename ComponentType>
    inline bool HasComponentOfType() const
    {
        return HasComponentOfClass(ComponentType::GetStaticClass());
    }

    /**
     * @brief                - Retrieve a component from the actor of the component-class 
     * @param ComponentClass - ClassObject to of the component to retrieve
     * @return               - Returns a pointer to the requested component, or nullptr if no component of the type exist
     */
    FComponent* GetComponentOfClass(class FObjectClass* ComponentClass) const;

    /**
     * @brief  - Retrieve a component from the actor of the component-class
     * @return - Returns a pointer to the requested component, or nullptr if no component of the type exist
     */
    template <typename ComponentType>
    inline ComponentType* GetComponentOfType() const
    {
        return static_cast<ComponentType*>(GetComponentOfClass(ComponentType::StaticClass()));
    }

    /**
     * @brief             - Set the transform of the actor
     * @param InTransform - New transform of the actor
     */
    void SetTransform(const FActorTransform& InTransform)
    {
        Transform = InTransform;
    }

    /**
     * @brief  - Retrieve the name of the actor
     * @return - Returns the name of the actor
     */
    const FString& GetName() const
    {
        return Name;
    }

    /**
     * @brief  - Retrieve the Scene that owns the actor
     * @return - Returns the Scene that owns the actor
     */
    FScene* GetSceneOwner() const
    {
        return SceneOwner;
    }

    void SetSceneOwner(FScene* InSceneOwner)
    {
        SceneOwner = InSceneOwner;
    }

    /**
     * @brief  - Retrieve the transform of the actor
     * @return - Returns the transform of the actor
     */
    FActorTransform& GetTransform()
    {
        return Transform;
    }

    /**
     * @brief  - Retrieve the transform of the actor
     * @return - Returns the transform of the actor
     */
    const FActorTransform& GetTransform() const
    {
        return Transform;
    }

    /**
     * @brief  - Check if Start should be called on the component
     * @return - Returns true if the component's Start-method should be called
     */
    bool IsStartable() const { return bIsStartable; }

    /**
     * @brief  - Check if Tick should be called on the component
     * @return - Returns true if the component's Tick-method should be called
     */
    bool IsTickable() const { return bIsTickable; }

    void SetStartable(bool bInIsStartable)
    {
        bIsStartable = bInIsStartable;
    }

    void SetTickable(bool bInIsTickable)
    {
        bIsTickable = bInIsTickable;
    }

private:
    FString Name;
    FScene* SceneOwner = nullptr;

    bool bIsStartable : 1;
    bool bIsTickable  : 1;

    FActorTransform     Transform;
    TArray<FComponent*> Components;
};
