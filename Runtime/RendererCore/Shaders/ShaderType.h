#pragma once
#include "RendererCore/Shaders/ShaderPermutation.h"
#include "RHI/RHIShader.h"

template<typename ShaderType>
concept CShaderTypeHasShouldCompile = requires(const FShaderPermutationDesc& Desc)
{
    ShaderType::ShouldCompilePermutation(Desc);
};

template<typename ShaderType>
concept CShaderTypeHasModifyEnvironment = requires(const FShaderPermutationDesc& Desc, FShaderCompilationEnvironment& Environment)
{
    ShaderType::ModifyCompilationEnvironment(Desc, Environment);
};

template<typename ShaderType>
concept CShaderTypeHasRemapPermutation = requires(typename ShaderType::FPermutation Permutation)
{
    ShaderType::RemapPermutation(Permutation);
};

class RENDERERCORE_API FShaderType
{
public:
    NODISCARD static FShaderType* GetTypeList();
    NODISCARD static FShaderType* FindByName(const CHAR_T* InName);

public:
    using FModifyEnvFunc     = void (*)(const FShaderPermutationDesc&, FShaderCompilationEnvironment&);
    using FBuildEnvFunc      = void (*)(int32 PermutationID, FShaderCompilationEnvironment&);
    using FShouldCompileFunc = bool (*)(const FShaderPermutationDesc&);

    FShaderType(const CHAR_T* InName, const CHAR_T* InSourceFile, const CHAR_T* InEntryPoint, EShaderStage InStage, EShaderModel InMinShaderModel,
        int32 InPermutationCount, FBuildEnvFunc InBuildEnvironment, FShouldCompileFunc InShouldCompile, FModifyEnvFunc InModifyEnvironment);
    ~FShaderType();

    void BuildCompilationEnvironment(const FShaderPermutationDesc& Desc, FShaderCompilationEnvironment& OutEnvironment) const;

    NODISCARD bool   ShouldCompilePermutation(const FShaderPermutationDesc& Desc) const;
    NODISCARD uint64 GetPermutationSpaceSignature() const;

    NODISCARD FShaderType*  GetNext() const             { return Next; }
    NODISCARD const CHAR_T* GetName() const             { return Name; }
    NODISCARD const CHAR_T* GetSourceFile() const       { return SourceFile; }
    NODISCARD const CHAR_T* GetEntryPoint() const       { return EntryPoint; }
    NODISCARD EShaderStage  GetStage() const            { return Stage; }
    NODISCARD EShaderModel  GetMinShaderModel() const   { return MinShaderModel; }
    NODISCARD int32         GetPermutationCount() const { return PermutationCount; }

private:
    const CHAR_T*      Name;
    const CHAR_T*      SourceFile;
    const CHAR_T*      EntryPoint;
    EShaderStage       Stage;
    EShaderModel       MinShaderModel;
    int32              PermutationCount;
    FBuildEnvFunc      BuildEnvironment;
    FShouldCompileFunc ShouldCompile;
    FModifyEnvFunc     ModifyEnvironment;
    FShaderType*       Next;
};

template<typename ShaderType>
struct TShaderTypeHelpers
{
    static void BuildEnvironment(int32 PermutationID, FShaderCompilationEnvironment& OutEnvironment)
    {
        using FPermutation = typename ShaderType::FPermutation;

        const FPermutation Permutation = FPermutation(PermutationID);
        Permutation.ModifyCompilationEnvironment(OutEnvironment);
    }

    static bool ShouldCompile(const FShaderPermutationDesc& Desc)
    {
        if constexpr (CShaderTypeHasShouldCompile<ShaderType>)
        {
            return ShaderType::ShouldCompilePermutation(Desc);
        }
        else
        {
            return true;
        }
    }

    static void ModifyEnvironment(const FShaderPermutationDesc& Desc, FShaderCompilationEnvironment& OutEnvironment)
    {
        if constexpr (CShaderTypeHasModifyEnvironment<ShaderType>)
        {
            ShaderType::ModifyCompilationEnvironment(Desc, OutEnvironment);
        }
    }
};

#define DECLARE_SHADER_TYPE(ShaderClass, InStage)      \
    public:                                            \
        static constexpr EShaderStage Stage = InStage; \
        static FShaderType& GetStaticType();           \
    public:

#define IMPLEMENT_SHADER_TYPE(ShaderClass, InSourceFile, InEntryPoint, InMinShaderModel) \
    static FShaderType GShaderType##ShaderClass(                                         \
        #ShaderClass,                                                                    \
        InSourceFile,                                                                    \
        InEntryPoint,                                                                    \
        ShaderClass::Stage,                                                              \
        InMinShaderModel,                                                                \
        ShaderClass::FPermutation::PermutationCount,                                     \
        &TShaderTypeHelpers<ShaderClass>::BuildEnvironment,                              \
        &TShaderTypeHelpers<ShaderClass>::ShouldCompile,                                 \
        &TShaderTypeHelpers<ShaderClass>::ModifyEnvironment);                            \
                                                                                         \
    FShaderType& ShaderClass::GetStaticType()                                            \
    {                                                                                    \
        return GShaderType##ShaderClass;                                                 \
    }
