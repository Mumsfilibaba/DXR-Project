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
    const FString Version = FString::Printf(TEXT("Version %s  |  %s"), BuildInfo::GetVersionString(), BuildInfo::GetConfigurationName());

    return FEditorStyle::MakeHeaderCard(BuildInfo::GetEngineName(), Version);
}

FEditorAboutPanel::FEditorAboutPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, TEXT("About"), TEXT("About"))
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

TSharedPtr<FPropertyTable> FEditorAboutPanel::AddSection(const TSharedPtr<FVerticalBox>& Column, const FString& SectionName)
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
    if (TSharedPtr<FPropertyTable> Source = AddSection(Column, TEXT("Source")))
    {
        const FString Commit = BuildInfo::IsWorkingTreeDirty()
            ? FString::Printf(TEXT("%s (dirty)"), BuildInfo::GetCommit())
            : FString(BuildInfo::GetCommit());

        Source->AddRow(TEXT("Branch"), CreateValueText(BuildInfo::GetBranch()));
        Source->AddRow(TEXT("Commit"), CreateValueText(Commit));
        Source->AddRow(TEXT("Commit Date"), CreateValueText(BuildInfo::GetCommitDate()));
    }

    if (TSharedPtr<FPropertyTable> Build = AddSection(Column, TEXT("Build")))
    {
        Build->AddRow(TEXT("Configuration"), CreateValueText(FString::Printf(TEXT("%s (%s)"), BuildInfo::GetConfigurationName(), BuildInfo::GetLinkageName())));
        Build->AddRow(TEXT("Platform"), CreateValueText(FString::Printf(TEXT("%s %s"), BuildInfo::GetPlatformName(), BuildInfo::GetArchitectureName())));
        Build->AddRow(TEXT("Compiler"), CreateValueText(BuildInfo::GetCompilerName()));
        Build->AddRow(TEXT("Compiled"), CreateValueText(BuildInfo::GetCompileTimestamp()));
    }

    if (TSharedPtr<FPropertyTable> Graphics = AddSection(Column, TEXT("Graphics")))
    {
        if (RHI::Device)
        {
            const FString AdapterName = RHI::Device->GetAdapterName();

            Graphics->AddRow(TEXT("Backend"), CreateValueText(ToString(RHI::Device->GetRHIType())));
            Graphics->AddRow(TEXT("Adapter"), CreateValueText(AdapterName.IsEmpty() ? FString(TEXT("Unknown")) : AdapterName));
        }
        else
        {
            Graphics->AddRow(TEXT("Backend"), CreateValueText(TEXT("None")));
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

TSharedPtr<FVisualElement> FEditorAboutPanel::CreateValueText(const FString& Text)
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

    FString Value;
    if (const TSharedPtr<FTextBlock> ValueText = StaticCastSharedPtr<FTextBlock>(Row.Editor))
    {
        Value = ValueText->GetText();
    }

    FPlatformSystemClipboard::SetText(FString::Printf(TEXT("%s: %s"), *Row.Label, *Value));
}
