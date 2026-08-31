#include "Engine/EngineUI/EditorUI/Panels/EditorAboutPanel.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Application/Elements/PropertyTable.h"
#include "Application/Elements/ScrollBox.h"
#include "Application/Elements/TextBlock.h"
#include "Core/Misc/BuildInfo.h"
#include "RHI/RHI.h"
#include "RHI/RHIDevice.h"

FEditorAboutPanel::FEditorAboutPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, "About", "About")
    , Table(nullptr)
{
}

FEditorAboutPanel::~FEditorAboutPanel()
{
}

bool FEditorAboutPanel::Initialize()
{
    FPropertyTable::FDesc TableDesc;
    TableDesc.Font = FEditorStyle::GetFonts().Body;

    Table = FPropertyTable::Create(TableDesc);
    if (!Table)
    {
        return false;
    }

    BuildRows();

    TSharedPtr<FScrollBox> ScrollBox = FScrollBox::Create();
    if (!ScrollBox)
    {
        return false;
    }

    ScrollBox->SetContent(Table);

    Content = ScrollBox;
    return true;
}

void FEditorAboutPanel::BuildRows()
{
    Table->AddRow("Engine", CreateValueText(String::Printf("%s %s", BuildInfo::GetEngineName(), BuildInfo::GetVersionString())));

    Table->AddHeaderRow("Source");

    Table->AddRow("Branch", CreateValueText(BuildInfo::GetBranch())).IndentLevel = 1;

    const String Commit = BuildInfo::IsWorkingTreeDirty()
        ? String::Printf("%s (dirty)", BuildInfo::GetCommit())
        : String(BuildInfo::GetCommit());

    Table->AddRow("Commit", CreateValueText(Commit)).IndentLevel = 1;
    Table->AddRow("Commit Date", CreateValueText(BuildInfo::GetCommitDate())).IndentLevel = 1;

    Table->AddHeaderRow("Build");

    Table->AddRow("Configuration", CreateValueText(String::Printf("%s (%s)", BuildInfo::GetConfigurationName(), BuildInfo::GetLinkageName()))).IndentLevel = 1;
    Table->AddRow("Platform", CreateValueText(String::Printf("%s %s", BuildInfo::GetPlatformName(), BuildInfo::GetArchitectureName()))).IndentLevel = 1;
    Table->AddRow("Compiler", CreateValueText(BuildInfo::GetCompilerName())).IndentLevel = 1;
    Table->AddRow("Compiled", CreateValueText(BuildInfo::GetCompileTimestamp())).IndentLevel = 1;

    Table->AddHeaderRow("Graphics");

    if (RHI::Device)
    {
        const String AdapterName = RHI::Device->GetAdapterName();

        Table->AddRow("Backend", CreateValueText(ToString(RHI::Device->GetRHIType()))).IndentLevel = 1;
        Table->AddRow("Adapter", CreateValueText(AdapterName.IsEmpty() ? String("Unknown") : AdapterName)).IndentLevel = 1;
    }
    else
    {
        Table->AddRow("Backend", CreateValueText("None")).IndentLevel = 1;
    }
}

void FEditorAboutPanel::Release()
{
    Table.Reset();

    FEditorPanel::Release();
}

void FEditorAboutPanel::Tick(float /*DeltaTime*/)
{
}

TSharedPtr<FVisualElement> FEditorAboutPanel::CreateValueText(const String& Text)
{
    FTextBlock::FDesc Desc;
    Desc.Text = Text;
    Desc.Font = FEditorStyle::GetFonts().Body;

    return FTextBlock::Create(Desc);
}
