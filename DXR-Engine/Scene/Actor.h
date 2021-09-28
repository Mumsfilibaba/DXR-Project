#pragma once
#include "Core/CoreObject/CoreObject.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix3x4.h"
#include "Core/Math/Matrix4.h"
#include "Core/Time/Timestamp.h"

#include <string> // Remove later

class CActor;

// CComponent BaseClass
class CComponent : public CCoreObject
{
    CORE_OBJECT( CComponent, CCoreObject );

	CComponent();
	
public:
	
    CComponent( CActor* InOwningActor );
    virtual ~CComponent() = default;

	virtual void Tick( CTimestamp DeltaTime );
	
	FORCEINLINE CActor* GetOwningActor() const
    {
        return OwningActor;
    }
	
	FORCEINLINE bool IsTickable() const
	{
		return Tickable;
	}

protected:
	
	/* The actor that this component belongs to */
    CActor* OwningActor = nullptr;
	
	/* Flags for this component that decides if it should tick or not */
	bool Tickable : 1;
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

class Scene;

class CActor : public CCoreObject
{
    CORE_OBJECT( CActor, CCoreObject );

public:

    CActor();
    ~CActor();

	virtual void Tick( CTimestamp DeltaTime );
	
    void AddComponent( CComponent* InComponent );

    template<typename TComponent>
    FORCEINLINE bool HasComponentOfType() const noexcept
    {
        TComponent* Result = nullptr;
        for ( CComponent* Component : Components )
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
        for ( CComponent* Component : Components )
        {
            if ( IsSubClassOf<TComponent>( Component ) )
            {
                return static_cast<TComponent*>(Component);
            }
        }

        return nullptr;
    }

	FORCEINLINE void OnAddedToScene( Scene* InScene )
    {
        Scene = InScene;
    }

    void SetName( const std::string& InDebugName );

	FORCEINLINE void SetTransform( const Transform& InTransform )
    {
        Transform = InTransform;
    }

	FORCEINLINE const std::string& GetName() const
    {
        return Name;
    }

	FORCEINLINE Scene* GetScene() const
    {
        return Scene;
    }

	FORCEINLINE Transform& GetTransform()
    {
        return Transform;
    }

	FORCEINLINE const Transform& GetTransform() const
    {
        return Transform;
    }

	FORCEINLINE bool IsTickable() const
	{
		return Tickable;
	}
	
private:
	
	/* The scene that this actor belongs to */
    Scene* Scene = nullptr;
	
	/* The transform of this actor */
    Transform Transform;
	
	/* The components of this actor */
    TArray<CComponent*> Components;
	
	/* The name of this actor */
    std::string Name;
	
	/* Flags for this component that decides if it should tick or not */
	bool Tickable : 1;
};
