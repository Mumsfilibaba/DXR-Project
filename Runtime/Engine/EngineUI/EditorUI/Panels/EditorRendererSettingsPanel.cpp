#include "Engine/EngineUI/EditorUI/Panels/EditorRendererSettingsPanel.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/CheckBox.h"
#include "Application/Elements/EditableText.h"
#include "Application/Elements/Expander.h"
#include "Application/Elements/NumericEntry.h"
#include "Application/Elements/PropertyTable.h"
#include "Application/Elements/ScrollBox.h"
#include "Application/Elements/SearchBox.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Menus/ComboBox.h"
#include "ShaderCompiler/ShaderCompiler.h"

static const CHAR_T* const GCascadeResolutions[]      = { TEXT("512"), TEXT("1024"), TEXT("2048"), TEXT("4096") };
static const int32         GCascadeResolutionValues[] = { 512, 1024, 2048, 4096 };

static const CHAR_T* const GPointResolutions[]      = { TEXT("128"), TEXT("256"), TEXT("512"), TEXT("1024") };
static const int32         GPointResolutionValues[] = { 128, 256, 512, 1024 };

static const CHAR_T* const GSampleCounts[]      = { TEXT("16"), TEXT("32"), TEXT("64"), TEXT("128") };
static const int32         GSampleCountValues[] = { 16, 32, 64, 128 };

static const CHAR_T* const GFilterModes[]        = { TEXT("PCF"), TEXT("PCSS") };
static const CHAR_T* const GFilterFunctions[]    = { TEXT("Grid"), TEXT("Poisson Disk"), TEXT("Vogel Disk") };
static const CHAR_T* const GReflectionSamplers[] = { TEXT("White noise"), TEXT("Halton"), TEXT("Blue noise") };
static const CHAR_T* const GTonemappers[]        = { TEXT("Default"), TEXT("ACES"), TEXT("Reinhard"), TEXT("Uncharted 2") };

static const CHAR_T* const GRayTracingSection     = TEXT("Ray Tracing");
static const CHAR_T* const GShaderCompilerSection = TEXT("Shader Compiler");

static FString DescribeShaderCompilerStatus()
{
    const FShaderCompiler* Compiler = FShaderCompiler::TryGet();
    return Compiler ? Compiler->DescribeRemoteStatus() : FString(TEXT("Not initialized"));
}

static FString DescribeRayTracingSupport()
{
    if (!RHI::bSupportsRayTracing)
    {
        return TEXT("Not supported");
    }

    if (!RHI::bSupportsRayTracingPipeline)
    {
        return TEXT("Inline only");
    }

    return FString::Printf(TEXT("Tier %s, inline %s, SER %s"),
        ToString(RHI::RayTracingTier),
        RHI::bSupportsInlineRayTracing ? TEXT("yes") : TEXT("no"),
        RHI::bSupportsShaderExecutionReordering ? TEXT("yes") : TEXT("no"));
}

#define SETTING_BOOL(Section, Label, Name) \
    { Section, Label, Name, ERendererSettingKind::Bool, 0.0f, 0.0f, 0.0f, nullptr, nullptr, 0, nullptr }

#define SETTING_BOOL_GATED(Section, Label, Name, Predicate) \
    { Section, Label, Name, ERendererSettingKind::Bool, 0.0f, 0.0f, 0.0f, nullptr, nullptr, 0, Predicate }

#define SETTING_INT(Section, Label, Name, Min, Max) \
    { Section, Label, Name, ERendererSettingKind::Int, float(Min), float(Max), 1.0f, nullptr, nullptr, 0, nullptr }

#define SETTING_FLOAT(Section, Label, Name, Min, Max, Step) \
    { Section, Label, Name, ERendererSettingKind::Float, Min, Max, Step, nullptr, nullptr, 0, nullptr }

#define SETTING_COMBO(Section, Label, Name, Items) \
    { Section, Label, Name, ERendererSettingKind::Combo, 0.0f, 0.0f, 0.0f, Items, nullptr, int32(ARRAY_COUNT(Items)), nullptr }

#define SETTING_COMBO_VALUES(Section, Label, Name, Items, Values) \
    { Section, Label, Name, ERendererSettingKind::ComboValues, 0.0f, 0.0f, 0.0f, Items, Values, int32(ARRAY_COUNT(Items)), nullptr }

#define SETTING_STRING(Section, Label, Name) \
    { Section, Label, Name, ERendererSettingKind::String, 0.0f, 0.0f, 0.0f, nullptr, nullptr, 0, nullptr }

static const FRendererSetting GRendererSettings[] =
{
    SETTING_BOOL(TEXT("Deferred Rendering"), TEXT("Draw tile-debug"),   TEXT("Renderer.Debug.DrawTiledLightning")),
    SETTING_BOOL(TEXT("Deferred Rendering"), TEXT("Clear all targets"), TEXT("Renderer.BasePass.ClearAllTargets")),
    SETTING_BOOL(TEXT("Deferred Rendering"), TEXT("Enable pre-pass"),   TEXT("Renderer.Feature.PrePass")),
    SETTING_BOOL(TEXT("Deferred Rendering"), TEXT("Enable base-pass"),  TEXT("Renderer.Feature.BasePass")),

    SETTING_BOOL(TEXT("Shadows"), TEXT("Enable shadows"), TEXT("Renderer.Feature.Shadows")),

    SETTING_BOOL        (TEXT("Cascaded Shadow Maps"), TEXT("Enable sun shadows"),             TEXT("Renderer.Feature.SunShadows")),
    SETTING_BOOL        (TEXT("Cascaded Shadow Maps"), TEXT("Enable shadow mask"),             TEXT("Renderer.Feature.ShadowMask")),
    SETTING_BOOL        (TEXT("Cascaded Shadow Maps"), TEXT("Tight frustum (depth reduce)"),   TEXT("Renderer.CSM.TightFrustum")),
    SETTING_COMBO_VALUES(TEXT("Cascaded Shadow Maps"), TEXT("Cascade resolution"),             TEXT("Renderer.CSM.CascadeSize"), GCascadeResolutions, GCascadeResolutionValues),
    SETTING_BOOL        (TEXT("Cascaded Shadow Maps"), TEXT("Stable cascades"),                TEXT("Renderer.CSM.StableCascades")),
    SETTING_BOOL        (TEXT("Cascaded Shadow Maps"), TEXT("Single-pass rendering"),          TEXT("Renderer.CSM.EnableSinglePassRendering")),
    SETTING_BOOL        (TEXT("Cascaded Shadow Maps"), TEXT("Geometry shader instancing"),     TEXT("Renderer.CSM.EnableGeometryShaderInstancing")),
    SETTING_BOOL        (TEXT("Cascaded Shadow Maps"), TEXT("View instancing"),                TEXT("Renderer.CSM.EnableViewInstancing")),
    SETTING_BOOL        (TEXT("Cascaded Shadow Maps"), TEXT("Depth clipping"),                 TEXT("Renderer.CSM.EnableDepthClipping")),
    SETTING_BOOL        (TEXT("Cascaded Shadow Maps"), TEXT("Blend cascades"),                 TEXT("Renderer.CSM.BlendCascades")),
    SETTING_BOOL        (TEXT("Cascaded Shadow Maps"), TEXT("Select cascade from projection"), TEXT("Renderer.CSM.SelectCascadeFromProjection")),
    SETTING_COMBO       (TEXT("Cascaded Shadow Maps"), TEXT("Filter mode"),                    TEXT("Renderer.CSM.FilterMode"), GFilterModes),
    SETTING_COMBO       (TEXT("Cascaded Shadow Maps"), TEXT("Filter function"),                TEXT("Renderer.CSM.FilterFunction"), GFilterFunctions),
    SETTING_INT         (TEXT("Cascaded Shadow Maps"), TEXT("Filter size"),                    TEXT("Renderer.CSM.FilterSize"), 1, 2048),
    SETTING_INT         (TEXT("Cascaded Shadow Maps"), TEXT("Max filter size"),                TEXT("Renderer.CSM.MaxFilterSize"), 1, 2048),
    SETTING_COMBO_VALUES(TEXT("Cascaded Shadow Maps"), TEXT("Sample count"),                   TEXT("Renderer.CSM.NumPoissonDiscSamples"), GSampleCounts, GSampleCountValues),
    SETTING_BOOL        (TEXT("Cascaded Shadow Maps"), TEXT("Rotate samples"),                 TEXT("Renderer.CSM.RotateSamples")),

    SETTING_BOOL        (TEXT("Point-light Shadow Maps"), TEXT("Enable point-light shadows"), TEXT("Renderer.Feature.PointLightShadows")),
    SETTING_COMBO_VALUES(TEXT("Point-light Shadow Maps"), TEXT("Shadow map resolution"),      TEXT("Renderer.Shadows.PointLightShadowMapSize"), GPointResolutions, GPointResolutionValues),
    SETTING_BOOL        (TEXT("Point-light Shadow Maps"), TEXT("Single-pass rendering"),      TEXT("Renderer.PointLights.EnableSinglePassRendering")),
    SETTING_BOOL        (TEXT("Point-light Shadow Maps"), TEXT("Geometry shader instancing"), TEXT("Renderer.PointLights.EnableGeometryShaderInstancing")),

    SETTING_BOOL(TEXT("Skybox"), TEXT("Enable Skybox"),       TEXT("Renderer.Feature.Skybox")),
    SETTING_BOOL(TEXT("Skybox"), TEXT("Clear before skybox"), TEXT("Renderer.Skybox.ClearBeforeSkybox")),

    SETTING_BOOL (TEXT("SSAO"), TEXT("Enable SSAO"), TEXT("Renderer.Feature.SSAO")),
    SETTING_INT  (TEXT("SSAO"), TEXT("Kernel size"), TEXT("Renderer.SSAO.KernelSize"), 1, 128),
    SETTING_FLOAT(TEXT("SSAO"), TEXT("Radius"),      TEXT("Renderer.SSAO.Radius"), 0.01f, 1.0f, 0.01f),
    SETTING_FLOAT(TEXT("SSAO"), TEXT("Bias"),        TEXT("Renderer.SSAO.Bias"), 0.01f, 1.0f, 0.01f),

    SETTING_BOOL_GATED(TEXT("Ray Tracing"), TEXT("Enable ray tracing"),                TEXT("Renderer.Feature.RayTracing"),
        []() { return RHI::bSupportsRayTracing; }),
    SETTING_BOOL_GATED(TEXT("Ray Tracing"), TEXT("Use local shader bindings"),         TEXT("Renderer.RayTracing.EnableLocalShaderBindings"),
        []() { return RHI::bSupportsRayTracingPipeline; }),
    SETTING_BOOL_GATED(TEXT("Ray Tracing"), TEXT("Inline reflections (RayQuery)"),     TEXT("Renderer.RayTracing.InlineReflections"),
        []() { return RHI::bSupportsInlineRayTracing; }),
    SETTING_BOOL_GATED(TEXT("Ray Tracing"), TEXT("Shader Execution Reordering"),       TEXT("Renderer.RayTracing.SER"),
        []() { return RHI::bSupportsShaderExecutionReordering; }),
    SETTING_BOOL_GATED(TEXT("Ray Tracing"), TEXT("BLAS compaction"),                   TEXT("Renderer.RayTracing.Compaction"),
        []() { return RHI::bSupportsRayTracing; }),
    SETTING_BOOL_GATED(TEXT("Ray Tracing"), TEXT("Acceleration-structure disk cache"), TEXT("Renderer.RayTracing.ASCache"),
        []() { return RHI::bSupportsRayTracing; }),

    SETTING_BOOL (TEXT("Reflections"), TEXT("Enable"),                       TEXT("Renderer.RayTracing.Reflections.Enable")),
    SETTING_FLOAT(TEXT("Reflections"), TEXT("Indirect specular strength"),   TEXT("Renderer.Reflections.IndirectSpecularStrength"), 0.0f, 4.0f, 0.01f),
    SETTING_BOOL (TEXT("Reflections"), TEXT("Half resolution"),              TEXT("Renderer.RayTracing.Reflections.HalfRes")),
    SETTING_FLOAT(TEXT("Reflections"), TEXT("Max ray distance"),             TEXT("Renderer.RayTracing.Reflections.MaxRayDistance"), 1.0f, 100000.0f, 10.0f),
    SETTING_FLOAT(TEXT("Reflections"), TEXT("Mirror roughness threshold"),   TEXT("Renderer.RayTracing.Reflections.MirrorRoughnessThreshold"), 0.0f, 1.0f, 0.001f),
    SETTING_FLOAT(TEXT("Reflections"), TEXT("Ray bias"),                     TEXT("Renderer.RayTracing.Reflections.RayBias"), 0.0f, 1.0f, 0.001f),
    SETTING_COMBO(TEXT("Reflections"), TEXT("GGX sampler"),                  TEXT("Renderer.RayTracing.Reflections.Sampler"), GReflectionSamplers),
    SETTING_BOOL (TEXT("Reflections"), TEXT("Enable denoiser"),              TEXT("Renderer.RayTracing.Reflections.Denoise")),
    SETTING_FLOAT(TEXT("Reflections"), TEXT("Temporal alpha"),               TEXT("Renderer.RayTracing.Reflections.TemporalAlpha"), 0.01f, 1.0f, 0.01f),
    SETTING_FLOAT(TEXT("Reflections"), TEXT("Max radiance (firefly clamp)"), TEXT("Renderer.RayTracing.Reflections.MaxRadiance"), 0.0f, 100.0f, 0.1f),
    SETTING_FLOAT(TEXT("Reflections"), TEXT("History clamp gamma"),          TEXT("Renderer.RayTracing.Reflections.HistoryClampGamma"), 0.0f, 10.0f, 0.01f),
    SETTING_FLOAT(TEXT("Reflections"), TEXT("Max history length"),           TEXT("Renderer.RayTracing.Reflections.MaxHistoryLength"), 1.0f, 128.0f, 1.0f),
    SETTING_INT  (TEXT("Reflections"), TEXT("Neighborhood radius"),          TEXT("Renderer.RayTracing.Reflections.NeighborhoodRadius"), 0, 8),
    SETTING_FLOAT(TEXT("Reflections"), TEXT("Camera-motion max history"),    TEXT("Renderer.RayTracing.Reflections.CameraMotionMaxHistory"), 0.0f, 64.0f, 0.5f),
    SETTING_INT  (TEXT("Reflections"), TEXT("A-trous iterations"),           TEXT("Renderer.RayTracing.Reflections.AtrousIterations"), 0, 8),
    SETTING_FLOAT(TEXT("Reflections"), TEXT("A-trous color phi"),            TEXT("Renderer.RayTracing.Reflections.AtrousPhiColor"), 0.1f, 32.0f, 0.1f),

    SETTING_BOOL(TEXT("Temporal Anti-aliasing (TAA)"), TEXT("Enabled"),         TEXT("Renderer.Feature.TemporalAntiAliasing")),
    SETTING_BOOL(TEXT("Temporal Anti-aliasing (TAA)"), TEXT("Hardware jitter"), TEXT("Renderer.TemporalAntiAliasing.HardwareJitter")),

    SETTING_BOOL(TEXT("FXAA"), TEXT("Enable FXAA"),       TEXT("Renderer.Feature.FXAA")),
    SETTING_BOOL(TEXT("FXAA"), TEXT("Enable FXAA debug"), TEXT("Renderer.Debug.FXAADebug")),

    SETTING_COMBO(TEXT("Tonemapping"), TEXT("Tonemapping function"), TEXT("Renderer.Tonemapping.Function"), GTonemappers),
    SETTING_FLOAT(TEXT("Tonemapping"), TEXT("Exposure (EV100)"),     TEXT("Renderer.Tonemapping.EV100"), -10.0f, 20.0f, 0.1f),
    SETTING_FLOAT(TEXT("Tonemapping"), TEXT("Reinhard intensity"),   TEXT("Renderer.Tonemapping.ReinhardIntensity"), 0.1f, 10.0f, 0.01f),

    SETTING_BOOL(TEXT("Display"), TEXT("Enable VSync"), TEXT("Renderer.Feature.VerticalSync")),

    SETTING_BOOL(TEXT("Culling"), TEXT("Enable frustum-culling"), TEXT("Renderer.Feature.FrustumCulling")),

    SETTING_BOOL(TEXT("Debug"), TEXT("Enable debug-draw AABBs"),        TEXT("Renderer.Debug.DrawAABBs")),
    SETTING_BOOL(TEXT("Debug"), TEXT("Enable debug-draw point-lights"), TEXT("Renderer.Debug.DrawPointLights")),
    SETTING_BOOL(TEXT("Debug"), TEXT("Enable debug-draw light-probes"), TEXT("Renderer.Debug.LightProbes")),

    SETTING_BOOL  (TEXT("Shader Compiler"), TEXT("Use remote shader compiler"), TEXT("RHI.ShaderCompiler.UseRemote")),
    SETTING_STRING(TEXT("Shader Compiler"), TEXT("Remote host"),                TEXT("RHI.ShaderCompiler.RemoteHost")),
    SETTING_INT   (TEXT("Shader Compiler"), TEXT("Remote port"),                TEXT("RHI.ShaderCompiler.RemotePort"), 1, 65535),
    SETTING_BOOL  (TEXT("Shader Compiler"), TEXT("Fall back to local"),         TEXT("RHI.ShaderCompiler.RemoteFallbackToLocal")),
};

#undef SETTING_BOOL
#undef SETTING_INT
#undef SETTING_FLOAT
#undef SETTING_COMBO
#undef SETTING_COMBO_VALUES
#undef SETTING_STRING

static const FRendererSubsection GRendererSubsections[] =
{
    { TEXT("Cascaded Shadow Maps"),    TEXT("Shadows")     },
    { TEXT("Point-light Shadow Maps"), TEXT("Shadows")     },
    { TEXT("Reflections"),             TEXT("Ray Tracing") },
};

constexpr float RENDERER_SETTINGS_LABEL_FRACTION = 0.62f;

static const CHAR_T* FindParentSection(const CHAR_T* Section)
{
    for (const FRendererSubsection& Subsection : GRendererSubsections)
    {
        if (FString(Subsection.Section) == Section)
        {
            return Subsection.ParentSection;
        }
    }

    return nullptr;
}

FEditorRendererSettingsPanel::FEditorRendererSettingsPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, TEXT("RendererSettings"), TEXT("Renderer Settings"))
    , ScrollBox(nullptr)
    , Column(nullptr)
    , SearchBox(nullptr)
    , Rows()
    , FilterText()
{
}

FEditorRendererSettingsPanel::~FEditorRendererSettingsPanel()
{
}

bool FEditorRendererSettingsPanel::Initialize()
{
    Column = FVerticalBox::Create();

    ScrollBox = FScrollBox::Create();
    if (!ScrollBox)
    {
        return false;
    }

    ScrollBox->SetContent(Column);

    SearchBox = FSearchBox::Create(FEditorStyle::MakeSearchBoxDesc(TEXT("Search Settings"),
        FOnSearchTextChanged::CreateRaw(this, &FEditorRendererSettingsPanel::OnSearchTextChanged)));


    TSharedPtr<FVerticalBox> Layout = FVerticalBox::Create();
    Layout->AddSlot(SearchBox).SetPadding(FMargin(0, 0, 0, FEditorStyle::ItemSpacing));
    Layout->AddSlot(ScrollBox).SetFillCoefficient(1.0f);

    Content = Layout;

    RebuildSections();
    return true;
}

void FEditorRendererSettingsPanel::Release()
{
    ScrollBox.Reset();
    Column.Reset();
    SearchBox.Reset();
    ShaderCompilerStatus.Reset();
    Rows.Clear();

    FEditorPanel::Release();
}

bool FEditorRendererSettingsPanel::PassesFilter(const FRendererSetting& Setting) const
{
    if (FilterText.IsEmpty())
    {
        return true;
    }

    return FString(Setting.Label).Contains(FilterText, EStringCaseType::NoCase)
        || FString(Setting.CVarName).Contains(FilterText, EStringCaseType::NoCase);
}

void FEditorRendererSettingsPanel::RebuildSections()
{
    Column->ClearSlots();
    Rows.Clear();
    ShaderCompilerStatus.Reset();

    FConsoleManager& ConsoleManager = FConsoleManager::Get();

    struct FSectionBox
    {
        const CHAR_T*            Name;
        TSharedPtr<FVerticalBox> Box;
    };

    const bool bStartExpanded = !FilterText.IsEmpty();
    TArray<FSectionBox> SectionBoxes;

    const CHAR_T*              CurrentSection = nullptr;
    TSharedPtr<FPropertyTable> CurrentTable   = nullptr;

    for (const FRendererSetting& Setting : GRendererSettings)
    {
        if (!PassesFilter(Setting))
        {
            continue;
        }

        IConsoleVariable* Variable = ConsoleManager.FindConsoleVariable(Setting.CVarName);
        if (!Variable)
        {
            continue;
        }

        TSharedPtr<FVisualElement> Editor = BuildEditor(Setting, Variable);
        if (!Editor)
        {
            continue;
        }

        if (!CurrentSection || FString(CurrentSection) != Setting.Section)
        {
            CurrentTable   = FPropertyTable::Create(FEditorStyle::MakePropertyTableDesc(RENDERER_SETTINGS_LABEL_FRACTION));
            CurrentSection = Setting.Section;

            TSharedPtr<FVerticalBox> SectionBox = FVerticalBox::Create();
            SectionBox->AddSlot(CurrentTable);

            TSharedPtr<FExpander> Section = FExpander::Create(FEditorStyle::MakeExpanderDesc(Setting.Section, SectionBox, bStartExpanded));

            // A subsection is content of the section above it, so it indents and collapses with it
            const CHAR_T*            ParentName = FindParentSection(Setting.Section);
            TSharedPtr<FVerticalBox> ParentBox  = nullptr;

            for (const FSectionBox& Candidate : SectionBoxes)
            {
                if (ParentName && FString(Candidate.Name) == ParentName)
                {
                    ParentBox = Candidate.Box;
                    break;
                }
            }

            if (ParentBox)
            {
                ParentBox->AddSlot(Section).SetPadding(FEditorStyle::GetSectionStackSpacing());
            }
            else
            {
                Column->AddSlot(Section).SetPadding(FEditorStyle::GetSectionStackSpacing());
            }

            SectionBoxes.Emplace(FSectionBox{ Setting.Section, SectionBox });

            if (FString(Setting.Section) == GRayTracingSection)
            {
                FTextBlock::FDesc SupportDesc;
                SupportDesc.Text = DescribeRayTracingSupport();
                SupportDesc.Font = FEditorStyle::GetFonts().Monospace;

                CurrentTable->AddRow(TEXT("Hardware support"), FTextBlock::Create(SupportDesc)).ToolTipText =
                    TEXT("What this RHI reports, which is what decides whether the rows below it do anything");
            }
            else if (FString(Setting.Section) == GShaderCompilerSection)
            {
                FTextBlock::FDesc StatusDesc;
                StatusDesc.Text = DescribeShaderCompilerStatus();
                StatusDesc.Font = FEditorStyle::GetFonts().Monospace;

                ShaderCompilerStatus = FTextBlock::Create(StatusDesc);
                CurrentTable->AddRow(TEXT("Status"), ShaderCompilerStatus).ToolTipText =
                    TEXT("Where shaders compile right now. Shaders already compiled stay as they are until Renderer.RecompileShaders.");
            }
        }

        const bool bIsSupported = !Setting.IsSupported || Setting.IsSupported();

        FPropertyRow& Row = CurrentTable->AddRow(Setting.Label, Editor);
        Row.ToolTipText   = bIsSupported ? FString(Setting.CVarName) : FString::Printf(TEXT("%s (not supported by this RHI)"), Setting.CVarName);

        if (!bIsSupported && Setting.Kind == ERendererSettingKind::Bool)
        {
            StaticCastSharedPtr<FCheckBox>(Editor)->SetEnabled(false);
        }

        FSettingRow Entry;
        Entry.Setting  = &Setting;
        Entry.Variable = Variable;
        Entry.Editor   = Editor;

        Rows.Emplace(Entry);
    }
}

TSharedPtr<FVisualElement> FEditorRendererSettingsPanel::BuildEditor(const FRendererSetting& Setting, IConsoleVariable* Variable)
{
    switch (Setting.Kind)
    {
        case ERendererSettingKind::Bool:
        {
            FCheckBox::FDesc Desc;
            Desc.InitialState   = Variable->GetBool() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
            Desc.Font           = FEditorStyle::GetFonts().Body;
            Desc.OnStateChanged = FOnCheckStateChanged::CreateLambda([Variable](ECheckBoxState State)
            {
                Variable->SetAsBool(State == ECheckBoxState::Checked, EConsoleVariableFlags::SetByCode);
            });

            return FCheckBox::Create(Desc);
        }

        case ERendererSettingKind::Int:
        {
            TNumericEntry<int32>::FDesc Desc;
            Desc.Value          = Math::Clamp<int32>(Variable->GetInt(), int32(Setting.MinValue), int32(Setting.MaxValue));
            Desc.MinValue       = int32(Setting.MinValue);
            Desc.MaxValue       = int32(Setting.MaxValue);
            Desc.Font           = FEditorStyle::GetFonts().Body;
            Desc.bShowLabel     = false;
            Desc.OnValueChanged = TNumericEntry<int32>::FOnValueChanged::CreateLambda([Variable](int32 Value)
            {
                Variable->SetAsInt(Value, EConsoleVariableFlags::SetByCode);
            });

            return TNumericEntry<int32>::Create(Desc);
        }

        case ERendererSettingKind::Float:
        {
            TNumericEntry<float>::FDesc Desc;
            Desc.Value          = Math::Clamp<float>(Variable->GetFloat(), Setting.MinValue, Setting.MaxValue);
            Desc.MinValue       = Setting.MinValue;
            Desc.MaxValue       = Setting.MaxValue;
            Desc.Step           = Setting.Step;
            Desc.Font           = FEditorStyle::GetFonts().Body;
            Desc.bShowLabel     = false;
            Desc.OnValueChanged = TNumericEntry<float>::FOnValueChanged::CreateLambda([Variable](float Value)
            {
                Variable->SetAsFloat(Value, EConsoleVariableFlags::SetByCode);
            });

            return TNumericEntry<float>::Create(Desc);
        }

        case ERendererSettingKind::Combo:
        case ERendererSettingKind::ComboValues:
        {
            const bool   bMapsToValues = Setting.Kind == ERendererSettingKind::ComboValues;
            const int32  RawValue      = Variable->GetInt();
            const int32* OptionValues  = Setting.OptionValues;

            int32 SelectedIndex = 0;
            if (bMapsToValues)
            {
                for (int32 Index = 0; Index < Setting.NumOptions; ++Index)
                {
                    if (OptionValues[Index] == RawValue)
                    {
                        SelectedIndex = Index;
                        break;
                    }
                }
            }
            else
            {
                SelectedIndex = Math::Clamp<int32>(RawValue, 0, Setting.NumOptions - 1);
            }

            TArray<FString> Options;
            Options.Reserve(Setting.NumOptions);
            for (int32 Index = 0; Index < Setting.NumOptions; ++Index)
            {
                Options.Emplace(Setting.Options[Index]);
            }

            FComboBox::FDesc Desc;
            Desc.Options            = Options;
            Desc.SelectedIndex      = SelectedIndex;
            Desc.Font               = FEditorStyle::GetFonts().Body;
            Desc.OnSelectionChanged = FOnComboSelectionChanged::CreateLambda([Variable, OptionValues](int32 Index)
            {
                if (Index >= 0)
                {
                    Variable->SetAsInt(OptionValues ? OptionValues[Index] : Index, EConsoleVariableFlags::SetByCode);
                }
            });

            return FComboBox::Create(Desc);
        }

        case ERendererSettingKind::String:
        {
            FEditableText::FDesc Desc;
            Desc.Text = Variable->GetString();
            Desc.Font = FEditorStyle::GetFonts().Body;

            TSharedPtr<FEditableText> TextEditor = FEditableText::Create(Desc);
            TextEditor->GetOnTextCommitted() = FOnTextCommittedDelegate::CreateLambda([Variable](const FString& Text)
            {
                Variable->SetString(Text, EConsoleVariableFlags::SetByCode);
            });

            return TextEditor;
        }
    }

    return nullptr;
}

void FEditorRendererSettingsPanel::OnSearchTextChanged(const FString& SearchText)
{
    if (FilterText == SearchText)
    {
        return;
    }

    FilterText = SearchText;
    RebuildSections();
}

void FEditorRendererSettingsPanel::Tick(float /*DeltaTime*/)
{
    if (!IsVisible())
    {
        return;
    }

    if (Rows.IsEmpty() && FilterText.IsEmpty())
    {
        RebuildSections();
    }

    RefreshValues();
}

void FEditorRendererSettingsPanel::RefreshValues()
{
    for (const FSettingRow& Row : Rows)
    {
        switch (Row.Setting->Kind)
        {
            case ERendererSettingKind::Bool:
            {
                TSharedPtr<FCheckBox> CheckBox = StaticCastSharedPtr<FCheckBox>(Row.Editor);
                CheckBox->SetCheckState(Row.Variable->GetBool() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
                break;
            }

            case ERendererSettingKind::Int:
            {
                TSharedPtr<TNumericEntry<int32>> Entry = StaticCastSharedPtr<TNumericEntry<int32>>(Row.Editor);
                Entry->SetValue(Row.Variable->GetInt());
                break;
            }

            case ERendererSettingKind::Float:
            {
                TSharedPtr<TNumericEntry<float>> Entry = StaticCastSharedPtr<TNumericEntry<float>>(Row.Editor);
                Entry->SetValue(Row.Variable->GetFloat());
                break;
            }

            case ERendererSettingKind::Combo:
            case ERendererSettingKind::ComboValues:
            {
                break;
            }

            case ERendererSettingKind::String:
            {
                TSharedPtr<FEditableText> TextEditor = StaticCastSharedPtr<FEditableText>(Row.Editor);
                const FString Value = Row.Variable->GetString();
                if (!TextEditor->HasKeyboardFocus() && TextEditor->GetText() != Value)
                {
                    TextEditor->SetTextSilently(Value);
                }

                break;
            }
        }
    }

    if (ShaderCompilerStatus)
    {
        const FString Status = DescribeShaderCompilerStatus();
        if (ShaderCompilerStatus->GetText() != Status)
        {
            ShaderCompilerStatus->SetText(Status);
        }
    }
}
