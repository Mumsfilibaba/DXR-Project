#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix3x4.h"
#include "Core/Math/Matrix4.h"
#include "Core/Time/Timestamp.h"

#include "Engine/CoreObject/CoreObject.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Transform

class ENGINE_API CTransform
{
public:

    CTransform();
    ~CTransform() = default;

    void SetTranslation(float x, float y, float z);
    void SetTranslation(const CVector3& InPosition);

    void SetScale(float x, float y, float z);
    void SetScale(const CVector3& InScale);

    FORCEINLINE void SetUniformScale(float InScale)
    {
        SetScale(InScale, InScale, InScale);
    }

    void SetRotation(float x, float y, float z);
    void SetRotation(const CVector3& InRotation);

    FORCEINLINE const CVector3& GetTranslation() const
    {
        return Translation;
    }

    FORCEINLINE const CVector3& GetScale() const
    {
        return Scale;
    }

    FORCEINLINE const CVector3& GetRotation() const
    {
        return Rotation;
    }

    FORCEINLINE const CMatrix4& GetMatrix() const
    {
        return Matrix;
    }
    FORCEINLINE const CMatrix4& GetMatrixInverse() const
    {
        return MatrixInv;
    }

    FORCEINLINE const CMatrix3x4& GetTinyMatrix() const
    {
        return TinyMatrix;
    }

private:

    void CalculateMatrix();

    CMatrix4 Matrix;
    CMatrix4 MatrixInv;

    CMatrix3x4 TinyMatrix;

    CVector3 Translation;
    CVector3 Scale;
    CVector3 Rotation;
};

class CScene;
class CComponent;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Actor

class ENGINE_API CActor : public CCoreObject
{
    CORE_OBJECT(CActor, CCoreObject);

public:

    CActor(class CScene* InSceneOwner);
    ~CActor();

    /** Start actor, called in the beginning of the run, perform initialization here */
    virtual void Start();

    /**
     * @brief: Tick component, should be called once every frame
     *
     * @param DeltaTime: Time since the last call to tick
     */
    virtual void Tick(CTimestamp DeltaTime);

    /**
     * @brief: Add a new component to the actor 
     * 
     * @param InComponent: Component to add to the Actor
     */
    void AddComponent(CComponent* InComponent);

    /**
     * @brief: Set name of the actor 
     * 
     * @param InName: Name of the actor
     */
    void SetName(const String& InName);

    /**
     * @brief: Check if the actor has a component of the component-class 
     * 
     * @param ComponentClass: ClassObject to of the component to retrieve 
     * @return: Returns true if the actor contains a component of a certain type
     */
    bool HasComponentOfClass(class CClassType* ComponentClass) const;

    /**
     * @brief: Check if the actor has a component of the component-class
     *
     * @return: Returns true if the actor contains a component of a certain type
     */
    template<typename ComponentType>
    inline bool HasComponentOfType() const
    {
        return HasComponentOfClass(ComponentType::GetStaticClass());
    }

    /**
     * @brief: Retrieve a component from the actor of the component-class 
     * 
     * @param ComponentClass: ClassObject to of the component to retrieve
     * @return: Returns a pointer to the requested component, or nullptr if no component of the type exist
     */
    CComponent* GetComponentOfClass(class CClassType* ComponentClass) const;

    /**
     * @brief: Retrieve a component from the actor of the component-class
     *
     * @return: Returns a pointer to the requested component, or nullptr if no component of the type exist
     */
    template <typename ComponentType>
    inline ComponentType* GetComponentOfType() const
    {
        return static_cast<ComponentType*>(GetComponentOfClass(ComponentType::GetStaticClass()));
    }

    /**
     * @brief: Set the transform of the actor
     * 
     * @param InTransform: New transform of the actor
     */
    FORCEINLINE void SetTransform(const CTransform& InTransform)
    {
        Transform = InTransform;
    }

    /**
     * @brief: Retrieve the name of the actor
     * 
     * @return: Returns the name of the actor
     */
    FORCEINLINE const String& GetName() const
    {
        return Name;
    }

    /**
     * @brief: Retrieve the Scene that owns the actor
     *
     * @return: Returns the Scene that owns the actor
     */
    FORCEINLINE CScene* GetScene() const
    {
        return SceneOwner;
    }

    /**
     * @brief: Retrieve the transform of the actor
     *
     * @return: Returns the transform of the actor
     */
    FORCEINLINE CTransform& GetTransform()
    {
        return Transform;
    }

    /**
     * @brief: Retrieve the transform of the actor
     *
     * @return: Returns the transform of the actor
     */
    FORCEINLINE const CTransform& GetTransform() const
    {
        return Transform;
    }

    /**
     * @brief: Check if Start should be called on the component
     *
     * @return: Returns true if the component's Start-method should be called
     */
    FORCEINLINE bool IsStartable() const
    {
        return bIsStartable;
    }

    /**
     * @brief: Check if Tick should be called on the component
     *
     * @return: Returns true if the component's Tick-method should be called
     */
    FORCEINLINE bool IsTickable() const
    {
        return bIsTickable;
    }

private:
    String Name;

    CScene* SceneOwner = nullptr;

    CTransform Transform;

    TArray<CComponent*> Components;

    bool bIsStartable : 1;
    bool bIsTickable : 1;
};
