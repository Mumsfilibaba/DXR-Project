#pragma once
#include "Core/CoreObject.h"

#include <Containers/Array.h>

class Actor;

// Component BaseClass
class Component : public CoreObject
{
    CORE_OBJECT(Component, CoreObject);

public:
    Component(Actor* InOwningActor);
    virtual ~Component() = default;

    Actor* GetOwningActor() const { return OwningActor; }

protected:
    Actor* OwningActor = nullptr;
};

class Transform
{
public:
    Transform();
    ~Transform() = default;

    void SetTranslation(Float x, Float y, Float z);
    void SetTranslation(const XMFLOAT3& InPosition);

    void SetScale(Float x, Float y, Float z);
    void SetScale(const XMFLOAT3& InScale);

    void SetUniformScale(Float InScale)
    {
        SetScale(InScale, InScale, InScale);
    }

    void SetRotation(Float x, Float y, Float z);
    void SetRotation(const XMFLOAT3& InRotation);

    const XMFLOAT3& GetTranslation() const { return Translation; }

    const XMFLOAT3& GetScale() const { return Scale; }

    const XMFLOAT3& GetRotation() const { return Rotation; }

    const XMFLOAT4X4& GetMatrix() const { return Matrix; }
    const XMFLOAT4X4& GetMatrixInverse() const { return MatrixInv; }

private:
    void CalculateMatrix();

    XMFLOAT4X4 Matrix;
    XMFLOAT4X4 MatrixInv;
    XMFLOAT3   Translation;
    XMFLOAT3   Scale;
    XMFLOAT3   Rotation;
};

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

    void OnAddedToScene(Scene* InScene)
    {
        Scene = InScene;
    }
    
    void SetName(const std::string& InDebugName);

    void SetTransform(const Transform& InTransform)
    {
        Transform = InTransform;
    }

    const std::string& GetName() const { return Name; }

    Scene* GetScene() const { return Scene; }

    Transform& GetTransform() { return Transform; }
    const Transform& GetTransform() const { return Transform; }

private:
    Scene*    Scene = nullptr;
    Transform Transform;
    TArray<Component*> Components;
    std::string        Name;
};