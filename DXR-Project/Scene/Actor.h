#pragma once
#include "Core/CoreObject.h"

#include <DirectXMath.h>

/*
* Component Base-Class
*/

class Actor;

class Component : public CoreObject
{
	CORE_OBJECT(Component, CoreObject);

public:
	Component(Actor* InOwningActor);
	virtual ~Component();

	FORCEINLINE Actor* GetOwningActor() const
	{
		return OwningActor;
	}

protected:
	Actor* OwningActor = nullptr;
};

/*
* Transform
*/

class Transform
{
public:
	Transform();
	~Transform() = default;

	void SetTranslation(Float x, Float y, Float z);
	void SetTranslation(const XMFLOAT3& InPosition);

	void SetScale(Float x, Float y, Float z);
	void SetScale(const XMFLOAT3& InScale);

	void SetRotation(Float x, Float y, Float z);
	void SetRotation(const XMFLOAT3& InRotation);

	FORCEINLINE const XMFLOAT3& GetTranslation() const
	{
		return Translation;
	}

	FORCEINLINE const XMFLOAT3& GetScale() const
	{
		return Scale;
	}

	FORCEINLINE const XMFLOAT3& GetRotation() const
	{
		return Rotation;
	}

	FORCEINLINE const XMFLOAT4X4& GetMatrix() const
	{
		return Matrix;
	}

private:
	void CalculateMatrix();

private:
	XMFLOAT4X4	Matrix;
	XMFLOAT3	Translation;
	XMFLOAT3	Scale;
	XMFLOAT3	Rotation;
};

/*
* Actor
*/

class Scene;

class Actor : public CoreObject
{
	CORE_OBJECT(Actor, CoreObject);

public:
	Actor();
	~Actor();

	void AddComponent(Component* InComponent);

	template<typename TComponent>
	FORCEINLINE bool HasComponentOfType() const noexcept
	{
		TComponent* Result = nullptr;
		for (Component* Component : Components)
		{
			if (IsSubClassOf<TComponent>(Component))
			{
				return true;
			}
		}

		return false;
	}

	void OnAddedToScene(Scene* InScene);
	
	void SetDebugName(const std::string& InDebugName);

	FORCEINLINE void SetTransform(const Transform& InTransform)
	{
		Transform = InTransform;
	}

	FORCEINLINE const std::string& GetDebugName() const
	{
		return DebugName;
	}

	FORCEINLINE Scene* GetScene() const
	{
		return CurrentScene;
	}

	FORCEINLINE Transform& GetTransform()
	{
		return Transform;
	}

	FORCEINLINE const Transform& GetTransform() const
	{
		return Transform;
	}

	template <typename TComponent>
	FORCEINLINE TComponent* GetComponentOfType() const
	{
		for (Component* Component : Components)
		{
			if (IsSubClassOf<TComponent>(Component))
			{
				return static_cast<TComponent*>(Component);
			}
		}

		return nullptr;
	}

private:
	Scene* CurrentScene = nullptr;

	Transform Transform;

	TArray<Component*>	Components;
	std::string			DebugName;
};