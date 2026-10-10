#pragma once
#include "Core/Delegates/Delegate.h"
#include "Application/Elements/VisualElement.h"
#include "Application/Gizmo/GizmoMath.h"
#include "Application/Gizmo/GizmoTypes.h"
#include "Application/Text/IFontFace.h"

/** @brief Called for every change a drag makes, with the delta measured from the transform it began on. */
DECLARE_DELEGATE(FOnGizmoTransformChanged, const FMatrix4& /*NewTransform*/, const FMatrix4& /*Delta*/);

/** @brief Called as a handle is grabbed, which is where a host opens its undo transaction. */
DECLARE_DELEGATE(FOnGizmoDragStarted, EGizmoHandle /*Handle*/);

/** @brief Called as a handle is let go, with the transform the drag began and ended on. */
DECLARE_DELEGATE(FOnGizmoDragFinished, const FMatrix4& /*TransformAtDragStart*/, const FMatrix4& /*Transform*/);

class APPLICATION_API FGizmo final : public FVisualElement
{
public:

    /** @brief How much of clip space the gizmo spans, which is what keeps it one size on screen. */
    static constexpr float SizeInClipSpace = 0.1f;

    /** @brief The range Editor.Gizmo.Size is clamped into, so a typo cannot make the gizmo unusable. */
    static constexpr float SizeScaleMin = 0.5f;
    static constexpr float SizeScaleMax = 3.0f;

    /** @brief An axis whose projected length falls below this is pointing at the camera, so it is culled. */
    static constexpr float AxisVisibilityLimit = 0.02f;

    /** @brief A plane handle whose projected area falls below this is edge-on, so it is culled. */
    static constexpr float PlaneVisibilityLimit = 0.0025f;

    /** @brief Where a plane handle starts and ends along each of the two axes it spans. */
    static constexpr float PlaneHandleStart = 0.18f;
    static constexpr float PlaneHandleEnd   = 0.48f;

    /** @brief The half-extent of the handle at the pivot, in pixels, being a radius or a half-side. */
    static constexpr float CenterHandleSize = 9.0f;

    /** @brief The radius of the outer ring, as a multiple of the gizmo's reach. */
    static constexpr float OuterRingScale = 1.15f;

    /** @brief How many segments a rotation ring is drawn and picked as. */
    static constexpr int32 RingSegments = 64;

public:
    struct FDesc
    {
        /** @brief Which handle set the gizmo shows and what a drag does. */
        EGizmoOperation Operation = EGizmoOperation::Translate;

        /** @brief Whether the handles align to the world axes or to the transform being edited. */
        EGizmoMode Mode = EGizmoMode::World;

        /** @brief The increments a drag steps in, each ignored when zero. */
        FGizmoSnapSettings Snap;

        /** @brief The face the drag readout is drawn with, which is left out when there is none. */
        TSharedPtr<IFontFace> Font = nullptr;

        /** @brief Fired for every change a drag makes, with the delta measured from the transform it began on. */
        FOnGizmoTransformChanged OnTransformChanged;

        /** @brief Fired as a handle is grabbed, which is where a host opens its undo transaction. */
        FOnGizmoDragStarted OnDragStarted;

        /** @brief Fired as a handle is let go, with the transform the drag began and ended on. */
        FOnGizmoDragFinished OnDragFinished;
    };

public:
    static TSharedPtr<FGizmo> Create(const FDesc& Desc);

public:
    FGizmo();
    virtual ~FGizmo();

    /**
     * @brief Initializes the gizmo with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual FIntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override;

    /**
     * @brief Sets the camera the gizmo projects through, which it re-reads every frame.
     *
     * @param InViewMatrix       The world-to-view matrix.
     * @param InProjectionMatrix The view-to-clip matrix.
     * @param bInIsOrthographic  True to skip the behind-the-camera hiding rule.
     */
    void SetCamera(const FMatrix4& InViewMatrix, const FMatrix4& InProjectionMatrix, bool bInIsOrthographic);

    /**
     * @brief Sets the transform being edited. Ignored while a drag is in progress.
     *
     * @param InTransform The transform, which the gizmo writes back through its delegate.
     */
    void SetTransform(const FMatrix4& InTransform);

    /**
     * @brief Sets which handles the gizmo shows. Ignored while a drag is in progress.
     *
     * @param InOperation The operation to show.
     */
    void SetOperation(EGizmoOperation InOperation);

    /**
     * @brief Sets whether the handles align to the world axes or to the transform. Ignored mid-drag.
     *
     * @param InMode The mode to use.
     */
    void SetMode(EGizmoMode InMode);

    /**
     * @brief Sets the increments a drag quantises to, each ignored when zero.
     *
     * @param InSnap The increments.
     */
    void SetSnap(const FGizmoSnapSettings& InSnap);

    /**
     * @brief Whether an axis is drawn at all, which it is not when it points at the camera.
     *
     * @param AxisIndex Zero for X, one for Y, two for Z.
     * @return True while the axis is worth showing.
     */
    NODISCARD bool IsAxisVisible(int32 AxisIndex) const;

    /**
     * @brief Whether an axis is drawn the other way, which it is when that end faces the camera.
     *
     * @param AxisIndex Zero for X, one for Y, two for Z.
     * @return True while the axis is flipped.
     */
    NODISCARD bool IsAxisFlipped(int32 AxisIndex) const;

    /**
     * @brief Whether a plane handle is drawn at all, which it is not when the plane is edge-on.
     *
     * @param AxisIndex The axis the plane is normal to.
     * @return True while the handle is worth showing.
     */
    NODISCARD bool IsPlaneVisible(int32 AxisIndex) const;

    /**
     * @brief The direction an axis is drawn along, in world space, flipping included.
     *
     * @param AxisIndex Zero for X, one for Y, two for Z.
     * @return The unit direction.
     */
    NODISCARD FVector3 GetAxisDirection(int32 AxisIndex) const;

    /** @return Where the gizmo sits in world space, the translation of the transform being edited. */
    NODISCARD FVector3 GetPivot() const;

    NODISCARD FORCEINLINE EGizmoOperation           GetOperation() const { return Operation; }
    NODISCARD FORCEINLINE EGizmoMode                GetMode()      const { return Mode; }
    NODISCARD FORCEINLINE const FGizmoSnapSettings& GetSnap()      const { return Snap; }

    /** @return The transform being edited, which a drag in progress has already written into. */
    NODISCARD FORCEINLINE const FMatrix4& GetTransform() const
    {
        return Transform;
    }

    /**
     * @brief Gets whether the cursor is over a handle, so a host can suppress its own picking.
     *
     * @return True while a handle is hovered, and throughout a drag, since the held handle counts as one.
     */
    NODISCARD FORCEINLINE bool IsHovered() const
    {
        return HoveredHandle != EGizmoHandle::None;
    }

    /**
     * @brief Gets whether a handle is being dragged, so a host can suppress its camera controls.
     *
     * @return True from the press on a handle until it is let go.
     */
    NODISCARD FORCEINLINE bool IsDragging() const
    {
        return ActiveHandle != EGizmoHandle::None;
    }

    /** @return The handle under the cursor, None when it is over none, and the held one while dragging. */
    NODISCARD FORCEINLINE EGizmoHandle GetHoveredHandle() const
    {
        return HoveredHandle;
    }

    /** @return The handle being dragged, or None when no drag is in progress. */
    NODISCARD FORCEINLINE EGizmoHandle GetActiveHandle() const
    {
        return ActiveHandle;
    }

    /**
     * @brief Gets how far one axis reaches, which is what holds the gizmo at one size on screen.
     *
     * @return The reach in world units, re-read from the camera whenever the gizmo is arranged.
     */
    NODISCARD FORCEINLINE float GetScreenFactor() const
    {
        return ScreenFactor;
    }

    /** @return True while the pivot projects in front of the camera, and false when nothing is drawn and nothing can be grabbed. */
    NODISCARD FORCEINLINE bool IsProjected() const
    {
        return HasStateBit(StateBit_Projected);
    }

private:
    enum EStateBits : uint16
    {
        StateBit_Orthographic = 1 << 0,
        StateBit_Projected    = 1 << 1,
        StateBit_AxisVisible  = 1 << 2,
        StateBit_AxisFlipped  = 1 << 5,
        StateBit_PlaneVisible = 1 << 8,
    };

    NODISCARD static constexpr uint16 GetAxisBit(EStateBits FirstAxisBit, int32 AxisIndex)
    {
        return static_cast<uint16>(FirstAxisBit << AxisIndex);
    }

    void UpdateContext();
    void ComputeAxisVisibility();

    NODISCARD EGizmoHandle FindHandleAt(const FIntVector2& ClientPosition) const;
    NODISCARD bool GetAxisSegment(int32 AxisIndex, FVector2& OutStart, FVector2& OutEnd) const;
    NODISCARD bool GetPlaneQuad(int32 AxisIndex, FVector2 OutQuad[4]) const;
    NODISCARD bool GetRingPoints(const FVector3& BasisU, const FVector3& BasisV, float Radius, TArray<FVector2>& OutPoints) const;
    NODISCARD float GetAngleOnDragPlane(const FVector3& WorldPosition) const;
    NODISCARD FFloatColor GetHandleColor(EGizmoHandle Handle, int32 AxisIndex) const;    
    NODISCARD bool ProjectOntoDragPlane(const FIntVector2& ClientPosition, FVector3& OutHit) const;

    void GetDragPlane(EGizmoHandle Handle, FVector3& OutPoint, FVector3& OutNormal) const;
    void BeginDrag(EGizmoHandle Handle, const FIntVector2& ClientPosition);
    void UpdateDrag(const FIntVector2& ClientPosition);
    void EndDrag();
    void ApplyTransform(const FMatrix4& NewTransform);
    void DrawTranslate(FDrawCommandList& OutCommandList, int32 LayerId) const;
    void DrawRotate(FDrawCommandList& OutCommandList, int32 LayerId) const;
    void DrawScale(FDrawCommandList& OutCommandList, int32 LayerId) const;
    void DrawCenterHandle(FDrawCommandList& OutCommandList, int32 LayerId, EGizmoHandle Handle, bool bDrawAsSphere) const;
    void DrawReadout(FDrawCommandList& OutCommandList, int32 LayerId) const;

    NODISCARD FORCEINLINE bool HasStateBit(uint16 Bit)                              const { return (StateBits & Bit) != 0; }
    NODISCARD FORCEINLINE bool HasAxisBit(EStateBits FirstAxisBit, int32 AxisIndex) const { return HasStateBit(GetAxisBit(FirstAxisBit, AxisIndex)); }

    FORCEINLINE void SetStateBit(uint16 Bit, bool bValue) { StateBits = static_cast<uint16>(bValue ? (StateBits | Bit) : (StateBits & ~Bit)); }

    FMatrix4                 ViewMatrix;
    FMatrix4                 ProjectionMatrix;
    FMatrix4                 ViewProjectionMatrix;
    FMatrix4                 Transform;
    FMatrix4                 TransformAtDragStart;
    TSharedPtr<IFontFace>    Font;
    EGizmoOperation          Operation;
    EGizmoMode               Mode;
    EGizmoHandle             HoveredHandle;
    EGizmoHandle             ActiveHandle;
    FGizmoSnapSettings       Snap;
    FVector3                 Pivot;
    FVector3                 Axes[3];
    FVector3                 CameraRight;
    FVector3                 CameraUp;
    FVector3                 ViewDirection;
    FVector3                 DragStartHit;
    FVector3                 DragPlanePoint;
    FVector3                 DragPlaneNormal;
    FVector3                 DragAxis;
    FVector3                 DragBasisU;
    FVector3                 DragBasisV;
    float                    DragStartAngle;
    float                    DragLastAngle;
    float                    DragTotalAngle;
    float                    DragStartLength;
    float                    DragStartLengthU;
    float                    DragStartLengthV;
    float                    ScreenFactor;
    uint16                   StateBits;
    FString                  Readout;
    FOnGizmoTransformChanged OnTransformChangedDelegate;
    FOnGizmoDragStarted      OnDragStartedDelegate;
    FOnGizmoDragFinished     OnDragFinishedDelegate;
};
