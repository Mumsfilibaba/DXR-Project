#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix3x4.h"
#include "Core/Math/Matrix4.h"
#include "Core/Time/Timespan.h"
#include "Engine/Core/Object.h"

class FWorld;
class FActorComponent;

class ENGINE_API FActorTransform
{
public:
    FActorTransform();
    ~FActorTransform() = default;

    void SetTranslation(float x, float y, float z);
    void SetTranslation(const Vector3& InPosition);
    void SetScale(float x, float y, float z);
    void SetScale(const Vector3& InScale);

    void SetUniformScale(float InScale)
    {
        SetScale(InScale, InScale, InScale);
    }

    void SetRotation(float x, float y, float z);
    void SetRotation(const Vector3& InRotation);

    const Vector3& GetTranslation() const
    {
        return Translation;
    }

    const Vector3& GetScale() const
    {
        return Scale;
    }

    const Vector3& GetRotation() const
    {
        return Rotation;
    }

    const Matrix4& GetTransformMatrix() const
    {
        return TransformMatrix;
    }

    Matrix4 GetTransformMatrixInverse() const
    {
        Matrix4 MatrixInverse = TransformMatrix.GetInverse();
        return MatrixInverse;
    }

    Matrix3x4 GetTinyMatrix() const
    {
        return Matrix3x4(TransformMatrix);
    }

private:
    void CalculateMatrix();

    Vector3 Translation;
    Vector3 Scale;
    Vector3 Rotation;
    Matrix4 TransformMatrix;
};

class ENGINE_API FActor : public FObject
{
public:
    FOBJECT_DECLARE_CLASS(FActor, FObject);

    /**
     * @brief Construct a new FActor object with the given object initializer.
     *
     * @param ObjectInitializer The object initializer for actor creation.
     */
    FActor(const FObjectInitializer& ObjectInitializer);

    /**
     * @brief Destroy the FActor object and perform any necessary cleanup.
     */
    ~FActor();

    /**
     * @brief Start actor, called in the beginning of the run, perform initialization here
     */
    virtual void Start();

    /**
     * @brief Tick component, should be called once every frame
     *
     * @param DeltaTime Time since the last call to tick in seconds
     */
    virtual void Tick(float DeltaTime);

    /**
     * @brief Add a new component to the actor
     *
     * @param InComponent Component to add to the Actor
     */
    void AddComponent(FActorComponent* InComponent);

    /**
     * @brief Set name of the actor
     *
     * @param InName Name of the actor
     */
    void SetName(const String& InName);

    /**
     * @brief Check if the actor has a component of the component-class
     *
     * @param ComponentClass ClassObject of the component to retrieve
     * @return Returns true if the actor contains a component of a certain type
     */
    bool HasComponentOfClass(class FObjectClass* ComponentClass) const;

    /**
     * @brief Check if the actor has a component of the component-class
     *
     * @return Returns true if the actor contains a component of a certain type
     */
    template<typename ComponentType>
    inline bool HasComponentOfType() const
    {
        return HasComponentOfClass(ComponentType::GetStaticClass());
    }

    /**
     * @brief Retrieve a component from the actor of the component-class
     *
     * @param ComponentClass ClassObject of the component to retrieve
     * @return Returns a pointer to the requested component, or nullptr if no component of the type exists
     */
    FActorComponent* GetComponentOfClass(class FObjectClass* ComponentClass) const;

    /**
     * @brief Retrieve a component from the actor of the component-class
     *
     * @return Returns a pointer to the requested component, or nullptr if no component of the type exists
     */
    template <typename ComponentType>
    inline ComponentType* GetComponentOfType() const
    {
        return static_cast<ComponentType*>(GetComponentOfClass(ComponentType::StaticClass()));
    }

    /**
     * @brief Set the transform of the actor
     *
     * @param InTransform New transform of the actor
     */
    void SetTransform(const FActorTransform& InTransform)
    {
        Transform = InTransform;
    }

    /**
     * @brief Retrieve the name of the actor
     *
     * @return Returns the name of the actor
     */
    const String& GetName() const
    {
        return Name;
    }

    /**
     * @brief Retrieve the World that owns the actor
     *
     * @return Returns the World that owns the actor
     */
    FWorld* GetWorld() const
    {
        return World;
    }

    /**
     * @brief Set the World that owns the actor
     *
     * @param InWorld Pointer to the new World
     */
    void SetWorld(FWorld* InWorld)
    {
        World = InWorld;
    }

    /**
     * @brief Retrieve the transform of the actor
     *
     * @return Returns the transform of the actor
     */
    FActorTransform& GetTransform()
    {
        return Transform;
    }

    /**
     * @brief Retrieve the transform of the actor (const version)
     *
     * @return Returns the transform of the actor
     */
    const FActorTransform& GetTransform() const
    {
        return Transform;
    }

    /**
     * @brief Check if Start should be called on the component
     *
     * @return Returns true if the component's Start method should be called
     */
    bool IsStartable() const
    {
        return bIsStartable;
    }

    /**
     * @brief Check if Tick should be called on the component
     *
     * @return Returns true if the component's Tick method should be called
     */
    bool IsTickable() const
    {
        return bIsTickable;
    }

    /**
     * @brief Set whether the actor is startable
     *
     * @param bInIsStartable New startable flag value
     */
    void SetStartable(bool bInIsStartable)
    {
        bIsStartable = bInIsStartable;
    }

    /**
     * @brief Set whether the actor is tickable
     *
     * @param bInIsTickable New tickable flag value
     */
    void SetTickable(bool bInIsTickable)
    {
        bIsTickable = bInIsTickable;
    }

private:
    String                   Name;
    FWorld*                  World;
    FActorTransform          Transform;
    TArray<FActorComponent*> Components;
    bool                     bIsStartable : 1;
    bool                     bIsTickable  : 1;
};
