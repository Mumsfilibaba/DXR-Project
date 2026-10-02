#include "Engine/EngineUI/EditorUI/Panels/EditorAboutPanel.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/Expander.h"
#include "Application/Elements/PropertyTable.h"
#include "Application/Elements/ScrollBox.h"
#include "Application/Elements/TextBlock.h"
#include "Core/Misc/BuildInfo.h"
#include "Core/Platform/PlatformSystemClipboard.h"
#include "RHI/RHI.h"
#include "RHI/RHIDevice.h"

static TSharedPtr<FVisualElement> BuildAboutHeader()
{
    const String Version = String::Printf("Version %s  |  %s", BuildInfo::GetVersionString(), BuildInfo::GetConfigurationName());

    return FEditorStyle::MakeHeaderCard(BuildInfo::GetEngineName(), Version);
}

FEditorAboutPanel::FEditorAboutPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, "About", "About")
    , Tables()
{
}

FEditorAboutPanel::~FEditorAboutPanel()
{
}

bool FEditorAboutPanel::Initialize()
{
    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    if (!Column)
    {
        return false;
    }

    BuildSections(Column);

    TSharedPtr<FScrollBox> ScrollBox = FScrollBox::Create();
    if (!ScrollBox)
    {
        return false;
    }

    ScrollBox->SetContent(Column);

    TSharedPtr<FVerticalBox> Layout = FVerticalBox::Create();
    Layout->AddSlot(BuildAboutHeader()).SetPadding(FMargin(0, 0, 0, FEditorStyle::ItemSpacing));
    Layout->AddSlot(ScrollBox).SetFillCoefficient(1.0f);

    Content = Layout;
    return true;
}

TSharedPtr<FPropertyTable> FEditorAboutPanel::AddSection(const TSharedPtr<FVerticalBox>& Column, const String& SectionName)
{
    TSharedPtr<FPropertyTable> SectionTable = FPropertyTable::Create(FEditorStyle::MakeInfoTableDesc());
    if (!SectionTable)
    {
        return nullptr;
    }

    SectionTable->SetOnRowContext(FOnPropertyRowContext::CreateLambda([this, SectionTable](int32 RowIndex)
    {
        CopyRow(SectionTable, RowIndex);
    }));

    Column->AddSlot(FExpander::Create(FEditorStyle::MakeExpanderDesc(SectionName, SectionTable, true))).SetPadding(FEditorStyle::GetSectionStackSpacing());
    Tables.Emplace(SectionTable);

    return SectionTable;
}

void FEditorAboutPanel::BuildSections(const TSharedPtr<FVerticalBox>& Column)
{
    if (TSharedPtr<FPropertyTable> Source = AddSection(Column, "Source"))
    {
        const String Commit = BuildInfo::IsWorkingTreeDirty()
            ? String::Printf("%s (dirty)", BuildInfo::GetCommit())
            : String(BuildInfo::GetCommit());

        Source->AddRow("Branch", CreateValueText(BuildInfo::GetBranch()));
        Source->AddRow("Commit", CreateValueText(Commit));
        Source->AddRow("Commit Date", CreateValueText(BuildInfo::GetCommitDate()));
    }

    if (TSharedPtr<FPropertyTable> Build = AddSection(Column, "Build"))
    {
        Build->AddRow("Configuration", CreateValueText(String::Printf("%s (%s)", BuildInfo::GetConfigurationName(), BuildInfo::GetLinkageName())));
        Build->AddRow("Platform", CreateValueText(String::Printf("%s %s", BuildInfo::GetPlatformName(), BuildInfo::GetArchitectureName())));
        Build->AddRow("Compiler", CreateValueText(BuildInfo::GetCompilerName()));
        Build->AddRow("Compiled", CreateValueText(BuildInfo::GetCompileTimestamp()));
    }

    if (TSharedPtr<FPropertyTable> Graphics = AddSection(Column, "Graphics"))
    {
        if (RHI::Device)
        {
            const String AdapterName = RHI::Device->GetAdapterName();

            Graphics->AddRow("Backend", CreateValueText(ToString(RHI::Device->GetRHIType())));
            Graphics->AddRow("Adapter", CreateValueText(AdapterName.IsEmpty() ? String("Unknown") : AdapterName));
        }
        else
        {
            Graphics->AddRow("Backend", CreateValueText("None"));
        }
    }
}

void FEditorAboutPanel::Release()
{
    Tables.Clear();

    FEditorPanel::Release();
}

void FEditorAboutPanel::Tick(float /*DeltaTime*/)
{
}

TSharedPtr<FVisualElement> FEditorAboutPanel::CreateValueText(const String& Text)
{
    FTextBlock::FDesc Desc;
    Desc.Text     = Text;
    Desc.Font     = FEditorStyle::GetFonts().Monospace;
    Desc.Overflow = ETextOverflow::Elide;

    return FTextBlock::Create(Desc);
}

void FEditorAboutPanel::CopyRow(const TSharedPtr<FPropertyTable>& FromTable, int32 RowIndex)
{
    if (!FromTable || RowIndex < 0 || RowIndex >= FromTable->GetNumRows())
    {
        return;
    }

    const FPropertyRow& Row = FromTable->GetRow(RowIndex);

    String Value;
    if (const TSharedPtr<FTextBlock> ValueText = StaticCastSharedPtr<FTextBlock>(Row.Editor))
    {
        Value = ValueText->GetText();
    }

    FPlatformSystemClipboard::SetText(String::Printf("%s: %s", *Row.Label, *Value));
}
