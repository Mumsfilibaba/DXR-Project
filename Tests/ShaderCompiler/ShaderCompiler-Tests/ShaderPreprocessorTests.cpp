#include "ShaderPreprocessorTests.h"

#include <Core/Misc/Paths.h>
#include <ShaderCompiler/ShaderPreprocessor.h>
#include <ShaderCompiler/FXC/FXCShaderTranslator.h>

#include "TestCommon/TestMacros.h"

static String JoinTokens(const FShaderPreprocessorOutput& Output)
{
    String Result;
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

static bool Preprocess(const CHAR* Source, FShaderPreprocessorOutput& OutOutput, const CHAR* DefineName = nullptr, const CHAR* DefineValue = nullptr)
{
    FShaderPreprocessor Preprocessor(Paths::GetAssetDir() + "/Shaders");
    if (DefineName)
    {
        Preprocessor.AddDefine(DefineName, DefineValue);
    }

    return Preprocessor.Preprocess(String(), StringView(Source), OutOutput);
}

static String PreprocessToText(const CHAR* Source, const CHAR* DefineName = nullptr, const CHAR* DefineValue = nullptr)
{
    FShaderPreprocessorOutput Output;
    if (!Preprocess(Source, Output, DefineName, DefineValue))
    {
        return String("<failed> ") + Output.Errors;
    }

    return JoinTokens(Output);
}

static String TranslateToText(const CHAR* Source, bool* bOutSucceeded = nullptr)
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

    TEST_SECTION("Object-like macros");
    TEST_EXPECT(PreprocessToText("#define A 1\nA + A") == "1 + 1");
    TEST_EXPECT(PreprocessToText("#define EMPTY\na EMPTY b") == "a b");

    TEST_SECTION("Function-like macros expand their arguments first");
    TEST_EXPECT(PreprocessToText("#define ADD(a, b) ((a) + (b))\n#define TWO 2\nADD(TWO, 3)") == "( ( 2 ) + ( 3 ) )");
    TEST_EXPECT(PreprocessToText("#define CLAUSE(Register) register(Register)\n#define SLOT t4\nCLAUSE(SLOT)") == "register ( t4 )");
    TEST_EXPECT(PreprocessToText("#define F(x) x\nF((1, 2))") == "( 1 , 2 )");

    TEST_SECTION("A function-like macro without parentheses is left alone");
    TEST_EXPECT(PreprocessToText("#define F(x) x\nfloat F;") == "float F ;");

    TEST_SECTION("Rescanning continues into the tokens after the expansion");
    TEST_EXPECT(PreprocessToText("#define F G\n#define G(x) x * 2\nF(4)") == "4 * 2");

    TEST_SECTION("A macro is never expanded inside its own expansion");
    TEST_EXPECT(PreprocessToText("#define A A + 1\nA") == "A + 1");
    TEST_EXPECT(PreprocessToText("#define A B\n#define B A\nA B") == "A B");

    TEST_SECTION("Invocations can span lines");
    TEST_EXPECT(PreprocessToText("#define SUM(a, b) a + b\nSUM(1,\n2)") == "1 + 2");

    TEST_SECTION("Token pasting and stringizing");
    TEST_EXPECT(PreprocessToText("#define CAT(a, b) a##b\nCAT(Foo, Bar)") == "FooBar");
    TEST_EXPECT(PreprocessToText("#define CAT(a, b) a##b\nCAT(, Bar)") == "Bar");
    TEST_EXPECT(PreprocessToText("#define STR(x) #x\nSTR(a + b)") == "\"a + b\"");

    TEST_SECTION("Line continuations and comments");
    TEST_EXPECT(PreprocessToText("#define LONG a \\\n + b // trailing\n/* multi\nline */ LONG") == "a + b");

    TEST_SECTION("Defines added by the compiler");
    TEST_EXPECT(PreprocessToText("VALUE", "VALUE", "(3)") == "( 3 )");
    TEST_EXPECT(PreprocessToText("#undef VALUE\nVALUE", "VALUE", "(3)") == "VALUE");

    TEST_END();
}

bool ShaderPreprocessorConditionals_Test()
{
    TEST_BEGIN();

    TEST_SECTION("#if evaluates shifts, which FXC's own preprocessor cannot");
    TEST_EXPECT(PreprocessToText("#define FLAG (1 << 2)\n#if (FLAG & 4) != 0\nyes\n#else\nno\n#endif") == "yes");

    TEST_SECTION("#elif, defined and logical operators");
    TEST_EXPECT(PreprocessToText("#if defined(X)\na\n#elif !defined Y && 3 > 2\nb\n#else\nc\n#endif") == "b");
    TEST_EXPECT(PreprocessToText("#define X\n#ifdef X\na\n#endif\n#ifndef X\nb\n#endif") == "a");

    TEST_SECTION("Undefined names are zero and the unevaluated side can divide by zero");
    TEST_EXPECT(PreprocessToText("#if UNDEFINED || 1\na\n#endif") == "a");
    TEST_EXPECT(PreprocessToText("#if 0 && (1 / 0)\na\n#else\nb\n#endif") == "b");
    TEST_EXPECT(PreprocessToText("#if 1 ? 0x10 == 16 : 0\na\n#endif") == "a");

    TEST_SECTION("Directives in skipped blocks are not evaluated");
    TEST_EXPECT(PreprocessToText("#ifdef NOPE\n#if 1 / 0\n#error Unreachable\n#endif\n#endif\nok") == "ok");

    TEST_SECTION("Values from the compiler defines");
    TEST_EXPECT(PreprocessToText("#if VALUE == 3\nthree\n#endif", "VALUE", "(3)") == "three");

    TEST_END();
}

bool ShaderPreprocessorIncludes_Test()
{
    TEST_BEGIN();

    const String FilePath = Paths::GetAssetDir() + "/Shaders/GenerateMipsTex2D.hlsl";

    FShaderPreprocessor Preprocessor(Paths::GetAssetDir() + "/Shaders");
    Preprocessor.AddDefine("SHADER_BACKEND_D3D12", "(1)");
    Preprocessor.AddDefine("SHADER_BACKEND", "SHADER_BACKEND_D3D12");

    FShaderPreprocessorOutput Output;
    const bool bPreprocessed = Preprocessor.Preprocess(FilePath, StringView("#define CONFIG_CUBE_MAP 0\n#include \"GenerateMips.hlsli\""), Output);

    TEST_SECTION("Includes resolve next to the including file and in the shader directory");
    TEST_EXPECT(bPreprocessed);
    TEST_EXPECT(Output.Errors.IsEmpty());

    bool bFoundGenerateMips = false;
    bool bFoundCoreDefines  = false;
    for (const String& Dependency : Output.Dependencies)
    {
        bFoundGenerateMips |= Dependency.EndsWith("Shaders/GenerateMips.hlsli");
        bFoundCoreDefines  |= Dependency.EndsWith("Shaders/CoreDefines.hlsli");
    }

    TEST_EXPECT(bFoundGenerateMips);
    TEST_EXPECT(bFoundCoreDefines);

    TEST_SECTION("Include guards keep each header to one copy");
    int32 NumSourceMips = 0;
    for (const FShaderToken& Token : Output.Tokens)
    {
        NumSourceMips += Token.Is("SourceMip") ? 1 : 0;
    }

    TEST_EXPECT(NumSourceMips > 0);
    TEST_EXPECT(JoinTokens(Output).Contains("void Main"));

    TEST_END();
}

bool ShaderPreprocessorErrors_Test()
{
    TEST_BEGIN();

    struct FErrorCase
    {
        const CHAR* Source;
        const CHAR* ExpectedMessage;
    };

    const FErrorCase Cases[] =
    {
        { "#error Something went wrong\n",       "Something went wrong" },
        { "#if 1\na\n",                           "Unterminated #if" },
        { "#endif\n",                             "#endif without #if" },
        { "#include \"DoesNotExist.hlsli\"\n",    "Cannot find the include" },
        { "#define F(a, b) a\nF(1)\n",           "takes 2 arguments" },
        { "#define F(a) a\nF(1\n",               "Unterminated invocation" },
        { "#if 1 +\n#endif\n",                    "Unexpected end" },
        { "#bogus\n",                             "Unsupported directive" },
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

bool ShaderPreprocessorRender_Test()
{
    TEST_BEGIN();

    FShaderPreprocessorOutput Output;
    TEST_EXPECT(Preprocess("#define X 1\nfloat a = X;\n\n\nfloat b;\n#pragma warning(disable : 3078)\nfloat c;", Output));

    const String Rendered = Output.Render();

    TEST_SECTION("Every line keeps its line number");
    TEST_EXPECT(Rendered.StartsWith("#line 2 \"ShaderSource\"\nfloat a = 1;\n\n\nfloat b;\n"));

    TEST_SECTION("Pragmas are passed through for the compiler");
    TEST_EXPECT(Rendered.Contains("\n#pragma warning(disable : 3078)\nfloat c;"));

    TEST_SECTION("Tokens that were apart stay apart");
    FShaderPreprocessorOutput Adjacent;
    TEST_EXPECT(Preprocess("#define NEG -\nint a = -NEG 1;", Adjacent));
    TEST_EXPECT(Adjacent.Render().Contains("- - 1"));

    TEST_END();
}

bool FXCShaderTranslator_Test()
{
    TEST_BEGIN();

    TEST_SECTION("ConstantBuffer<T> becomes a cbuffer with the same layout");
    TEST_EXPECT(TranslateToText("ConstantBuffer<FCamera> CameraBuffer : register(b0);") == "cbuffer CameraBuffer_CB : register ( b0 ) { FCamera CameraBuffer ; } ;");
    TEST_EXPECT(TranslateToText("ConstantBuffer<FCamera> CameraBuffer;") == "cbuffer CameraBuffer_CB { FCamera CameraBuffer ; } ;");

    TEST_SECTION("The shader constants in space1 get no register, so FXC picks a free slot");
    TEST_EXPECT(TranslateToText("ConstantBuffer<FShaderBlockConstants> Constants : register(b0, space1);") == "cbuffer Constants_CB { FShaderBlockConstants Constants ; } ;");

    TEST_SECTION("Register spaces and arrays cannot be expressed");
    bool bSucceeded = true;
    TEST_EXPECT(TranslateToText("ConstantBuffer<FLocal> Local : register(b0, space2);", &bSucceeded).Contains("no register spaces"));
    TEST_EXPECT(!bSucceeded);
    TEST_EXPECT(TranslateToText("ConstantBuffer<FLocal> Locals[4] : register(b0);", &bSucceeded).Contains("array of constant buffers"));
    TEST_EXPECT(!bSucceeded);

    TEST_SECTION("Attributes lose the second pair of brackets, indexing is left alone");
    TEST_EXPECT(TranslateToText("[[branch]] if (x) { a[b[c]] = 0; }") == "[ branch ] if ( x ) { a [ b [ c ] ] = 0 ; }");
    TEST_EXPECT(TranslateToText("[[vk::push_constant]] struct S {};") == "[ [ vk :: push_constant ] ] struct S { } ;");

    TEST_SECTION("Hexadecimal floats become decimals with the same value");
    TEST_EXPECT(TranslateToText("0x1.fffffep-1") == "0.99999994039535522");
    TEST_EXPECT(TranslateToText("0x1p4f") == "16.0f");
    TEST_EXPECT(TranslateToText("0x10") == "0x10");

    TEST_END();
}
