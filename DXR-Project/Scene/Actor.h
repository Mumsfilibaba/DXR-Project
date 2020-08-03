#pragma once
#include <vector>

#include <DirectXMath.h>

/*
* Component Base-Class
*/

class Actor;

class Component
{
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

class Transform
{
public:
	Transform();
	~Transform() = default;

	void SetPosition(float X, float Y, float Z);
	void SetPosition(const XMFLOAT3& InPosition);
	void SetScale(float X, float Y, float Z);
	void SetScale(const XMFLOAT3& InScale);

	FORCEINLINE const XMFLOAT3& GetPosition() const
	{
		return Position;
	}

	FORCEINLINE const XMFLOAT4X4& GetMatrix() const
	{
		return Matrix;
	}

private:
	void CalculateMatrix();

private:
	XMFLOAT4X4	Matrix;
	XMFLOAT3	Position;
	XMFLOAT3	Scale;
};

/*
* Actor
*/

class Scene;

class Actor
{
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
			Result = dynamic_cast<TComponent*>(Component);
			if (Result)
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
		TComponent* Result = nullptr;
		for (Component* Component : Components)
		{
			Result = dynamic_cast<TComponent*>(Component);
			if (Result)
			{
				return Result;
			}
		}

		return nullptr;
	}

private:
	Scene* CurrentScene = nullptr;

	Transform Transform;

	std::vector<Component*> Components;
	std::string	DebugName;
};