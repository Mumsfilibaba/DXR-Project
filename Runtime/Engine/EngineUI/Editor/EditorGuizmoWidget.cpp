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

static void CompensateGizmoPivot(const FActorTransform& OldTransform, const Vector3& GizmoPosition, FActorTransform& InOutTransform)
{
    const Vector3 OldScale = OldTransform.GetScale();
    if (Math::Abs(OldScale.X) < Math::Constants::Epsilon || 
        Math::Abs(OldScale.Y) < Math::Constants::Epsilon || 
        Math::Abs(OldScale.Z) < Math::Constants::Epsilon)
    {
        return;
    }

    const Matrix4 OldBasis = Matrix4::Scale(OldScale)                  * Matrix4::RotationRollPitchYaw(OldTransform.GetRotation());
    const Matrix4 NewBasis = Matrix4::Scale(InOutTransform.GetScale()) * Matrix4::RotationRollPitchYaw(InOutTransform.GetRotation());

    const Vector4 PivotOffset      = Vector4(GizmoPosition - OldTransform.GetTranslation(), 0.0f);
    const Vector4 PivotOffsetLocal = OldBasis.GetInverse().Transform(PivotOffset);
    const Vector4 PivotOffsetWorld = NewBasis.Transform(PivotOffsetLocal);

    InOutTransform.SetTranslation(GizmoPosition - Vector3(PivotOffsetWorld.X, PivotOffsetWorld.Y, PivotOffsetWorld.Z));
}

static bool HasSelectedAncestor(const FEditorEngine* EditorEngine, FActor* Actor)
{
    for (FActor* Parent = Actor->GetParentActor(); Parent; Parent = Parent->GetParentActor())
    {
        if (EditorEngine->IsActorSelected(Parent))
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

    if (ImGui::IsKeyPressed(ImGuiKey_1))
    {
        Viewport->SetGizmoOperation(EditorGuizmo::EOperation::Translate);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_2))
    {
        Viewport->SetGizmoOperation(EditorGuizmo::EOperation::Rotate);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_3))
    {
        Viewport->SetGizmoOperation(EditorGuizmo::EOperation::Scale);
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

    if (!EditorEngine)
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

    // The scene image, not the window content region, since the latter starts at the top of the toolbar
    const ImVec2 ViewportMin  = Viewport->GetViewportImageMin();
    const ImVec2 ViewportSize = Viewport->GetViewportImageSize();
    const ImVec2 ViewportMax  = ImVec2(ViewportMin.x + ViewportSize.x, ViewportMin.y + ViewportSize.y);

    if (ViewportSize.x <= 0.0f || ViewportSize.y <= 0.0f)
    {
        return false;
    }

    EditorGuizmo::SetAlternativeWindow(ViewportWindow);
    EditorGuizmo::BeginFrame();
    EditorGuizmo::SetDrawlist(ViewportWindow->DrawList);

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

    const Matrix4& View       = Camera->GetViewMatrix();
    const Matrix4& Projection = Camera->GetProjectionMatrix();

    if (SelectedActors.Size() == 1)
    {
        DrawSingleActor(SelectedActors[0], View, Projection, Operation, Orientation, bUseBoundsCenter);
        return true;
    }

    return DrawMultipleActors(SelectedActors, View, Projection, Operation, Orientation, bUseBoundsCenter);
}

Vector3 FEditorGuizmoWidget::GetActorGizmoPoint(FActor* Actor, bool bUseBoundsCenter) const
{
    const Matrix4 ActorModel = Actor->GetWorldTransform().GetTransformMatrix();

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

void FEditorGuizmoWidget::DrawSingleActor(FActor* Actor, const Matrix4& View, const Matrix4& Projection, EditorGuizmo::EOperation::Type Operation, EditorGuizmo::EMode Orientation, bool bUseBoundsCenter)
{
    EditorGuizmo::EOperation::Type EffectiveOperation = Operation;

    if (Actor->HasComponentOfType<FPointLightComponent>() || Actor->HasComponentOfType<FLightProbeComponent>())
    {
        EffectiveOperation = EditorGuizmo::EOperation::Translate;
    }
    else if (Actor->HasComponentOfType<FDirectionalLightComponent>())
    {
        EffectiveOperation = EditorGuizmo::EOperation::Rotate;
    }
    else if (Actor->HasComponentOfType<FCameraComponent>() && EffectiveOperation == EditorGuizmo::EOperation::Scale)
    {
        EffectiveOperation = EditorGuizmo::EOperation::Translate;
    }

    const FActorTransform& ActorWorldTransform = Actor->GetWorldTransform();

    // Without bounds this is the actor's own pivot, which makes the translation delta below reduce to the gizmo position
    const Vector3 InitialGizmoPosition = GetActorGizmoPoint(Actor, bUseBoundsCenter);

    Matrix4 Model = ActorWorldTransform.GetTransformMatrix();
    Model.SetTranslation(InitialGizmoPosition);

    const bool bChanged = EditorGuizmo::Manipulate(View, Projection, EffectiveOperation, Orientation, Model);
    if (bChanged || EditorGuizmo::IsUsing())
    {
        Vector3 Translation;
        Vector3 RotationDegrees;
        Vector3 Scale;

        EditorGuizmo::DecomposeMatrixToComponents(Model, Translation, RotationDegrees, Scale);

        // The gizmo always manipulates the actor in world-space, only the component being manipulated is 
        // taken from the gizmo, the remaining ones are carried over from the current world-space transform.
        FActorTransform NewWorldTransform;
        NewWorldTransform.SetTranslation(ActorWorldTransform.GetTranslation());
        NewWorldTransform.SetRotation(ActorWorldTransform.GetRotation());
        NewWorldTransform.SetScale(ActorWorldTransform.GetScale());

        if (EffectiveOperation == EditorGuizmo::EOperation::Translate)
        {
            const Vector3 TranslationDelta = Translation - InitialGizmoPosition;
            NewWorldTransform.SetTranslation(ActorWorldTransform.GetTranslation() + TranslationDelta);

            Actor->SetWorldTransform(NewWorldTransform);
        }
        else if (EffectiveOperation == EditorGuizmo::EOperation::Rotate)
        {
            const Vector3 RotationRadians = Vector3::DegreesToRadians(RotationDegrees);
            if (FCameraComponent* CameraComponent = Actor->GetComponentOfType<FCameraComponent>())
            {
                // The camera clamps and derives its direction vectors from the relative rotation, 
                // so feed it that instead of overwriting the whole transform.
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

bool FEditorGuizmoWidget::DrawMultipleActors(const TArray<FActor*>& Actors, const Matrix4& View, const Matrix4& Projection, EditorGuizmo::EOperation::Type Operation, EditorGuizmo::EMode Orientation, bool bUseBoundsCenter)
{
    if (!EditorGuizmo::IsUsing())
    {
        CaptureMultiDragState(Actors, Orientation, bUseBoundsCenter);
    }

    if (DragActors.IsEmpty())
    {
        return false;
    }

    Matrix4 Model = GizmoMatrix;

    const bool bChanged = EditorGuizmo::Manipulate(View, Projection, Operation, Orientation, Model);
    GizmoMatrix = Model;

    if (!bChanged && !EditorGuizmo::IsUsing())
    {
        return true;
    }

    const Matrix4 Delta = GizmoStartMatrix.GetInverse() * GizmoMatrix;
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

    Vector3 Center     = Vector3(0.0f, 0.0f, 0.0f);
    int32   PointCount = 0;

    for (FActor* Actor : Actors)
    {
        if (!Actor)
        {
            continue;
        }

        Center = Center + GetActorGizmoPoint(Actor, bUseBoundsCenter);
        PointCount++;

        // An actor already moves with its parent, so transforming both would move the child twice
        if (!HasSelectedAncestor(EditorEngine, Actor))
        {
            DragActors.Add(Actor);
            DragStartTransforms.Add(Actor->GetWorldTransform().GetTransformMatrix());
        }
    }

    if (PointCount > 0)
    {
        Center = Center / static_cast<float>(PointCount);
    }

    Vector3 RotationDegrees = Vector3(0.0f, 0.0f, 0.0f);
    if (Orientation == EditorGuizmo::EMode::Local)
    {
        if (FActor* PrimaryActor = EditorEngine->GetSelectedActor())
        {
            RotationDegrees = Vector3::RadiansToDegrees(PrimaryActor->GetWorldTransform().GetRotation());
        }
    }

    // Kept at unit scale, a scaled basis would leak the primary actor's scale into the delta every actor receives
    EditorGuizmo::RecomposeMatrixFromComponents(Center, RotationDegrees, Vector3(1.0f, 1.0f, 1.0f), GizmoMatrix);
    GizmoStartMatrix = GizmoMatrix;
}
