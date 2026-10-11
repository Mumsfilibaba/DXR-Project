#include "Core/Misc/FrameProfiler.h"
#include "Engine/EditorEngine.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/CameraComponent.h"
#include "Engine/World/Components/DirectionalLightComponent.h"
#include "Engine/World/Components/LightProbeComponent.h"
#include "Engine/World/Components/PointLightComponent.h"
#include "Engine/World/Components/StaticMeshComponent.h"
#include "Engine/EngineUI/Editor/EditorGuizmoWidget.h"
#include "Engine/EngineUI/Editor/EditorViewportWidget.h"
#include "ImGuiPlugin/ImGuiCore.h"

static void CompensateGizmoPivot(const FActorTransform& OldTransform, const FVector3& GizmoPosition, FActorTransform& InOutTransform)
{
    const FVector3 OldScale = OldTransform.GetScale();

    if (Math::Abs(OldScale.X) < Math::Constants::Epsilon || 
        Math::Abs(OldScale.Y) < Math::Constants::Epsilon || 
        Math::Abs(OldScale.Z) < Math::Constants::Epsilon)
    {
        return;
    }

    const FMatrix4 OldBasis = FMatrix4::Scale(OldScale)                  * FMatrix4::RotationRollPitchYaw(OldTransform.GetRotation());
    const FMatrix4 NewBasis = FMatrix4::Scale(InOutTransform.GetScale()) * FMatrix4::RotationRollPitchYaw(InOutTransform.GetRotation());

    const FVector4 PivotOffset      = FVector4(GizmoPosition - OldTransform.GetTranslation(), 0.0f);
    const FVector4 PivotOffsetLocal = OldBasis.GetInverse().Transform(PivotOffset);
    const FVector4 PivotOffsetWorld = NewBasis.Transform(PivotOffsetLocal);

    InOutTransform.SetTranslation(GizmoPosition - FVector3(PivotOffsetWorld.X, PivotOffsetWorld.Y, PivotOffsetWorld.Z));
}

static bool HasSelectedAncestor(const TSet<FActor*>& Selection, FActor* Actor)
{
    for (FActor* Parent = Actor->GetParentActor(); Parent; Parent = Parent->GetParentActor())
    {
        if (Selection.Contains(Parent))
        {
            return true;
        }
    }

    return false;
}

FEditorGuizmoWidget::FEditorGuizmoWidget(FEditorEngine* InEditorEngine)
    : EditorEngine(InEditorEngine)
    , ImGuiEndFrameDelegateHandle()
    , GizmoMatrix()
    , GizmoStartMatrix()
    , DragActors()
    , DragStartTransforms()
    , SelectionLookup()
    , OperationOverrideActor(nullptr)
    , OperationOverride(EOperationOverride::None)
    , bVisible(true)
{
    GizmoMatrix.SetIdentity();
    GizmoStartMatrix.SetIdentity();

    if (IImguiPlugin::IsEnabled())
    {
        ImGuiEndFrameDelegateHandle = IImguiPlugin::Get().AddEndFrameDelegate(FImGuiDelegate::CreateRaw(this, &FEditorGuizmoWidget::Draw));
        CHECK(ImGuiEndFrameDelegateHandle.IsValid());

        EditorGuizmo::SetImGuiContext(IImguiPlugin::Get().GetImGuiContext());
    }
}

FEditorGuizmoWidget::~FEditorGuizmoWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        EditorGuizmo::SetImGuiContext(nullptr);

        IImguiPlugin::Get().RemoveEndFrameDelegate(ImGuiEndFrameDelegateHandle);
    }
}

void FEditorGuizmoWidget::UpdateShortcuts(bool bViewportHovered)
{
    if (!bViewportHovered)
    {
        return;
    }

    if (!EditorEngine || !EditorEngine->IsEditing())
    {
        return;
    }

    const ImGuiIO& IO = ImGui::GetIO();
    if (IO.WantTextInput)
    {
        return;
    }

    const TSharedPtr<FEditorViewportWidget>& Viewport = EditorEngine->GetEditorViewportWidget();
    if (!Viewport)
    {
        return;
    }

    const bool bFlyActive = ImGui::IsMouseDown(ImGuiMouseButton_Right) && !IO.KeyAlt;
    if (bFlyActive)
    {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_W) || ImGui::IsKeyPressed(ImGuiKey_1))
    {
        Viewport->SetGizmoOperation(EditorGuizmo::EOperation::Translate);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_E) || ImGui::IsKeyPressed(ImGuiKey_2))
    {
        Viewport->SetGizmoOperation(EditorGuizmo::EOperation::Rotate);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_R) || ImGui::IsKeyPressed(ImGuiKey_3))
    {
        Viewport->SetGizmoOperation(EditorGuizmo::EOperation::Scale);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_Space))
    {
        const EditorGuizmo::EOperation::Type CurrentOperation = Viewport->GetGizmoOperation();
        if (CurrentOperation == EditorGuizmo::EOperation::Translate)
        {
            Viewport->SetGizmoOperation(EditorGuizmo::EOperation::Rotate);
        }
        else if (CurrentOperation == EditorGuizmo::EOperation::Rotate)
        {
            Viewport->SetGizmoOperation(EditorGuizmo::EOperation::Scale);
        }
        else
        {
            Viewport->SetGizmoOperation(EditorGuizmo::EOperation::Translate);
        }
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_4))
    {
        const EditorGuizmo::EMode CurrentOrientation = Viewport->GetGizmoOrientation();
        Viewport->SetGizmoOrientation(CurrentOrientation == EditorGuizmo::EMode::Local ? EditorGuizmo::EMode::World : EditorGuizmo::EMode::Local);
    }
}

void FEditorGuizmoWidget::Draw()
{
    if (!DrawGuizmo())
    {
        EditorGuizmo::CancelUsing();
    }
}

bool FEditorGuizmoWidget::DrawGuizmo()
{
    if (!bVisible)
    {
        return false;
    }

    TRACE_SCOPE(TEXT("Gizmo"));

    if (!EditorEngine || !EditorEngine->IsEditing())
    {
        return false;
    }

    const TSharedPtr<FEditorViewportWidget>& Viewport = EditorEngine->GetEditorViewportWidget();
    if (!Viewport)
    {
        return false;
    }

    if (Viewport->GetDebugView() != FSceneRenderView::EDebugView::None)
    {
        return false;
    }

    FCameraComponent* Camera = EditorEngine->GetActiveViewportCamera();
    if (!Camera)
    {
        return false;
    }

    ImGuiWindow* ViewportWindow = ImGui::FindWindowByName("Viewport");
    if (!ViewportWindow)
    {
        return false;
    }

    const ImVec2 ViewportMin  = Viewport->GetViewportImageMin();
    const ImVec2 ViewportSize = Viewport->GetViewportImageSize();
    const ImVec2 ViewportMax  = ImVec2(ViewportMin.x + ViewportSize.x, ViewportMin.y + ViewportSize.y);

    if (ViewportSize.x <= 0.0f || ViewportSize.y <= 0.0f)
    {
        return false;
    }

    ImDrawList* ImageDrawList = Viewport->GetViewportImageDrawList();
    if (!ImageDrawList)
    {
        return false;
    }

    EditorGuizmo::SetAlternativeWindow(ViewportWindow);

    EditorGuizmo::BeginFrame(ImageDrawList);

    const ImVec2 MousePos = ImGui::GetIO().MousePos;

    const bool bViewportHovered =
        MousePos.x >= ViewportMin.x &&
        MousePos.y >= ViewportMin.y &&
        MousePos.x <= ViewportMax.x &&
        MousePos.y <= ViewportMax.y;

    UpdateShortcuts(bViewportHovered);

    const TArray<FActor*>& SelectedActors = EditorEngine->GetSelectedActors();
    if (SelectedActors.IsEmpty())
    {
        return false;
    }

    EditorGuizmo::SetRect(ViewportMin.x, ViewportMin.y, ViewportSize.x, ViewportSize.y);
    EditorGuizmo::SetOrthographic(false);

    const EditorGuizmo::EOperation::Type Operation   = Viewport->GetGizmoOperation();
    const EditorGuizmo::EMode            Orientation = Viewport->GetGizmoOrientation();

    const bool bUseBoundsCenter = Viewport->GetGizmoPlacement() == FEditorViewportWidget::EGizmoPlacement::Center;

    const FMatrix4& View       = Camera->GetViewMatrix();
    const FMatrix4& Projection = Camera->GetProjectionMatrix();

    if (SelectedActors.Size() == 1)
    {
        DrawSingleActor(SelectedActors[0], View, Projection, Operation, Orientation, bUseBoundsCenter);
        return true;
    }

    return DrawMultipleActors(SelectedActors, View, Projection, Operation, Orientation, bUseBoundsCenter);
}

FVector3 FEditorGuizmoWidget::GetActorGizmoPoint(FActor* Actor, bool bUseBoundsCenter) const
{
    const FMatrix4 ActorModel = Actor->GetWorldTransform().GetTransformMatrix();

    if (bUseBoundsCenter)
    {
        if (FStaticMeshComponent* MeshComponent = Actor->GetComponentOfType<FStaticMeshComponent>())
        {
            const TSharedPtr<FMesh> Mesh = MeshComponent->GetMesh();
            if (Mesh && Mesh->GetVertexCount() > 0)
            {
                return ActorModel.Transform(Mesh->GetAABB().GetCenter());
            }
        }
    }

    return ActorModel.GetTranslation();
}

EditorGuizmo::EOperation::Type FEditorGuizmoWidget::ResolveOperation(FActor* Actor, EditorGuizmo::EOperation::Type Operation)
{
    if (Actor != OperationOverrideActor)
    {
        OperationOverrideActor = Actor;
        OperationOverride      = EOperationOverride::None;

        if (Actor->HasComponentOfType<FPointLightComponent>() || Actor->HasComponentOfType<FLightProbeComponent>())
        {
            OperationOverride = EOperationOverride::Translate;
        }
        else if (Actor->HasComponentOfType<FDirectionalLightComponent>())
        {
            OperationOverride = EOperationOverride::Rotate;
        }
        else if (Actor->HasComponentOfType<FCameraComponent>())
        {
            OperationOverride = EOperationOverride::TranslateInsteadOfScale;
        }
    }

    switch (OperationOverride)
    {
        case EOperationOverride::Translate:
            return EditorGuizmo::EOperation::Translate;

        case EOperationOverride::Rotate:
            return EditorGuizmo::EOperation::Rotate;

        case EOperationOverride::TranslateInsteadOfScale:
            return (Operation == EditorGuizmo::EOperation::Scale) ? EditorGuizmo::EOperation::Translate : Operation;

        default:
            return Operation;
    }
}

void FEditorGuizmoWidget::DrawSingleActor(FActor* Actor, const FMatrix4& View, const FMatrix4& Projection, EditorGuizmo::EOperation::Type Operation, EditorGuizmo::EMode Orientation, bool bUseBoundsCenter)
{
    const EditorGuizmo::EOperation::Type EffectiveOperation = ResolveOperation(Actor, Operation);

    const FActorTransform& ActorWorldTransform = Actor->GetWorldTransform();
    const FVector3         InitialGizmoPosition = GetActorGizmoPoint(Actor, bUseBoundsCenter);

    FMatrix4 Model = ActorWorldTransform.GetTransformMatrix();
    Model.SetTranslation(InitialGizmoPosition);

    const bool bChanged = EditorGuizmo::Manipulate(View, Projection, EffectiveOperation, Orientation, Model);
    if (bChanged || EditorGuizmo::IsUsing())
    {
        FVector3 Translation;
        FVector3 RotationDegrees;
        FVector3 Scale;

        EditorGuizmo::DecomposeMatrixToComponents(Model, Translation, RotationDegrees, Scale);

        FActorTransform NewWorldTransform;
        NewWorldTransform.SetTranslation(ActorWorldTransform.GetTranslation());
        NewWorldTransform.SetRotation(ActorWorldTransform.GetRotation());
        NewWorldTransform.SetScale(ActorWorldTransform.GetScale());

        if (EffectiveOperation == EditorGuizmo::EOperation::Translate)
        {
            const FVector3 TranslationDelta = Translation - InitialGizmoPosition;
            NewWorldTransform.SetTranslation(ActorWorldTransform.GetTranslation() + TranslationDelta);

            Actor->SetWorldTransform(NewWorldTransform);
        }
        else if (EffectiveOperation == EditorGuizmo::EOperation::Rotate)
        {
            const FVector3 RotationRadians = FVector3::DegreesToRadians(RotationDegrees);
            if (FCameraComponent* CameraComponent = Actor->GetComponentOfType<FCameraComponent>())
            {
                NewWorldTransform.SetRotation(RotationRadians);
                CameraComponent->SetRotation(Actor->ConvertWorldToRelativeTransform(NewWorldTransform).GetRotation());
            }
            else
            {
                NewWorldTransform.SetRotation(RotationRadians);
                CompensateGizmoPivot(ActorWorldTransform, InitialGizmoPosition, NewWorldTransform);

                Actor->SetWorldTransform(NewWorldTransform);
            }
        }
        else if (EffectiveOperation == EditorGuizmo::EOperation::Scale)
        {
            NewWorldTransform.SetScale(Scale);
            CompensateGizmoPivot(ActorWorldTransform, InitialGizmoPosition, NewWorldTransform);

            Actor->SetWorldTransform(NewWorldTransform);
        }
    }
}

bool FEditorGuizmoWidget::DrawMultipleActors(const TArray<FActor*>& Actors, const FMatrix4& View, const FMatrix4& Projection, EditorGuizmo::EOperation::Type Operation, EditorGuizmo::EMode Orientation, bool bUseBoundsCenter)
{
    if (!EditorGuizmo::IsUsing())
    {
        CaptureMultiDragState(Actors, Orientation, bUseBoundsCenter);
    }

    if (DragActors.IsEmpty())
    {
        return false;
    }

    FMatrix4 Model = GizmoMatrix;

    const bool bChanged = EditorGuizmo::Manipulate(View, Projection, Operation, Orientation, Model);
    GizmoMatrix = Model;

    if (!bChanged && !EditorGuizmo::IsUsing())
    {
        return true;
    }

    const FMatrix4 Delta = GizmoStartMatrix.GetInverse() * GizmoMatrix;
    for (int32 Index = 0; Index < DragActors.Size(); Index++)
    {
        FActor* Actor = DragActors[Index];
        if (!Actor || !EditorEngine->IsActorSelected(Actor))
        {
            continue;
        }

        Actor->SetWorldTransformMatrix(DragStartTransforms[Index] * Delta);

        if (FCameraComponent* CameraComponent = Actor->GetComponentOfType<FCameraComponent>())
        {
            CameraComponent->SetRotation(Actor->GetTransform().GetRotation());
        }
    }

    return true;
}

void FEditorGuizmoWidget::CaptureMultiDragState(const TArray<FActor*>& Actors, EditorGuizmo::EMode Orientation, bool bUseBoundsCenter)
{
    DragActors.Clear();
    DragStartTransforms.Clear();
    DragActors.Reserve(Actors.Size());
    DragStartTransforms.Reserve(Actors.Size());

    SelectionLookup.Clear();

    for (FActor* Actor : Actors)
    {
        if (Actor)
        {
            SelectionLookup.Add(Actor);
        }
    }

    FVector3 Center     = FVector3(0.0f, 0.0f, 0.0f);
    int32    PointCount = 0;

    for (FActor* Actor : Actors)
    {
        if (!Actor)
        {
            continue;
        }

        Center = Center + GetActorGizmoPoint(Actor, bUseBoundsCenter);
        PointCount++;

        if (!HasSelectedAncestor(SelectionLookup, Actor))
        {
            DragActors.Add(Actor);
            DragStartTransforms.Add(Actor->GetWorldTransform().GetTransformMatrix());
        }
    }

    if (PointCount > 0)
    {
        Center = Center / static_cast<float>(PointCount);
    }

    FVector3 RotationDegrees = FVector3(0.0f, 0.0f, 0.0f);
    if (Orientation == EditorGuizmo::EMode::Local)
    {
        if (FActor* PrimaryActor = EditorEngine->GetSelectedActor())
        {
            RotationDegrees = FVector3::RadiansToDegrees(PrimaryActor->GetWorldTransform().GetRotation());
        }
    }

    EditorGuizmo::RecomposeMatrixFromComponents(Center, RotationDegrees, FVector3(1.0f, 1.0f, 1.0f), GizmoMatrix);
    GizmoStartMatrix = GizmoMatrix;
}
