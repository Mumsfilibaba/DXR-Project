#include "Core/Templates/CString.h"
#include "RendererCore/Shaders/ShaderType.h"

static FShaderType* GShaderTypeList = nullptr;

FShaderType* FShaderType::GetTypeList()
{
    return GShaderTypeList;
}

FShaderType* FShaderType::FindByName(const CHAR* InName)
{
    for (FShaderType* Type = GShaderTypeList; Type; Type = Type->Next)
    {
        if (CString::Strcmp(Type->Name, InName) == 0)
        {
            return Type;
        }
    }

    return nullptr;
}

FShaderType::FShaderType(const CHAR* InName, const CHAR* InSourceFile, const CHAR* InEntryPoint, EShaderStage InStage, EShaderModel InMinShaderModel,
    int32 InPermutationCount, FBuildEnvFunc InBuildEnvironment, FShouldCompileFunc InShouldCompile, FModifyEnvFunc InModifyEnvironment)
    : Name(InName)
    , SourceFile(InSourceFile)
    , EntryPoint(InEntryPoint)
    , Stage(InStage)
    , MinShaderModel(InMinShaderModel)
    , PermutationCount(InPermutationCount)
    , BuildEnvironment(InBuildEnvironment)
    , ShouldCompile(InShouldCompile)
    , ModifyEnvironment(InModifyEnvironment)
    , Next(GShaderTypeList)
{
    GShaderTypeList = this;
}

FShaderType::~FShaderType() = default;

bool FShaderType::ShouldCompilePermutation(const FShaderPermutationDesc& Desc) const
{
    return ShouldCompile ? ShouldCompile(Desc) : true;
}

void FShaderType::BuildCompilationEnvironment(const FShaderPermutationDesc& Desc, FShaderCompilationEnvironment& OutEnvironment) const
{
    OutEnvironment.RequireShaderModel(MinShaderModel);

    if (BuildEnvironment)
    {
        BuildEnvironment(Desc.PermutationID, OutEnvironment);
    }

    if (ModifyEnvironment)
    {
        ModifyEnvironment(Desc, OutEnvironment);
    }
}

uint64 FShaderType::GetPermutationSpaceSignature() const
{
    // The dimensions emit their defines head-to-tail, so the order they arrive in is the dimension order.
    FShaderCompilationEnvironment Environment;
    BuildCompilationEnvironment(FShaderPermutationDesc(), Environment);

    uint64 Signature = static_cast<uint64>(PermutationCount);
    for (const FShaderDefine& Define : Environment.Defines)
    {
        HashCombine(Signature, Define.Define);
    }

    return Signature;
}
