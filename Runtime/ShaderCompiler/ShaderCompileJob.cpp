#include "ShaderCompiler/ShaderCompileJob.h"

FShaderCompileJob FShaderCompileJob::FromCompileInfo(const FString& InSourceFile, const FShaderCompileInfo& CompileInfo)
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
    for (const FString& IncludeDir : CompileInfo.IncludeDirs)
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
    CompileInfo.IncludeDirs = TArrayView<const FString>(IncludeDirs);
    return CompileInfo;
}

uint64 FShaderCompileJob::GetKey() const
{
    uint64 Key = THash<FString>::GetHash(SourceFile);
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

    for (const FString& IncludeDir : IncludeDirs)
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
    Object.AddMember(TEXT("name"), FJsonValue(Name));
    Object.AddMember(TEXT("source"), FJsonValue(SourceFile));
    Object.AddMember(TEXT("entry"), FJsonValue(EntryPoint));
    Object.AddMember(TEXT("stage"), FJsonValue(ToString(ShaderStage)));
    Object.AddMember(TEXT("model"), FJsonValue(ToString(ShaderModel)));
    Object.AddMember(TEXT("language"), FJsonValue(ToString(OutputLanguage)));
    Object.AddMember(TEXT("optimize"), FJsonValue(bOptimize));
    Object.AddMember(TEXT("debug"), FJsonValue(bDebugInfo));
    Object.AddMember(TEXT("engineDefines"), FJsonValue(bHasEngineDefines));
    Object.AddMember(TEXT("defines"), ::Move(DefineArray));

    if (!IncludeDirs.IsEmpty())
    {
        FJsonValue IncludeDirArray = FJsonValue::MakeArray();
        for (const FString& IncludeDir : IncludeDirs)
        {
            IncludeDirArray.Add(FJsonValue(IncludeDir));
        }

        Object.AddMember(TEXT("includeDirs"), ::Move(IncludeDirArray));
    }

    return Object;
}

static bool ReadStringMember(const FJsonValue& Object, const CHAR_T* Name, FString& OutValue, FString& OutError)
{
    const FJsonValue* Member = Object.Find(Name);
    if (!Member || !Member->TryGetString(OutValue))
    {
        OutError = FString::Printf(TEXT("'%s' must be a string"), Name);
        return false;
    }

    return true;
}

bool FShaderCompileJob::FromJson(const FJsonValue& Value, FShaderCompileJob& OutJob, FString& OutError)
{
    if (!Value.IsObject())
    {
        OutError = TEXT("A job must be an object");
        return false;
    }

    FShaderCompileJob Job;

    if (const FJsonValue* NameValue = Value.Find(TEXT("name")))
    {
        Job.Name = NameValue->GetStringOr(TEXT(""));
    }

    FString StageText;
    FString ModelText;
    FString LanguageText;

    if (!ReadStringMember(Value, TEXT("source"), Job.SourceFile, OutError) ||
        !ReadStringMember(Value, TEXT("entry"), Job.EntryPoint, OutError) ||
        !ReadStringMember(Value, TEXT("stage"), StageText, OutError) ||
        !ReadStringMember(Value, TEXT("model"), ModelText, OutError) ||
        !ReadStringMember(Value, TEXT("language"), LanguageText, OutError))
    {
        return false;
    }

    if (Job.SourceFile.IsEmpty() || Job.EntryPoint.IsEmpty())
    {
        OutError = TEXT("'source' and 'entry' must not be empty");
        return false;
    }

    if (!TryParseShaderStage(StageText, Job.ShaderStage))
    {
        OutError = FString::Printf(TEXT("'%s' is not a shader stage"), *StageText);
        return false;
    }

    if (!TryParseShaderModel(ModelText, Job.ShaderModel))
    {
        OutError = FString::Printf(TEXT("'%s' is not a shader model"), *ModelText);
        return false;
    }

    if (!TryParseShaderOutputLanguage(LanguageText, Job.OutputLanguage))
    {
        OutError = FString::Printf(TEXT("'%s' is not an output language"), *LanguageText);
        return false;
    }

    if (const FJsonValue* OptimizeValue = Value.Find(TEXT("optimize")))
    {
        Job.bOptimize = OptimizeValue->GetBoolOr(true);
    }

    if (const FJsonValue* DebugValue = Value.Find(TEXT("debug")))
    {
        Job.bDebugInfo = DebugValue->GetBoolOr(false);
    }

    if (const FJsonValue* EngineDefinesValue = Value.Find(TEXT("engineDefines")))
    {
        Job.bHasEngineDefines = EngineDefinesValue->GetBoolOr(false);
    }

    if (const FJsonValue* DefinesValue = Value.Find(TEXT("defines")))
    {
        if (!DefinesValue->IsArray())
        {
            OutError = TEXT("'defines' must be an array");
            return false;
        }

        for (int32 Index = 0; Index < DefinesValue->Num(); ++Index)
        {
            const FJsonValue& Pair = (*DefinesValue)[Index];

            FString DefineName;
            FString DefineValue;

            if (!Pair.IsArray() || Pair.Num() != 2 || !Pair[0].TryGetString(DefineName) || !Pair[1].TryGetString(DefineValue) || DefineName.IsEmpty())
            {
                OutError = TEXT("Every define must be a [name, value] pair of strings");
                return false;
            }

            Job.Defines.Emplace(DefineName, DefineValue);
        }
    }

    if (const FJsonValue* IncludeDirsValue = Value.Find(TEXT("includeDirs")))
    {
        if (!IncludeDirsValue->IsArray())
        {
            OutError = TEXT("'includeDirs' must be an array");
            return false;
        }

        for (int32 Index = 0; Index < IncludeDirsValue->Num(); ++Index)
        {
            FString IncludeDir;
            if (!(*IncludeDirsValue)[Index].TryGetString(IncludeDir) || IncludeDir.IsEmpty())
            {
                OutError = TEXT("Every include directory must be a non-empty string");
                return false;
            }

            Job.IncludeDirs.Emplace(::Move(IncludeDir));
        }
    }

    OutJob = ::Move(Job);
    return true;
}

bool TryParseShaderStage(const FString& Text, EShaderStage& OutStage)
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

bool TryParseShaderModel(const FString& Text, EShaderModel& OutModel)
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

bool TryParseShaderOutputLanguage(const FString& Text, EShaderOutputLanguage& OutOutputLanguage)
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
