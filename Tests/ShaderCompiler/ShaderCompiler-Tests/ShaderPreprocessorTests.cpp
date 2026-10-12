#include "ShaderPreprocessorTests.h"

#include <Core/Misc/Paths.h>
#include <ShaderCompiler/ShaderPreprocessor.h>
#include <ShaderCompiler/FXC/FXCShaderTranslator.h>

#include "TestCommon/TestMacros.h"

static FString JoinTokens(const FShaderPreprocessorOutput& Output)
{
    FString Result;
    for (const FShaderToken& Token : Output.Tokens)
    {
        if (!Result.IsEmpty())
        {
            Result += ' ';
        }

        Result += Token.Text;
    }

    return Result;
}

static bool Preprocess(const CHAR_T* Source, FShaderPreprocessorOutput& OutOutput, const CHAR_T* DefineName = nullptr, const CHAR_T* DefineValue = nullptr)
{
    FShaderPreprocessor Preprocessor(Paths::GetAssetDir() + TEXT("/Shaders"));
    if (DefineName)
    {
        Preprocessor.AddDefine(DefineName, DefineValue);
    }

    return Preprocessor.Preprocess(FString(), FStringView(Source), OutOutput);
}

static FString PreprocessToText(const CHAR_T* Source, const CHAR_T* DefineName = nullptr, const CHAR_T* DefineValue = nullptr)
{
    FShaderPreprocessorOutput Output;
    if (!Preprocess(Source, Output, DefineName, DefineValue))
    {
        return FString(TEXT("<failed> ")) + Output.Errors;
    }

    return JoinTokens(Output);
}

static FString TranslateToText(const CHAR_T* Source, bool* bOutSucceeded = nullptr)
{
    FShaderPreprocessorOutput Output;
    const bool bSucceeded = Preprocess(Source, Output) && FFXCShaderTranslator::Translate(Output);
    if (bOutSucceeded)
    {
        *bOutSucceeded = bSucceeded;
    }

    return bSucceeded ? JoinTokens(Output) : Output.Errors;
}

bool ShaderPreprocessorMacros_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("Object-like macros"));
    TEST_EXPECT(PreprocessToText(TEXT("#define A 1\nA + A")) == TEXT("1 + 1"));
    TEST_EXPECT(PreprocessToText(TEXT("#define EMPTY\na EMPTY b")) == TEXT("a b"));

    TEST_SECTION(TEXT("Function-like macros expand their arguments first"));
    TEST_EXPECT(PreprocessToText(TEXT("#define ADD(a, b) ((a) + (b))\n#define TWO 2\nADD(TWO, 3)")) == TEXT("( ( 2 ) + ( 3 ) )"));
    TEST_EXPECT(PreprocessToText(TEXT("#define CLAUSE(Register) register(Register)\n#define SLOT t4\nCLAUSE(SLOT)")) == TEXT("register ( t4 )"));
    TEST_EXPECT(PreprocessToText(TEXT("#define F(x) x\nF((1, 2))")) == TEXT("( 1 , 2 )"));

    TEST_SECTION(TEXT("A function-like macro without parentheses is left alone"));
    TEST_EXPECT(PreprocessToText(TEXT("#define F(x) x\nfloat F;")) == TEXT("float F ;"));

    TEST_SECTION(TEXT("Rescanning continues into the tokens after the expansion"));
    TEST_EXPECT(PreprocessToText(TEXT("#define F G\n#define G(x) x * 2\nF(4)")) == TEXT("4 * 2"));

    TEST_SECTION(TEXT("A macro is never expanded inside its own expansion"));
    TEST_EXPECT(PreprocessToText(TEXT("#define A A + 1\nA")) == TEXT("A + 1"));
    TEST_EXPECT(PreprocessToText(TEXT("#define A B\n#define B A\nA B")) == TEXT("A B"));

    TEST_SECTION(TEXT("Invocations can span lines"));
    TEST_EXPECT(PreprocessToText(TEXT("#define SUM(a, b) a + b\nSUM(1,\n2)")) == TEXT("1 + 2"));

    TEST_SECTION(TEXT("Token pasting and stringizing"));
    TEST_EXPECT(PreprocessToText(TEXT("#define CAT(a, b) a##b\nCAT(Foo, Bar)")) == TEXT("FooBar"));
    TEST_EXPECT(PreprocessToText(TEXT("#define CAT(a, b) a##b\nCAT(, Bar)")) == TEXT("Bar"));
    TEST_EXPECT(PreprocessToText(TEXT("#define STR(x) #x\nSTR(a + b)")) == TEXT("\"a + b\""));

    TEST_SECTION(TEXT("Line continuations and comments"));
    TEST_EXPECT(PreprocessToText(TEXT("#define LONG a \\\n + b // trailing\n/* multi\nline */ LONG")) == TEXT("a + b"));

    TEST_SECTION(TEXT("Defines added by the compiler"));
    TEST_EXPECT(PreprocessToText(TEXT("VALUE"), TEXT("VALUE"), TEXT("(3)")) == TEXT("( 3 )"));
    TEST_EXPECT(PreprocessToText(TEXT("#undef VALUE\nVALUE"), TEXT("VALUE"), TEXT("(3)")) == TEXT("VALUE"));

    TEST_END();
}

bool ShaderPreprocessorConditionals_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("#if evaluates shifts, which FXC's own preprocessor cannot"));
    TEST_EXPECT(PreprocessToText(TEXT("#define FLAG (1 << 2)\n#if (FLAG & 4) != 0\nyes\n#else\nno\n#endif")) == TEXT("yes"));

    TEST_SECTION(TEXT("#elif, defined and logical operators"));
    TEST_EXPECT(PreprocessToText(TEXT("#if defined(X)\na\n#elif !defined Y && 3 > 2\nb\n#else\nc\n#endif")) == TEXT("b"));
    TEST_EXPECT(PreprocessToText(TEXT("#define X\n#ifdef X\na\n#endif\n#ifndef X\nb\n#endif")) == TEXT("a"));

    TEST_SECTION(TEXT("Undefined names are zero and the unevaluated side can divide by zero"));
    TEST_EXPECT(PreprocessToText(TEXT("#if UNDEFINED || 1\na\n#endif")) == TEXT("a"));
    TEST_EXPECT(PreprocessToText(TEXT("#if 0 && (1 / 0)\na\n#else\nb\n#endif")) == TEXT("b"));
    TEST_EXPECT(PreprocessToText(TEXT("#if 1 ? 0x10 == 16 : 0\na\n#endif")) == TEXT("a"));

    TEST_SECTION(TEXT("Directives in skipped blocks are not evaluated"));
    TEST_EXPECT(PreprocessToText(TEXT("#ifdef NOPE\n#if 1 / 0\n#error Unreachable\n#endif\n#endif\nok")) == TEXT("ok"));

    TEST_SECTION(TEXT("Values from the compiler defines"));
    TEST_EXPECT(PreprocessToText(TEXT("#if VALUE == 3\nthree\n#endif"), TEXT("VALUE"), TEXT("(3)")) == TEXT("three"));

    TEST_END();
}

bool ShaderPreprocessorIncludes_Test()
{
    TEST_BEGIN();

    const FString FilePath = Paths::GetAssetDir() + TEXT("/Shaders/GenerateMipsTex2D.hlsl");

    FShaderPreprocessor Preprocessor(Paths::GetAssetDir() + TEXT("/Shaders"));
    Preprocessor.AddDefine(TEXT("SHADER_BACKEND_D3D12"), TEXT("(1)"));
    Preprocessor.AddDefine(TEXT("SHADER_BACKEND"), TEXT("SHADER_BACKEND_D3D12"));

    FShaderPreprocessorOutput Output;
    const bool bPreprocessed = Preprocessor.Preprocess(FilePath, FStringView(TEXT("#define CONFIG_CUBE_MAP 0\n#include \"GenerateMips.hlsli\"")), Output);

    TEST_SECTION(TEXT("Includes resolve next to the including file and in the shader directory"));
    TEST_EXPECT(bPreprocessed);
    TEST_EXPECT(Output.Errors.IsEmpty());

    bool bFoundGenerateMips = false;
    bool bFoundCoreDefines  = false;
    for (const FString& Dependency : Output.Dependencies)
    {
        bFoundGenerateMips |= Dependency.EndsWith(TEXT("Shaders/GenerateMips.hlsli"));
        bFoundCoreDefines  |= Dependency.EndsWith(TEXT("Shaders/CoreDefines.hlsli"));
    }

    TEST_EXPECT(bFoundGenerateMips);
    TEST_EXPECT(bFoundCoreDefines);

    TEST_SECTION(TEXT("Include guards keep each header to one copy"));
    int32 NumSourceMips = 0;
    for (const FShaderToken& Token : Output.Tokens)
    {
        NumSourceMips += Token.Is(TEXT("SourceMip")) ? 1 : 0;
    }

    TEST_EXPECT(NumSourceMips > 0);
    TEST_EXPECT(JoinTokens(Output).Contains(TEXT("void Main")));

    TEST_END();
}

bool ShaderPreprocessorErrors_Test()
{
    TEST_BEGIN();

    struct FErrorCase
    {
        const CHAR_T* Source;
        const CHAR_T* ExpectedMessage;
    };

    const FErrorCase Cases[] =
    {
        { TEXT("#error Something went wrong\n"),       TEXT("Something went wrong") },
        { TEXT("#if 1\na\n"),                           TEXT("Unterminated #if") },
        { TEXT("#endif\n"),                             TEXT("#endif without #if") },
        { TEXT("#include \"DoesNotExist.hlsli\"\n"),    TEXT("Cannot find the include") },
        { TEXT("#define F(a, b) a\nF(1)\n"),           TEXT("takes 2 arguments") },
        { TEXT("#define F(a) a\nF(1\n"),               TEXT("Unterminated invocation") },
        { TEXT("#if 1 +\n#endif\n"),                    TEXT("Unexpected end") },
        { TEXT("#bogus\n"),                             TEXT("Unsupported directive") },
    };

    for (const FErrorCase& Case : Cases)
    {
        TEST_SECTION(Case.ExpectedMessage);

        FShaderPreprocessorOutput Output;
        TEST_EXPECT(!Preprocess(Case.Source, Output));
        TEST_EXPECT(Output.Errors.Contains(Case.ExpectedMessage));
    }

    TEST_END();
}

bool ShaderPreprocessorGenerateSource_Test()
{
    TEST_BEGIN();

    FShaderPreprocessorOutput Output;
    TEST_EXPECT(Preprocess(TEXT("#define X 1\nfloat a = X;\n\n\nfloat b;\n#pragma warning(disable : 3078)\nfloat c;"), Output));

    const FString Generated = Output.GenerateSource();

    TEST_SECTION(TEXT("Every line keeps its line number"));
    TEST_EXPECT(Generated.StartsWith(TEXT("#line 2 \"ShaderSource\"\nfloat a = 1;\n\n\nfloat b;\n")));

    TEST_SECTION(TEXT("Pragmas are passed through for the compiler"));
    TEST_EXPECT(Generated.Contains(TEXT("\n#pragma warning(disable : 3078)\nfloat c;")));

    TEST_SECTION(TEXT("Tokens that were apart stay apart"));
    FShaderPreprocessorOutput Adjacent;
    TEST_EXPECT(Preprocess(TEXT("#define NEG -\nint a = -NEG 1;"), Adjacent));
    TEST_EXPECT(Adjacent.GenerateSource().Contains(TEXT("- - 1")));

    TEST_SECTION(TEXT("Files under the base directory are named relative to it, so no machine path reaches the compiler"));
    FShaderPreprocessorOutput Named;
    TEST_EXPECT(Preprocess(TEXT("float a;"), Named));
    Named.Files[0] = TEXT("C:/Motor åäö 日本/Assets/Shaders/Lighting/Sun.hlsl");
    TEST_EXPECT(Named.GenerateSource(TEXT("C:/Motor åäö 日本/Assets/Shaders")).StartsWith(TEXT("#line 1 \"Lighting/Sun.hlsl\"\n")));
    TEST_EXPECT(Named.GenerateSource(TEXT("C:/Elsewhere")).StartsWith(TEXT("#line 1 \"C:/Motor åäö 日本/Assets/Shaders/Lighting/Sun.hlsl\"\n")));
    TEST_EXPECT(Named.GenerateSource().StartsWith(TEXT("#line 1 \"C:/Motor åäö 日本/Assets/Shaders/Lighting/Sun.hlsl\"\n")));

    TEST_END();
}

bool FXCShaderTranslator_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("ConstantBuffer<T> becomes a cbuffer with the same layout"));
    TEST_EXPECT(TranslateToText(TEXT("ConstantBuffer<FCamera> CameraBuffer : register(b0);")) == TEXT("cbuffer CameraBuffer_CB : register ( b0 ) { FCamera CameraBuffer ; } ;"));
    TEST_EXPECT(TranslateToText(TEXT("ConstantBuffer<FCamera> CameraBuffer;")) == TEXT("cbuffer CameraBuffer_CB { FCamera CameraBuffer ; } ;"));

    TEST_SECTION(TEXT("The shader constants in space1 get no register, so FXC picks a free slot"));
    TEST_EXPECT(TranslateToText(TEXT("ConstantBuffer<FShaderBlockConstants> Constants : register(b0, space1);")) == TEXT("cbuffer Constants_CB { FShaderBlockConstants Constants ; } ;"));

    TEST_SECTION(TEXT("Register spaces and arrays cannot be expressed"));
    bool bSucceeded = true;
    TEST_EXPECT(TranslateToText(TEXT("ConstantBuffer<FLocal> Local : register(b0, space2);"), &bSucceeded).Contains(TEXT("no register spaces")));
    TEST_EXPECT(!bSucceeded);
    TEST_EXPECT(TranslateToText(TEXT("ConstantBuffer<FLocal> Locals[4] : register(b0);"), &bSucceeded).Contains(TEXT("array of constant buffers")));
    TEST_EXPECT(!bSucceeded);

    TEST_SECTION(TEXT("Attributes lose the second pair of brackets, indexing is left alone"));
    TEST_EXPECT(TranslateToText(TEXT("[[branch]] if (x) { a[b[c]] = 0; }")) == TEXT("[ branch ] if ( x ) { a [ b [ c ] ] = 0 ; }"));
    TEST_EXPECT(TranslateToText(TEXT("[[vk::push_constant]] struct S {};")) == TEXT("[ [ vk :: push_constant ] ] struct S { } ;"));

    TEST_SECTION(TEXT("Hexadecimal floats become decimals with the same value"));
    TEST_EXPECT(TranslateToText(TEXT("0x1.fffffep-1")) == TEXT("0.99999994039535522"));
    TEST_EXPECT(TranslateToText(TEXT("0x1p4f")) == TEXT("16.0f"));
    TEST_EXPECT(TranslateToText(TEXT("0x10")) == TEXT("0x10"));

    TEST_END();
}
