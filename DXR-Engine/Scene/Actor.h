#pragma once
#include "Core/CoreObject/CoreObject.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix3x4.h"
#include "Core/Math/Matrix4.h"

#include <string> // Remove later

class Actor;

// Component BaseClass
class Component : public CoreObject
{
    CORE_OBJECT( Component, CoreObject );

public:
    Component( Actor* InOwningActor );
    virtual ~Component() = default;

    Actor* GetOwningActor() const
    {
        return OwningActor;
    }

protected:
    Actor* OwningActor = nullptr;
};

class Transform
{
public:
    Transform();
    ~Transform() = default;

    void SetTranslation( float x, float y, float z );
    void SetTranslation( const CVector3& InPosition );

    void SetScale( float x, float y, float z );
    void SetScale( const CVector3& InScale );

    void SetUniformScale( float InScale )
    {
        SetScale( InScale, InScale, InScale );
    }

    void SetRotation( float x, float y, float z );
    void SetRotation( const CVector3& InRotation );

    const CVector3& GetTranslation() const
    {
        return Translation;
    }

    const CVector3& GetScale() const
    {
        return Scale;
    }

    const CVector3& GetRotation() const
    {
        return Rotation;
    }

    const CMatrix4& GetMatrix() const
    {
        return Matrix;
    }
    const CMatrix4& GetMatrixInverse() const
    {
        return MatrixInv;
    }

    const CMatrix3x4& GetTinyMatrix() const
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

class Scene;

class Actor : public CoreObject
{
    CORE_OBJECT( Actor, CoreObject );

public:
    Actor();
    ~Actor();

    void AddComponent( Component* InComponent );

    template<typename TComponent>
    FORCEINLINE bool HasComponentOfType() const noexcept
    {
        TComponent* Result = nullptr;
        for ( Component* Component : Components )
        {
            if ( IsSubClassOf<TComponent>( Component ) )
            {
                return true;
            }
        }

        return false;
    }

    template <typename TComponent>
    FORCEINLINE TComponent* GetComponentOfType() const
    {
        for ( Component* Component : Components )
        {
            if ( IsSubClassOf<TComponent>( Component ) )
            {
                return static_cast<TComponent*>(Component);
            }
        }

        return nullptr;
    }

    void OnAddedToScene( Scene* InScene )
    {
        Scene = InScene;
    }

    void SetName( const std::string& InDebugName );

    void SetTransform( const Transform& InTransform )
    {
        Transform = InTransform;
    }

    const std::string& GetName() const
    {
        return Name;
    }

    Scene* GetScene() const
    {
        return Scene;
    }

    Transform& GetTransform()
    {
        return Transform;
    }

    const Transform& GetTransform() const
    {
        return Transform;
    }

private:
    Scene* Scene = nullptr;
    Transform          Transform;
    TArray<Component*> Components;
    std::string        Name;
};