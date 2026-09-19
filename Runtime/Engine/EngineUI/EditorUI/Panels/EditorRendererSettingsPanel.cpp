#include "Engine/EngineUI/EditorUI/Panels/EditorRendererSettingsPanel.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/CheckBox.h"
#include "Application/Elements/Expander.h"
#include "Application/Elements/NumericEntry.h"
#include "Application/Elements/PropertyTable.h"
#include "Application/Elements/ScrollBox.h"
#include "Application/Elements/SearchBox.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Menus/ComboBox.h"

static const CHAR* const GCascadeResolutions[]      = { "512", "1024", "2048", "4096" };
static const int32       GCascadeResolutionValues[] = { 512, 1024, 2048, 4096 };

static const CHAR* const GPointResolutions[]      = { "128", "256", "512", "1024" };
static const int32       GPointResolutionValues[] = { 128, 256, 512, 1024 };

static const CHAR* const GSampleCounts[]      = { "16", "32", "64", "128" };
static const int32       GSampleCountValues[] = { 16, 32, 64, 128 };

static const CHAR* const GFilterModes[]        = { "PCF", "PCSS" };
static const CHAR* const GFilterFunctions[]    = { "Grid", "Poisson Disk", "Vogel Disk" };
static const CHAR* const GReflectionSamplers[] = { "White noise", "Halton", "Blue noise" };
static const CHAR* const GTonemappers[]        = { "Default", "ACES", "Reinhard", "Uncharted 2" };

// The section whose rows the capability line above them explains
static const CHAR* const GRayTracingSection = "Ray Tracing";

static String DescribeRayTracingSupport()
{
    if (!RHI::bSupportsRayTracing)
    {
        return "Not supported";
    }

    return String::Printf("Tier %s, inline %s, SER %s",
        ToString(RHI::RayTracingTier),
        RHI::bSupportsInlineRayTracing ? "yes" : "no",
        RHI::bSupportsShaderExecutionReordering ? "yes" : "no");
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

static const FRendererSetting GRendererSettings[] =
{
    SETTING_BOOL("Deferred Rendering", "Draw tile-debug",   "Renderer.Debug.DrawTiledLightning"),
    SETTING_BOOL("Deferred Rendering", "Clear all targets", "Renderer.BasePass.ClearAllTargets"),
    SETTING_BOOL("Deferred Rendering", "Enable pre-pass",   "Renderer.Feature.PrePass"),
    SETTING_BOOL("Deferred Rendering", "Enable base-pass",  "Renderer.Feature.BasePass"),

    SETTING_BOOL("Shadows", "Enable shadows", "Renderer.Feature.Shadows"),

    SETTING_BOOL        ("Cascaded Shadow Maps", "Enable sun shadows",             "Renderer.Feature.SunShadows"),
    SETTING_BOOL        ("Cascaded Shadow Maps", "Enable shadow mask",             "Renderer.Feature.ShadowMask"),
    SETTING_BOOL        ("Cascaded Shadow Maps", "Tight frustum (depth reduce)",   "Renderer.CSM.TightFrustum"),
    SETTING_COMBO_VALUES("Cascaded Shadow Maps", "Cascade resolution",             "Renderer.CSM.CascadeSize", GCascadeResolutions, GCascadeResolutionValues),
    SETTING_BOOL        ("Cascaded Shadow Maps", "Stable cascades",                "Renderer.CSM.StableCascades"),
    SETTING_BOOL        ("Cascaded Shadow Maps", "Single-pass rendering",          "Renderer.CSM.EnableSinglePassRendering"),
    SETTING_BOOL        ("Cascaded Shadow Maps", "Geometry shader instancing",     "Renderer.CSM.EnableGeometryShaderInstancing"),
    SETTING_BOOL        ("Cascaded Shadow Maps", "View instancing",                "Renderer.CSM.EnableViewInstancing"),
    SETTING_BOOL        ("Cascaded Shadow Maps", "Depth clipping",                 "Renderer.CSM.EnableDepthClipping"),
    SETTING_BOOL        ("Cascaded Shadow Maps", "Blend cascades",                 "Renderer.CSM.BlendCascades"),
    SETTING_BOOL        ("Cascaded Shadow Maps", "Select cascade from projection", "Renderer.CSM.SelectCascadeFromProjection"),
    SETTING_COMBO       ("Cascaded Shadow Maps", "Filter mode",                    "Renderer.CSM.FilterMode", GFilterModes),
    SETTING_COMBO       ("Cascaded Shadow Maps", "Filter function",                "Renderer.CSM.FilterFunction", GFilterFunctions),
    SETTING_INT         ("Cascaded Shadow Maps", "Filter size",                    "Renderer.CSM.FilterSize", 1, 2048),
    SETTING_INT         ("Cascaded Shadow Maps", "Max filter size",                "Renderer.CSM.MaxFilterSize", 1, 2048),
    SETTING_COMBO_VALUES("Cascaded Shadow Maps", "Sample count",                   "Renderer.CSM.NumPoissonDiscSamples", GSampleCounts, GSampleCountValues),
    SETTING_BOOL        ("Cascaded Shadow Maps", "Rotate samples",                 "Renderer.CSM.RotateSamples"),

    SETTING_BOOL        ("Point-light Shadow Maps", "Enable point-light shadows", "Renderer.Feature.PointLightShadows"),
    SETTING_COMBO_VALUES("Point-light Shadow Maps", "Shadow map resolution",      "Renderer.Shadows.PointLightShadowMapSize", GPointResolutions, GPointResolutionValues),
    SETTING_BOOL        ("Point-light Shadow Maps", "Single-pass rendering",      "Renderer.PointLights.EnableSinglePassRendering"),
    SETTING_BOOL        ("Point-light Shadow Maps", "Geometry shader instancing", "Renderer.PointLights.EnableGeometryShaderInstancing"),

    SETTING_BOOL("Skybox", "Enable Skybox",       "Renderer.Feature.Skybox"),
    SETTING_BOOL("Skybox", "Clear before skybox", "Renderer.Skybox.ClearBeforeSkybox"),

    SETTING_BOOL ("SSAO", "Enable SSAO", "Renderer.Feature.SSAO"),
    SETTING_INT  ("SSAO", "Kernel size", "Renderer.SSAO.KernelSize", 1, 128),
    SETTING_FLOAT("SSAO", "Radius",      "Renderer.SSAO.Radius", 0.01f, 1.0f, 0.01f),
    SETTING_FLOAT("SSAO", "Bias",        "Renderer.SSAO.Bias", 0.01f, 1.0f, 0.01f),

    SETTING_BOOL_GATED("Ray Tracing", "Enable ray tracing",                "Renderer.Feature.RayTracing",
        []() { return RHI::bSupportsRayTracing; }),
    SETTING_BOOL_GATED("Ray Tracing", "Use local shader bindings",         "Renderer.RayTracing.EnableLocalShaderBindings",
        []() { return RHI::bSupportsRayTracing; }),
    SETTING_BOOL_GATED("Ray Tracing", "Inline reflections (RayQuery)",     "Renderer.RayTracing.InlineReflections",
        []() { return RHI::bSupportsInlineRayTracing; }),
    SETTING_BOOL_GATED("Ray Tracing", "Shader Execution Reordering",       "Renderer.RayTracing.SER",
        []() { return RHI::bSupportsShaderExecutionReordering; }),
    SETTING_BOOL_GATED("Ray Tracing", "BLAS compaction",                   "Renderer.RayTracing.Compaction",
        []() { return RHI::bSupportsRayTracing; }),
    SETTING_BOOL_GATED("Ray Tracing", "Acceleration-structure disk cache", "Renderer.RayTracing.ASCache",
        []() { return RHI::bSupportsRayTracing; }),

    SETTING_BOOL ("Reflections", "Enable",                       "Renderer.RayTracing.Reflections.Enable"),
    SETTING_FLOAT("Reflections", "Indirect specular strength",   "Renderer.Reflections.IndirectSpecularStrength", 0.0f, 4.0f, 0.01f),
    SETTING_BOOL ("Reflections", "Half resolution",              "Renderer.RayTracing.Reflections.HalfRes"),
    SETTING_FLOAT("Reflections", "Max ray distance",             "Renderer.RayTracing.Reflections.MaxRayDistance", 1.0f, 100000.0f, 10.0f),
    SETTING_FLOAT("Reflections", "Mirror roughness threshold",   "Renderer.RayTracing.Reflections.MirrorRoughnessThreshold", 0.0f, 1.0f, 0.001f),
    SETTING_FLOAT("Reflections", "Ray bias",                     "Renderer.RayTracing.Reflections.RayBias", 0.0f, 1.0f, 0.001f),
    SETTING_COMBO("Reflections", "GGX sampler",                  "Renderer.RayTracing.Reflections.Sampler", GReflectionSamplers),
    SETTING_BOOL ("Reflections", "Enable denoiser",              "Renderer.RayTracing.Reflections.Denoise"),
    SETTING_FLOAT("Reflections", "Temporal alpha",               "Renderer.RayTracing.Reflections.TemporalAlpha", 0.01f, 1.0f, 0.01f),
    SETTING_FLOAT("Reflections", "Max radiance (firefly clamp)", "Renderer.RayTracing.Reflections.MaxRadiance", 0.0f, 100.0f, 0.1f),
    SETTING_FLOAT("Reflections", "History clamp gamma",          "Renderer.RayTracing.Reflections.HistoryClampGamma", 0.0f, 10.0f, 0.01f),
    SETTING_FLOAT("Reflections", "Max history length",           "Renderer.RayTracing.Reflections.MaxHistoryLength", 1.0f, 128.0f, 1.0f),
    SETTING_INT  ("Reflections", "Neighborhood radius",          "Renderer.RayTracing.Reflections.NeighborhoodRadius", 0, 8),
    SETTING_FLOAT("Reflections", "Camera-motion max history",    "Renderer.RayTracing.Reflections.CameraMotionMaxHistory", 0.0f, 64.0f, 0.5f),
    SETTING_INT  ("Reflections", "A-trous iterations",           "Renderer.RayTracing.Reflections.AtrousIterations", 0, 8),
    SETTING_FLOAT("Reflections", "A-trous color phi",            "Renderer.RayTracing.Reflections.AtrousPhiColor", 0.1f, 32.0f, 0.1f),

    SETTING_BOOL("Temporal Anti-aliasing (TAA)", "Enabled",         "Renderer.Feature.TemporalAntiAliasing"),
    SETTING_BOOL("Temporal Anti-aliasing (TAA)", "Hardware jitter", "Renderer.TemporalAntiAliasing.HardwareJitter"),

    SETTING_BOOL("FXAA", "Enable FXAA",       "Renderer.Feature.FXAA"),
    SETTING_BOOL("FXAA", "Enable FXAA debug", "Renderer.Debug.FXAADebug"),

    SETTING_COMBO("Tonemapping", "Tonemapping function", "Renderer.Tonemapping.Function", GTonemappers),
    SETTING_FLOAT("Tonemapping", "Exposure (EV100)",     "Renderer.Tonemapping.EV100", -10.0f, 20.0f, 0.1f),
    SETTING_FLOAT("Tonemapping", "Reinhard intensity",   "Renderer.Tonemapping.ReinhardIntensity", 0.1f, 10.0f, 0.01f),

    SETTING_BOOL("Display", "Enable VSync", "Renderer.Feature.VerticalSync"),

    SETTING_BOOL("Culling", "Enable frustum-culling", "Renderer.Feature.FrustumCulling"),

    SETTING_BOOL("Debug", "Enable debug-draw AABBs",        "Renderer.Debug.DrawAABBs"),
    SETTING_BOOL("Debug", "Enable debug-draw point-lights", "Renderer.Debug.DrawPointLights"),
    SETTING_BOOL("Debug", "Enable debug-draw light-probes", "Renderer.Debug.LightProbes"),
};

#undef SETTING_BOOL
#undef SETTING_INT
#undef SETTING_FLOAT
#undef SETTING_COMBO
#undef SETTING_COMBO_VALUES

static const FRendererSubsection GRendererSubsections[] =
{
    { "Cascaded Shadow Maps",    "Shadows"     },
    { "Point-light Shadow Maps", "Shadows"     },
    { "Reflections",             "Ray Tracing" },
};

constexpr float RENDERER_SETTINGS_LABEL_FRACTION = 0.62f;

static const CHAR* FindParentSection(const CHAR* Section)
{
    for (const FRendererSubsection& Subsection : GRendererSubsections)
    {
        if (String(Subsection.Section) == Section)
        {
            return Subsection.ParentSection;
        }
    }

    return nullptr;
}

FEditorRendererSettingsPanel::FEditorRendererSettingsPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, "RendererSettings", "Renderer Settings")
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

    SearchBox = FSearchBox::Create(FEditorStyle::MakeSearchBoxDesc("Search Settings",
        FOnSearchTextChanged::CreateRaw(this, &FEditorRendererSettingsPanel::OnSearchTextChanged)));


    TSharedPtr<FVerticalBox> Layout = FVerticalBox::Create();
    Layout->AddSlot(SearchBox).SetPadding(FMargin(0, 0, 0, FEditorStyle::ItemSpacing));
    Layout->AddSlot(FEditorStyle::MakeInnerFrame(ScrollBox)).SetFillCoefficient(1.0f);

    Content = Layout;

    RebuildSections();
    return true;
}

void FEditorRendererSettingsPanel::Release()
{
    ScrollBox.Reset();
    Column.Reset();
    SearchBox.Reset();
    Rows.Clear();

    FEditorPanel::Release();
}

bool FEditorRendererSettingsPanel::PassesFilter(const FRendererSetting& Setting) const
{
    if (FilterText.IsEmpty())
    {
        return true;
    }

    return String(Setting.Label).Contains(FilterText, EStringCaseType::NoCase)
        || String(Setting.CVarName).Contains(FilterText, EStringCaseType::NoCase);
}

void FEditorRendererSettingsPanel::RebuildSections()
{
    Column->ClearSlots();
    Rows.Clear();

    FConsoleManager& ConsoleManager = FConsoleManager::Get();

    struct FSectionBox
    {
        const CHAR*              Name;
        TSharedPtr<FVerticalBox> Box;
    };

    const bool bStartExpanded = !FilterText.IsEmpty();
    TArray<FSectionBox> SectionBoxes;

    const CHAR*                CurrentSection = nullptr;
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

        if (!CurrentSection || String(CurrentSection) != Setting.Section)
        {
            CurrentTable   = FPropertyTable::Create(FEditorStyle::MakePropertyTableDesc(RENDERER_SETTINGS_LABEL_FRACTION));
            CurrentSection = Setting.Section;

            TSharedPtr<FVerticalBox> SectionBox = FVerticalBox::Create();
            SectionBox->AddSlot(CurrentTable);

            TSharedPtr<FExpander> Section = FExpander::Create(FEditorStyle::MakeExpanderDesc(Setting.Section, SectionBox, bStartExpanded));

            // A subsection is content of the section above it, so it indents and collapses with it
            const CHAR*              ParentName = FindParentSection(Setting.Section);
            TSharedPtr<FVerticalBox> ParentBox  = nullptr;

            for (const FSectionBox& Candidate : SectionBoxes)
            {
                if (ParentName && String(Candidate.Name) == ParentName)
                {
                    ParentBox = Candidate.Box;
                    break;
                }
            }

            if (ParentBox)
            {
                ParentBox->AddSlot(Section).SetPadding(FEditorStyle::GetSectionSpacing());
            }
            else
            {
                Column->AddSlot(Section).SetPadding(FEditorStyle::GetSectionSpacing());
            }

            SectionBoxes.Emplace(FSectionBox{ Setting.Section, SectionBox });

            if (String(Setting.Section) == GRayTracingSection)
            {
                FTextBlock::FDesc SupportDesc;
                SupportDesc.Text = DescribeRayTracingSupport();
                SupportDesc.Font = FEditorStyle::GetFonts().Monospace;

                CurrentTable->AddRow("Hardware support", FTextBlock::Create(SupportDesc)).ToolTipText =
                    "What this RHI reports, which is what decides whether the rows below it do anything";
            }
        }

        const bool bIsSupported = !Setting.IsSupported || Setting.IsSupported();

        FPropertyRow& Row = CurrentTable->AddRow(Setting.Label, Editor);
        Row.ToolTipText   = bIsSupported ? String(Setting.CVarName) : String::Printf("%s (not supported by this RHI)", Setting.CVarName);

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

            TArray<String> Options;
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
    }

    return nullptr;
}

void FEditorRendererSettingsPanel::OnSearchTextChanged(const String& SearchText)
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
        }
    }
}
