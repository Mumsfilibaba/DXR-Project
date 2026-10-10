#include "ShaderCompiler/ShaderCompileJob.h"

FShaderCompileJob FShaderCompileJob::FromCompileInfo(const String& InSourceFile, const FShaderCompileInfo& CompileInfo)
{
    FShaderCompileJob Job;
    Job.SourceFile     = InSourceFile;
    Job.EntryPoint     = CompileInfo.EntryPoint;
    Job.ShaderModel    = CompileInfo.ShaderModel;
    Job.ShaderStage    = CompileInfo.ShaderStage;
    Job.OutputLanguage = CompileInfo.OutputLanguage;
    Job.bOptimize      = CompileInfo.bOptimize;
    Job.bDebugInfo     = CompileInfo.bDebugInfo;

    Job.Defines.Reserve(CompileInfo.Defines.Size());
    for (const FShaderDefine& Define : CompileInfo.Defines)
    {
        Job.Defines.Emplace(Define);
    }

    Job.IncludeDirs.Reserve(CompileInfo.IncludeDirs.Size());
    for (const String& IncludeDir : CompileInfo.IncludeDirs)
    {
        Job.IncludeDirs.Emplace(IncludeDir);
    }

    return Job;
}

FShaderCompileInfo FShaderCompileJob::ToCompileInfo()
{
    FShaderCompileInfo CompileInfo(EntryPoint, ShaderModel, ShaderStage, OutputLanguage, TArrayView<FShaderDefine>(Defines));
    CompileInfo.bOptimize   = bOptimize;
    CompileInfo.bDebugInfo  = bDebugInfo;
    CompileInfo.IncludeDirs = TArrayView<const String>(IncludeDirs);
    return CompileInfo;
}

uint64 FShaderCompileJob::GetKey() const
{
    uint64 Key = THash<String>::GetHash(SourceFile);
    HashCombine(Key, EntryPoint);
    HashCombine(Key, ShaderModel);
    HashCombine(Key, ShaderStage);
    HashCombine(Key, OutputLanguage);
    HashCombine(Key, bOptimize);
    HashCombine(Key, bDebugInfo);
    HashCombine(Key, bHasEngineDefines);

    for (const FShaderDefine& Define : Defines)
    {
        HashCombine(Key, Define.Define);
        HashCombine(Key, Define.Value);
    }

    for (const String& IncludeDir : IncludeDirs)
    {
        HashCombine(Key, IncludeDir);
    }

    return Key;
}

FJsonValue FShaderCompileJob::ToJson() const
{
    FJsonValue DefineArray = FJsonValue::MakeArray();
    for (const FShaderDefine& Define : Defines)
    {
        FJsonValue Pair = FJsonValue::MakeArray();
        Pair.Add(FJsonValue(Define.Define));
        Pair.Add(FJsonValue(Define.Value));
        DefineArray.Add(::Move(Pair));
    }

    FJsonValue Object = FJsonValue::MakeObject();
    Object.AddMember("name", FJsonValue(Name));
    Object.AddMember("source", FJsonValue(SourceFile));
    Object.AddMember("entry", FJsonValue(EntryPoint));
    Object.AddMember("stage", FJsonValue(ToString(ShaderStage)));
    Object.AddMember("model", FJsonValue(ToString(ShaderModel)));
    Object.AddMember("language", FJsonValue(ToString(OutputLanguage)));
    Object.AddMember("optimize", FJsonValue(bOptimize));
    Object.AddMember("debug", FJsonValue(bDebugInfo));
    Object.AddMember("engineDefines", FJsonValue(bHasEngineDefines));
    Object.AddMember("defines", ::Move(DefineArray));

    if (!IncludeDirs.IsEmpty())
    {
        FJsonValue IncludeDirArray = FJsonValue::MakeArray();
        for (const String& IncludeDir : IncludeDirs)
        {
            IncludeDirArray.Add(FJsonValue(IncludeDir));
        }

        Object.AddMember("includeDirs", ::Move(IncludeDirArray));
    }

    return Object;
}

static bool ReadStringMember(const FJsonValue& Object, const CHAR* Name, String& OutValue, String& OutError)
{
    const FJsonValue* Member = Object.Find(Name);
    if (!Member || !Member->TryGetString(OutValue))
    {
        OutError = String::Printf("'%s' must be a string", Name);
        return false;
    }

    return true;
}

bool FShaderCompileJob::FromJson(const FJsonValue& Value, FShaderCompileJob& OutJob, String& OutError)
{
    if (!Value.IsObject())
    {
        OutError = "A job must be an object";
        return false;
    }

    FShaderCompileJob Job;

    if (const FJsonValue* NameValue = Value.Find("name"))
    {
        Job.Name = NameValue->GetStringOr("");
    }

    String StageText;
    String ModelText;
    String LanguageText;

    if (!ReadStringMember(Value, "source", Job.SourceFile, OutError) ||
        !ReadStringMember(Value, "entry", Job.EntryPoint, OutError) ||
        !ReadStringMember(Value, "stage", StageText, OutError) ||
        !ReadStringMember(Value, "model", ModelText, OutError) ||
        !ReadStringMember(Value, "language", LanguageText, OutError))
    {
        return false;
    }

    if (Job.SourceFile.IsEmpty() || Job.EntryPoint.IsEmpty())
    {
        OutError = "'source' and 'entry' must not be empty";
        return false;
    }

    if (!TryParseShaderStage(StageText, Job.ShaderStage))
    {
        OutError = String::Printf("'%s' is not a shader stage", *StageText);
        return false;
    }

    if (!TryParseShaderModel(ModelText, Job.ShaderModel))
    {
        OutError = String::Printf("'%s' is not a shader model", *ModelText);
        return false;
    }

    if (!TryParseShaderOutputLanguage(LanguageText, Job.OutputLanguage))
    {
        OutError = String::Printf("'%s' is not an output language", *LanguageText);
        return false;
    }

    if (const FJsonValue* OptimizeValue = Value.Find("optimize"))
    {
        Job.bOptimize = OptimizeValue->GetBoolOr(true);
    }

    if (const FJsonValue* DebugValue = Value.Find("debug"))
    {
        Job.bDebugInfo = DebugValue->GetBoolOr(false);
    }

    if (const FJsonValue* EngineDefinesValue = Value.Find("engineDefines"))
    {
        Job.bHasEngineDefines = EngineDefinesValue->GetBoolOr(false);
    }

    if (const FJsonValue* DefinesValue = Value.Find("defines"))
    {
        if (!DefinesValue->IsArray())
        {
            OutError = "'defines' must be an array";
            return false;
        }

        for (int32 Index = 0; Index < DefinesValue->Num(); ++Index)
        {
            const FJsonValue& Pair = (*DefinesValue)[Index];

            String DefineName;
            String DefineValue;

            if (!Pair.IsArray() || Pair.Num() != 2 || !Pair[0].TryGetString(DefineName) || !Pair[1].TryGetString(DefineValue) || DefineName.IsEmpty())
            {
                OutError = "Every define must be a [name, value] pair of strings";
                return false;
            }

            Job.Defines.Emplace(DefineName, DefineValue);
        }
    }

    if (const FJsonValue* IncludeDirsValue = Value.Find("includeDirs"))
    {
        if (!IncludeDirsValue->IsArray())
        {
            OutError = "'includeDirs' must be an array";
            return false;
        }

        for (int32 Index = 0; Index < IncludeDirsValue->Num(); ++Index)
        {
            String IncludeDir;
            if (!(*IncludeDirsValue)[Index].TryGetString(IncludeDir) || IncludeDir.IsEmpty())
            {
                OutError = "Every include directory must be a non-empty string";
                return false;
            }

            Job.IncludeDirs.Emplace(::Move(IncludeDir));
        }
    }

    OutJob = ::Move(Job);
    return true;
}

bool TryParseShaderStage(const String& Text, EShaderStage& OutStage)
{
    for (uint8 Value = static_cast<uint8>(EShaderStage::Vertex); Value <= static_cast<uint8>(EShaderStage::RayCallable); ++Value)
    {
        const EShaderStage Stage = static_cast<EShaderStage>(Value);
        if (Text.Equals(ToString(Stage), EStringCaseType::NoCase))
        {
            OutStage = Stage;
            return true;
        }
    }

    return false;
}

bool TryParseShaderModel(const String& Text, EShaderModel& OutModel)
{
    constexpr EShaderModel ShaderModels[] =
    {
        EShaderModel::SM_5_0,
        EShaderModel::SM_6_0,
        EShaderModel::SM_6_1,
        EShaderModel::SM_6_2,
        EShaderModel::SM_6_3,
        EShaderModel::SM_6_4,
        EShaderModel::SM_6_5,
        EShaderModel::SM_6_6,
        EShaderModel::SM_6_7,
        EShaderModel::SM_6_8,
        EShaderModel::SM_6_9,
        EShaderModel::SM_6_10,
    };

    for (EShaderModel ShaderModel : ShaderModels)
    {
        if (Text.Equals(ToString(ShaderModel), EStringCaseType::NoCase))
        {
            OutModel = ShaderModel;
            return true;
        }
    }

    return false;
}

bool TryParseShaderOutputLanguage(const String& Text, EShaderOutputLanguage& OutOutputLanguage)
{
    constexpr EShaderOutputLanguage OutputLanguages[] =
    {
        EShaderOutputLanguage::DXIL,
        EShaderOutputLanguage::MSL,
        EShaderOutputLanguage::SPIRV,
        EShaderOutputLanguage::DXBC,
    };

    for (EShaderOutputLanguage OutputLanguage : OutputLanguages)
    {
        if (Text.Equals(ToString(OutputLanguage), EStringCaseType::NoCase))
        {
            OutOutputLanguage = OutputLanguage;
            return true;
        }
    }

    return false;
}
