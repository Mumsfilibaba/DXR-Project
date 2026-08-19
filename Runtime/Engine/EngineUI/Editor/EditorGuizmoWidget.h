#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Set.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "Engine/EngineUI/Editor/EditorGuizmo.h"

class FEditorEngine;
class FActor;

class FEditorGuizmoWidget
{
public:
    FEditorGuizmoWidget(FEditorEngine* InEditorEngine);
    ~FEditorGuizmoWidget();

    void Draw();

    bool IsVisible() const
    {
        return bVisible;
    }
    
    void SetVisible(bool bInVisible)
    {
        bVisible = bInVisible;
    }

private:
    enum class EOperationOverride
    {
        None,
        Translate,
        Rotate,
        TranslateInsteadOfScale,
    };

    Vector3 GetActorGizmoPoint(FActor* Actor, bool bUseBoundsCenter) const;
    
    bool DrawGuizmo();
    void DrawSingleActor(FActor* Actor, const Matrix4& View, const Matrix4& Projection, EditorGuizmo::EOperation::Type Operation, EditorGuizmo::EMode Orientation, bool bUseBoundsCenter);
    bool DrawMultipleActors(const TArray<FActor*>& Actors, const Matrix4& View, const Matrix4& Projection, EditorGuizmo::EOperation::Type Operation, EditorGuizmo::EMode Orientation, bool bUseBoundsCenter);
    void UpdateShortcuts(bool bViewportHovered);
    void CaptureMultiDragState(const TArray<FActor*>& Actors, EditorGuizmo::EMode Orientation, bool bUseBoundsCenter);

    EditorGuizmo::EOperation::Type ResolveOperation(FActor* Actor, EditorGuizmo::EOperation::Type Operation);

    FEditorEngine*     EditorEngine;
    FDelegateHandle    ImGuiEndFrameDelegateHandle;
    Matrix4            GizmoMatrix;
    Matrix4            GizmoStartMatrix;
    TArray<FActor*>    DragActors;
    TArray<Matrix4>    DragStartTransforms;
    TSet<FActor*>      SelectionLookup;
    FActor*            OperationOverrideActor;
    EOperationOverride OperationOverride;
    bool               bVisible;
};

