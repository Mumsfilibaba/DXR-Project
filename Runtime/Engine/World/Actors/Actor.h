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
class FActorFilter;

enum class EAttachmentRule : uint8
{
    KeepWorld,    // Preserve the world-space transform, the relative transform is recalculated against the new parent
    KeepRelative, // Preserve the relative transform, which means that the actor moves along with the new parent
};

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

    /**
     * @brief Set the transform from a transformation-matrix
     *
     * The matrix is decomposed into translation, rotation and scale, but is also stored as-is, which means that
     * any shear introduced by a chain of rotated and non-uniformly scaled transforms is preserved in the matrix.
     *
     * @param InMatrix Transformation-matrix to assign to the transform
     */
    void SetFromMatrix(const Matrix4& InMatrix);

    /**
     * @brief Retrieve a counter that is incremented every time the transform is modified
     *
     * @return Returns the current version of the transform
     */
    uint64 GetVersion() const
    {
        return Version;
    }

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
    uint64  Version;
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
     * @brief Initializes the actor immediately after construction and before world or renderer registration
     */
    virtual void Initialize();

    /**
     * @brief Start actor, called in the beginning of the run, perform initialization here
     */
    virtual void Start();

    /**
     * @brief End actor, called when the run stops, release anything Start acquired here
     *
     * The actor itself outlives the run, so this has to leave it in a state a later Start can pick up again.
     */
    virtual void EndPlay();

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
     * @brief Remove and delete a component owned by the actor
     *
     * @param InComponent Component to remove from the Actor
     */
    void RemoveComponent(FActorComponent* InComponent);

    /**
     * @brief Attach this actor to another actor, making this actor a child of the specified parent
     *
     * The attachment is rejected if the new parent is the actor itself, if the new parent is a descendant of this
     * actor, or if the actors belong to different worlds.
     *
     * @param NewParent Actor to attach to, or nullptr to detach the actor from its current parent
     * @param Rule Determines if the world-space or the relative transform is preserved by the attachment
     * @return Returns true if the actor was attached to the new parent
     */
    bool AttachToActor(FActor* NewParent, EAttachmentRule Rule = EAttachmentRule::KeepWorld);

    /**
     * @brief Detach the actor from its current parent, turning it back into a root-actor
     *
     * @param Rule Determines if the world-space or the relative transform is preserved by the detachment
     */
    void DetachFromParent(EAttachmentRule Rule = EAttachmentRule::KeepWorld);

    /**
     * @brief Detach all children from this actor, turning each of them into a root-actor
     *
     * @param Rule Determines if the world-space or the relative transform is preserved by the detachment
     */
    void DetachAllChildren(EAttachmentRule Rule = EAttachmentRule::KeepWorld);

    /**
     * @brief Check if the actor is attached to another actor, either directly or through one of its ancestors
     *
     * @param PossibleParent Actor to look for among the ancestors of this actor
     * @return Returns true if the actor is a descendant of the specified actor
     */
    bool IsAttachedTo(const FActor* PossibleParent) const;

    /**
     * @brief Retrieve the world-space transform of the actor
     *
     * For root-actors this is the same as the relative transform, otherwise it is the relative transform composed
     * with the world-space transform of the parent.
     *
     * @return Returns the world-space transform of the actor
     */
    const FActorTransform& GetWorldTransform() const;

    /**
     * @brief Set the world-space transform of the actor, the relative transform is recalculated from the parent
     *
     * @param InTransform New world-space transform of the actor
     */
    void SetWorldTransform(const FActorTransform& InTransform);

    /**
     * @brief Set the world-space transform of the actor from a transformation-matrix
     *
     * @param InMatrix New world-space transformation-matrix of the actor
     */
    void SetWorldTransformMatrix(const Matrix4& InMatrix);

    /**
     * @brief Convert a world-space transformation-matrix into a matrix relative to the parent of this actor
     *
     * For root-actors the matrix is returned unchanged, since relative and world-space are the same.
     *
     * @param InWorldMatrix World-space transformation-matrix to convert
     * @return Returns the matrix expressed relative to the parent of the actor
     */
    Matrix4 ConvertWorldToRelativeMatrix(const Matrix4& InWorldMatrix) const;

    /**
     * @brief Convert a world-space transform into a transform relative to the parent of this actor
     *
     * @param InWorldTransform World-space transform to convert
     * @return Returns the transform expressed relative to the parent of the actor
     */
    FActorTransform ConvertWorldToRelativeTransform(const FActorTransform& InWorldTransform) const;

    /**
     * @brief Retrieve the actor that this actor is attached to
     *
     * @return Returns the parent of the actor, or nullptr if the actor is a root-actor
     */
    FActor* GetParentActor() const
    {
        return ParentActor;
    }

    /**
     * @return Returns all actors attached to this actor
     */
    const TArray<FActor*>& GetChildActors() const
    {
        return ChildActors;
    }

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
        return static_cast<ComponentType*>(GetComponentOfClass(ComponentType::GetStaticClass()));
    }

    /**
     * @return Returns all components owned by the actor
     */
    const TArray<FActorComponent*>& GetComponents() const
    {
        return Components;
    }

    /**
     * @brief Set the transform of the actor, relative to its parent
     *
     * @param InTransform New relative transform of the actor
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
     * @brief Retrieve a label describing the type of the actor, used to present the actor in editor tooling
     *
     * @return Returns the type-label of the actor
     */
    virtual const CHAR* GetTypeLabel() const
    {
        return "Actor";
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
     * @brief Place the actor in a filter, which decides where it is shown in the scene-hierarchy
     *
     * Filters only exist to organise the editor's scene-hierarchy, so outside an editor build this does nothing
     * and the actor carries no filter at all. Scene-setup code can keep calling it unconditionally.
     *
     * @param InFilter Filter to place the actor in, or nullptr to move it to the root of the hierarchy
     */
    void SetFilter(FActorFilter* InFilter)
    {
    #if EDITOR_BUILD
        Filter = InFilter;
    #else
        UNREFERENCED_VARIABLE(InFilter);
    #endif
    }

    /**
     * @brief Retrieve the filter the actor was placed in
     *
     * @return Returns the filter the actor was placed in, or nullptr when it sits at the root or outside an editor build
     */
    FActorFilter* GetFilter() const
    {
    #if EDITOR_BUILD
        return Filter;
    #else
        return nullptr;
    #endif
    }

    /**
     * @brief Retrieve the transform of the actor, relative to its parent
     *
     * For root-actors this is the same as the world-space transform, use GetWorldTransform to retrieve the
     * world-space transform of an actor that may be attached to a parent.
     *
     * @return Returns the relative transform of the actor
     */
    FActorTransform& GetTransform()
    {
        return Transform;
    }

    /**
     * @brief Retrieve the transform of the actor, relative to its parent (const version)
     *
     * @return Returns the relative transform of the actor
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
     * @brief Check if the actor keeps ticking while the world is being authored rather than run
     *
     * Game logic is held back in the editor, so an actor that has to keep updating outside a run, such as an
     * editor-owned preview or visualization, opts in here.
     *
     * @return Returns true if the actor should tick outside a run
     */
    bool TicksInEditor() const
    {
        return bTickInEditor;
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

    /**
     * @brief Set whether the actor ticks while the world is being authored rather than run
     *
     * @param bInTickInEditor New editor-tick flag value
     */
    void SetTickInEditor(bool bInTickInEditor)
    {
        bTickInEditor = bInTickInEditor;
    }

private:

    friend class FWorld;

    void UnlinkFromParent();
    void ClearAttachments();
    void InvalidateWorldTransformCache() const;

    String                   Name;
    FWorld*                  World;
#if EDITOR_BUILD
    FActorFilter*            Filter;
#endif
    FActorTransform          Transform;
    TArray<FActorComponent*> Components;
    FActor*                  ParentActor;
    TArray<FActor*>          ChildActors;
    mutable FActorTransform  CachedWorldTransform;
    mutable uint64           CachedLocalVersion;
    mutable uint64           CachedParentVersion;
    bool                     bIsStartable  : 1;
    bool                     bIsTickable   : 1;
    bool                     bTickInEditor : 1;
};
