#include "Engine/EditorEngine.h"
#include "Engine/World/World.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/CameraComponent.h"
#include "Engine/World/Components/DirectionalLightComponent.h"
#include "Engine/World/Components/LightProbeComponent.h"
#include "Engine/World/Components/PointLightComponent.h"
#include "Engine/World/Components/StaticMeshComponent.h"
#include "Engine/EngineUI/Editor/EditorGuizmoWidget.h"
#include "Engine/EngineUI/Editor/EditorViewportWidget.h"
#include "ImGuiPlugin/ImGuiCore.h"

FEditorGuizmoWidget::FEditorGuizmoWidget(FEditorEngine* InEditorEngine)
    : EditorEngine(InEditorEngine)
    , ImGuiEndFrameDelegateHandle()
    , bVisible(true)
{
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
    if (!bVisible)
    {
        return;
    }

    if (!EditorEngine)
    {
        return;
    }

    const TSharedPtr<FEditorViewportWidget>& Viewport = EditorEngine->GetEditorViewportWidget();
    if (Viewport)
    {
        if (Viewport->GetDebugView() != FSceneRenderView::EDebugView::None)
        {
            return;
        }
    }

    FWorld* World = EditorEngine->GetWorld();
    if (!World)
    {
        return;
    }

    FCameraComponent* Camera = World->GetActiveCamera();
    if (!Camera)
    {
        return;
    }

    ImGuiWindow* ViewportWindow = ImGui::FindWindowByName("Viewport");
    if (!ViewportWindow)
    {
        return;
    }

    const ImVec2 ViewportMin = ViewportWindow->ContentRegionRect.Min;
    const ImVec2 ViewportMax = ViewportWindow->ContentRegionRect.Max;

    const float ViewportWidth  = ViewportMax.x - ViewportMin.x;
    const float ViewportHeight = ViewportMax.y - ViewportMin.y;

    if (ViewportWidth <= 0.0f || ViewportHeight <= 0.0f)
    {
        return;
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

    FActor* SelectedActor = EditorEngine->GetSelectedActor();
    if (!SelectedActor)
    {
        return;
    }

    EditorGuizmo::SetRect(ViewportMin.x, ViewportMin.y, ViewportWidth, ViewportHeight);
    EditorGuizmo::SetOrthographic(false);

    EditorGuizmo::EOperation::Type EffectiveOperation = Viewport ? Viewport->GetGizmoOperation() : EditorGuizmo::EOperation::Translate;

    if (SelectedActor->HasComponentOfType<FPointLightComponent>() || SelectedActor->HasComponentOfType<FLightProbeComponent>())
    {
        EffectiveOperation = EditorGuizmo::EOperation::Translate;
    }
    else if (SelectedActor->HasComponentOfType<FDirectionalLightComponent>())
    {
        EffectiveOperation = EditorGuizmo::EOperation::Rotate;
    }
    else if (SelectedActor->HasComponentOfType<FCameraComponent>() && EffectiveOperation == EditorGuizmo::EOperation::Scale)
    {
        EffectiveOperation = EditorGuizmo::EOperation::Translate;
    }

    const FActorTransform& ActorWorldTransform = SelectedActor->GetWorldTransform();

    const Matrix4 ActorModel = ActorWorldTransform.GetTransformMatrix();
    Matrix4 Model = ActorModel;

    bool bUsingBoundsCenter = false;
    Vector3 InitialGizmoPosition = ActorModel.GetTranslation();

    if (Viewport && Viewport->GetGizmoPlacement() == FEditorViewportWidget::EGizmoPlacement::Center)
    {
        if (FStaticMeshComponent* MeshComponent = SelectedActor->GetComponentOfType<FStaticMeshComponent>())
        {
            const TSharedPtr<FMesh> Mesh = MeshComponent->GetMesh();
            if (Mesh && Mesh->GetVertexCount() > 0)
            {
                InitialGizmoPosition = ActorModel.Transform(Mesh->GetAABB().GetCenter());
                Model.SetTranslation(InitialGizmoPosition);
                bUsingBoundsCenter = true;
            }
        }
    }

    const EditorGuizmo::EMode Orientation = Viewport ? Viewport->GetGizmoOrientation() : EditorGuizmo::EMode::World;

    const bool bChanged = EditorGuizmo::Manipulate(Camera->GetViewMatrix(), Camera->GetProjectionMatrix(), EffectiveOperation, Orientation, Model);
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
            if (bUsingBoundsCenter)
            {
                const Vector3 TranslationDelta = Translation - InitialGizmoPosition;
                NewWorldTransform.SetTranslation(ActorWorldTransform.GetTranslation() + TranslationDelta);
            }
            else
            {
                NewWorldTransform.SetTranslation(Translation);
            }

            SelectedActor->SetWorldTransform(NewWorldTransform);
        }
        else if (EffectiveOperation == EditorGuizmo::EOperation::Rotate)
        {
            const Vector3 RotationRadians = Vector3::DegreesToRadians(RotationDegrees);
            if (FCameraComponent* CameraComponent = SelectedActor->GetComponentOfType<FCameraComponent>())
            {
                // The camera clamps and derives its direction vectors from the relative rotation, 
                // so feed it that instead of overwriting the whole transform.
                NewWorldTransform.SetRotation(RotationRadians);
                CameraComponent->SetRotation(SelectedActor->ConvertWorldToRelativeTransform(NewWorldTransform).GetRotation());
            }
            else
            {
                NewWorldTransform.SetRotation(RotationRadians);
                SelectedActor->SetWorldTransform(NewWorldTransform);
            }
        }
        else if (EffectiveOperation == EditorGuizmo::EOperation::Scale)
        {
            NewWorldTransform.SetScale(Scale);
            SelectedActor->SetWorldTransform(NewWorldTransform);
        }
    }
}
