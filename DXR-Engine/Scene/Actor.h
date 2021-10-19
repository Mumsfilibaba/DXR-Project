#pragma once
#include "Core/CoreObject/CoreObject.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix3x4.h"
#include "Core/Math/Matrix4.h"
#include "Core/Time/Timestamp.h"

class CORE_API CTransform
{
public:

    CTransform();
    ~CTransform() = default;

    void SetTranslation( float x, float y, float z );
    void SetTranslation( const CVector3& InPosition );

    void SetScale( float x, float y, float z );
    void SetScale( const CVector3& InScale );

    FORCEINLINE void SetUniformScale( float InScale )
    {
        SetScale( InScale, InScale, InScale );
    }

    void SetRotation( float x, float y, float z );
    void SetRotation( const CVector3& InRotation );

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

class CORE_API CActor : public CCoreObject
{
    CORE_OBJECT( CActor, CCoreObject );

public:

    CActor( class CScene* InSceneOwner );
    ~CActor();

    /* Start actor, called in the beginning of the run, perform initialization here */
    virtual void Start();

    /* Tick actor, should be called once every frame */
    virtual void Tick( CTimestamp DeltaTime );

    /* Add a new component to the actor */
    void AddComponent( CComponent* InComponent );

    /* Set name of the actor */
    void SetName( const CString& InName );

    /* Check if the actor has a component of the component-class */
    bool HasComponentOfClass( class CClassType* ComponentClass ) const;

    template<typename TComponent>
    inline bool HasComponentOfType() const
    {
        return HasComponentOfClass( TComponent::GetStaticClass() );
    }

    /* Retrieve a component from the actor of the component-class */
    CComponent* GetComponentOfClass( class CClassType* ComponentClass ) const;

    template <typename TComponent>
    inline TComponent* GetComponentOfType() const
    {
        return static_cast<TComponent*>(GetComponentOfClass( TComponent::GetStaticClass() ));
    }

    FORCEINLINE void SetTransform( const CTransform& InTransform )
    {
        Transform = InTransform;
    }

    FORCEINLINE const CString& GetName() const
    {
        return Name;
    }

    FORCEINLINE CScene* GetScene() const
    {
        return SceneOwner;
    }

    FORCEINLINE CTransform& GetTransform()
    {
        return Transform;
    }

    FORCEINLINE const CTransform& GetTransform() const
    {
        return Transform;
    }

    FORCEINLINE bool IsStartable() const
    {
        return Startable;
    }

    FORCEINLINE bool IsTickable() const
    {
        return Tickable;
    }

private:

    /* The name of this actor */
    CString Name;

    /* The scene that this actor belongs to */
    CScene* SceneOwner = nullptr;

    /* The transform of this actor */
    CTransform Transform;

    /* The components of this actor */
    TArray<CComponent*> Components;

    /* Flags for this component that decides if it should start or not */
    bool Startable : 1;

    /* Flags for this component that decides if it should tick or not */
    bool Tickable : 1;
};
