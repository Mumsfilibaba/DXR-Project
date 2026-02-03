#include "Engine/EditorEngine.h"
#include "Engine/World/World.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Lights/PointLight.h"
#include "Engine/World/Reflections/LightProbe.h"
#include "Engine/EngineUI/Editor/EditorGuizmoWidget.h"
#include "ImGuiPlugin/ImGuiCore.h"

FEditorGuizmoWidget::FEditorGuizmoWidget(FEditorEngine* InEditorEngine)
    : EditorEngine(InEditorEngine)
    , ImGuiEndFrameDelegateHandle()
    , bVisible(true)
    , Operation(EditorGuizmo::Translate)
    , Mode(EditorGuizmo::World)
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

    if (ImGui::IsKeyPressed(ImGuiKey_1))
    {
        Operation = EditorGuizmo::Translate;
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_2))
    {
        Operation = EditorGuizmo::Rotate;
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_3))
    {
        Operation = EditorGuizmo::Scale;
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_4))
    {
        Mode = (Mode == EditorGuizmo::Local) ? EditorGuizmo::World : EditorGuizmo::Local;
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

    FWorld* World = EditorEngine->GetWorld();
    if (!World)
    {
        return;
    }

    FCamera* Camera = World->GetCamera();
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

    // We only draw the Gizmo when something is selected.
    FActor*      SelectedActor      = EditorEngine->GetSelectedActor();
    FLightProbe* SelectedLightProbe = EditorEngine->GetSelectedLightProbe();
    FLight*      SelectedLight      = EditorEngine->GetSelectedLight();
    FCamera*     SelectedCamera     = EditorEngine->GetSelectedCamera();

    const bool bHasAnySelection = (SelectedActor != nullptr) || (SelectedLightProbe != nullptr) || (SelectedLight != nullptr) || (SelectedCamera != nullptr);
    if (!bHasAnySelection)
    {
        return;
    }

    EditorGuizmo::SetRect(ViewportMin.x, ViewportMin.y, ViewportWidth, ViewportHeight);
    EditorGuizmo::SetOrthographic(false);

    // ---------------------------------------------------------------------
    // Actor (full TRS)
    // ---------------------------------------------------------------------

    if (SelectedActor)
    {
        FMatrix4 Model = SelectedActor->GetTransform().GetTransformMatrix();

        const bool bChanged = EditorGuizmo::Manipulate(Camera->GetViewMatrix(), Camera->GetProjectionMatrix(), Operation, Mode, Model);
        if (bChanged || EditorGuizmo::IsUsing())
        {
            FVector3 Translation;
            FVector3 RotationDegrees;
            FVector3 Scale;
            EditorGuizmo::DecomposeMatrixToComponents(Model, Translation, RotationDegrees, Scale);

            SelectedActor->GetTransform().SetTranslation(Translation);
            SelectedActor->GetTransform().SetRotation(FVector3::DegreesToRadians(RotationDegrees));
            SelectedActor->GetTransform().SetScale(Scale);
        }

        return;
    }

    // ---------------------------------------------------------------------
    // Camera (TR, no scale)
    // ---------------------------------------------------------------------

    if (SelectedCamera)
    {
        const FVector3 CamPos = SelectedCamera->GetPosition();
        const FVector3 CamRot = SelectedCamera->GetRotation();

        FMatrix4 Model = (FMatrix4::Scale(FVector3(1.0f, 1.0f, 1.0f)) * FMatrix4::RotationRollPitchYaw(CamRot)) * FMatrix4::Translation(CamPos);

        const bool bChanged = EditorGuizmo::Manipulate(Camera->GetViewMatrix(), Camera->GetProjectionMatrix(), Operation, Mode, Model);
        if (bChanged || EditorGuizmo::IsUsing())
        {
            FVector3 Translation;
            FVector3 RotationDegrees;
            FVector3 Scale;
            EditorGuizmo::DecomposeMatrixToComponents(Model, Translation, RotationDegrees, Scale);

            SelectedCamera->SetPosition(Translation.X, Translation.Y, Translation.Z);

            const FVector3 RotationRadians = FVector3::DegreesToRadians(RotationDegrees);
            SelectedCamera->SetRotation(RotationRadians.X, RotationRadians.Y, RotationRadians.Z);
        }

        return;
    }

    // ---------------------------------------------------------------------
    // Light Probe (translation only)
    // ---------------------------------------------------------------------

    if (SelectedLightProbe)
    {
        const FVector3 Pos = SelectedLightProbe->GetPosition();
        FMatrix4 Model = FMatrix4::Translation(Pos);

        const bool bChanged = EditorGuizmo::Manipulate(Camera->GetViewMatrix(), Camera->GetProjectionMatrix(), EditorGuizmo::Translate, EditorGuizmo::World, Model);
        if (bChanged || EditorGuizmo::IsUsing())
        {
            FVector3 Translation;
            FVector3 RotationDegrees;
            FVector3 Scale;
            EditorGuizmo::DecomposeMatrixToComponents(Model, Translation, RotationDegrees, Scale);

            SelectedLightProbe->SetPosition(Translation);
        }

        return;
    }

    // ---------------------------------------------------------------------
    // Point Light (translation only)
    // ---------------------------------------------------------------------

    if (SelectedLight)
    {
        if (FPointLight* PointLight = Cast<FPointLight>(SelectedLight))
        {
            const FVector3 Pos = PointLight->GetPosition();
            FMatrix4 Model = FMatrix4::Translation(Pos);

            const bool bChanged = EditorGuizmo::Manipulate(Camera->GetViewMatrix(), Camera->GetProjectionMatrix(), EditorGuizmo::Translate, EditorGuizmo::World, Model);
            if (bChanged || EditorGuizmo::IsUsing())
            {
                FVector3 Translation;
                FVector3 RotationDegrees;
                FVector3 Scale;
                EditorGuizmo::DecomposeMatrixToComponents(Model, Translation, RotationDegrees, Scale);

                PointLight->SetPosition(Translation);
            }
        }
    }
}
