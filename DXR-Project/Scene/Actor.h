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

public:
	Actor* OwningActor = nullptr;
};

class Transform
{
public:
	Transform();
	~Transform() = default;

	void SetPosition(float X, float Y, float Z);
	void SetPosition(const XMFLOAT3& InPosition);

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
};

/*
* Actor
*/

class Actor
{
public:
	Actor();
	~Actor();

	void AddComponent(Component* InComponent);

	void SetDebugName(const std::string& InDebugName);

	FORCEINLINE void SetTransform(const Transform& InTransform)
	{
		Transform = InTransform;
	}

	FORCEINLINE const std::string& GetDebugName() const
	{
		return DebugName;
	}

	FORCEINLINE Transform& GetTransform()
	{
		return Transform;
	}

	FORCEINLINE const Transform& GetTransform() const
	{
		return Transform;
	}

	FORCEINLINE Component* GetComponent() const
	{
		return Components.front();
	}

private:
	Transform Transform;

	std::vector<Component*> Components;
	std::string				DebugName;
};