#include "Core/Filesystem/File.h"
#include "Core/Platform/PlatformFile.h"
#include "ShaderCompiler/ShaderPreprocessor.h"

static constexpr int32 MaxIncludeDepth = 64;

// Longest first, so the lexer can take the first match
static const CHAR* const GMultiCharPunctuators[] =
{
    "<<=", ">>=", "...",
    "##", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||", "++", "--",
    "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "->", "::",
};

static bool IsIdentifierStart(CHAR Char)
{
    return (Char >= 'a' && Char <= 'z') || (Char >= 'A' && Char <= 'Z') || Char == '_';
}

static bool IsDigit(CHAR Char)
{
    return Char >= '0' && Char <= '9';
}

static bool IsIdentifierChar(CHAR Char)
{
    return IsIdentifierStart(Char) || IsDigit(Char);
}

static bool IsHorizontalSpace(CHAR Char)
{
    return Char == ' ' || Char == '\t' || Char == '\v' || Char == '\f';
}

static bool IsPunctuator(CHAR Char)
{
    return (Char >= '!' && Char <= '/') || (Char >= ':' && Char <= '@') || (Char >= '[' && Char <= '^') || (Char >= '{' && Char <= '~');
}

static FShaderToken MakeToken(const FShaderToken& Location, const CHAR* Text, EShaderTokenType Type, bool bLeadingSpace)
{
    FShaderToken Token;
    Token.Text          = Text;
    Token.Type          = Type;
    Token.FileIndex     = Location.FileIndex;
    Token.Line          = Location.Line;
    Token.bLeadingSpace = bLeadingSpace;
    return Token;
}

static String JoinTokens(TArrayView<const FShaderToken> Tokens)
{
    String Result;
    for (int32 Index = 0; Index < Tokens.Size(); ++Index)
    {
        if (Index > 0 && Tokens[Index].bLeadingSpace)
        {
            Result += ' ';
        }

        Result += Tokens[Index].Text;
    }

    return Result;
}

// Two tokens that were separate must not merge into one when they are printed next to each other
static bool NeedsSeparator(const FShaderToken& Previous, const FShaderToken& Current)
{
    const bool bPreviousIsWord = Previous.Type == EShaderTokenType::Identifier || Previous.Type == EShaderTokenType::Number;
    const bool bCurrentIsWord  = Current.Type == EShaderTokenType::Identifier || Current.Type == EShaderTokenType::Number;
    if (bPreviousIsWord && (bCurrentIsWord || (Previous.Type == EShaderTokenType::Number && Current.Text.StartsWith("."))))
    {
        return true;
    }

    if (Previous.Text.IsEmpty() || Current.Text.IsEmpty() || Previous.Type != EShaderTokenType::Punctuator || Current.Type != EShaderTokenType::Punctuator)
    {
        return false;
    }

    const CHAR Pair[3] = { Previous.Text[Previous.Text.Length() - 1], Current.Text[0], '\0' };
    if (CString::Strcmp(Pair, "//") == 0 || CString::Strcmp(Pair, "/*") == 0 || CString::Strcmp(Pair, "..") == 0)
    {
        return true;
    }

    for (const CHAR* Punctuator : GMultiCharPunctuators)
    {
        if (Punctuator[0] == Pair[0] && Punctuator[1] == Pair[1])
        {
            return true;
        }
    }

    return false;
}

// ------------------------------------------------------------------------------------------------
// #if expressions
// ------------------------------------------------------------------------------------------------

class FConditionParser
{
public:
    FConditionParser(TArrayView<const FShaderToken> InTokens)
        : Tokens(InTokens)
        , Position(0)
        , ErrorMessage()
    {
    }

    bool Parse(int64& OutValue)
    {
        OutValue = ParseTernary(true);
        if (ErrorMessage.IsEmpty() && Position < Tokens.Size())
        {
            ErrorMessage = String::Printf("Unexpected '%s' in #if expression", *Tokens[Position].Text);
        }

        return ErrorMessage.IsEmpty();
    }

    NODISCARD const String& GetError() const
    {
        return ErrorMessage;
    }

private:
    bool Accept(const CHAR* Text)
    {
        if (Position < Tokens.Size() && Tokens[Position].Is(Text))
        {
            ++Position;
            return true;
        }

        return false;
    }

    int64 Fail(const String& Message)
    {
        if (ErrorMessage.IsEmpty())
        {
            ErrorMessage = Message;
        }

        Position = Tokens.Size();
        return 0;
    }

    int64 ParseTernary(bool bEvaluate)
    {
        const int64 Condition = ParseBinary(0, bEvaluate);
        if (!Accept("?"))
        {
            return Condition;
        }

        const int64 TrueValue = ParseTernary(bEvaluate && Condition != 0);
        if (!Accept(":"))
        {
            return Fail("Expected ':' in #if expression");
        }

        const int64 FalseValue = ParseTernary(bEvaluate && Condition == 0);
        return Condition != 0 ? TrueValue : FalseValue;
    }

    static int32 GetPrecedence(const String& Operator)
    {
        static const CHAR* const Levels[][4] =
        {
            { "||" },
            { "&&" },
            { "|" },
            { "^" },
            { "&" },
            { "==", "!=" },
            { "<", ">", "<=", ">=" },
            { "<<", ">>" },
            { "+", "-" },
            { "*", "/", "%" },
        };

        for (int32 Level = 0; Level < static_cast<int32>(ARRAY_COUNT(Levels)); ++Level)
        {
            for (const CHAR* Candidate : Levels[Level])
            {
                if (Candidate && Operator.Equals(Candidate))
                {
                    return Level;
                }
            }
        }

        return -1;
    }

    int64 ParseBinary(int32 MinPrecedence, bool bEvaluate)
    {
        int64 Left = ParseUnary(bEvaluate);
        while (Position < Tokens.Size())
        {
            const String& Operator = Tokens[Position].Text;
            const int32 Precedence = GetPrecedence(Operator);
            if (Precedence < MinPrecedence)
            {
                break;
            }

            ++Position;

            // The right-hand side of a short-circuit operator is only parsed, so '1 || 1/0' is fine
            bool bEvaluateRight = bEvaluate;
            if (Operator.Equals("&&"))
            {
                bEvaluateRight = bEvaluate && Left != 0;
            }
            else if (Operator.Equals("||"))
            {
                bEvaluateRight = bEvaluate && Left == 0;
            }

            const int64 Right = ParseBinary(Precedence + 1, bEvaluateRight);
            Left = Apply(Operator, Left, Right, bEvaluateRight);
        }

        return Left;
    }

    int64 Apply(const String& Operator, int64 Left, int64 Right, bool bEvaluate)
    {
        if (Operator.Equals("||"))
        {
            return (Left != 0 || Right != 0) ? 1 : 0;
        }

        if (Operator.Equals("&&"))
        {
            return (Left != 0 && Right != 0) ? 1 : 0;
        }

        if (Operator.Equals("|"))
        {
            return Left | Right;
        }

        if (Operator.Equals("^"))
        {
            return Left ^ Right;
        }

        if (Operator.Equals("&"))
        {
            return Left & Right;
        }

        if (Operator.Equals("=="))
        {
            return Left == Right ? 1 : 0;
        }

        if (Operator.Equals("!="))
        {
            return Left != Right ? 1 : 0;
        }

        if (Operator.Equals("<"))
        {
            return Left < Right ? 1 : 0;
        }

        if (Operator.Equals(">"))
        {
            return Left > Right ? 1 : 0;
        }

        if (Operator.Equals("<="))
        {
            return Left <= Right ? 1 : 0;
        }

        if (Operator.Equals(">="))
        {
            return Left >= Right ? 1 : 0;
        }

        if (Operator.Equals("<<"))
        {
            return (Right >= 0 && Right < 64) ? (Left << Right) : 0;
        }

        if (Operator.Equals(">>"))
        {
            return (Right >= 0 && Right < 64) ? (Left >> Right) : 0;
        }

        if (Operator.Equals("+"))
        {
            return Left + Right;
        }

        if (Operator.Equals("-"))
        {
            return Left - Right;
        }

        if (Operator.Equals("*"))
        {
            return Left * Right;
        }

        if (Right == 0)
        {
            return bEvaluate ? Fail("Division by zero in #if expression") : 0;
        }

        return Operator.Equals("/") ? (Left / Right) : (Left % Right);
    }

    int64 ParseUnary(bool bEvaluate)
    {
        if (Accept("!"))
        {
            return ParseUnary(bEvaluate) == 0 ? 1 : 0;
        }

        if (Accept("~"))
        {
            return ~ParseUnary(bEvaluate);
        }

        if (Accept("-"))
        {
            return -ParseUnary(bEvaluate);
        }

        if (Accept("+"))
        {
            return ParseUnary(bEvaluate);
        }

        return ParsePrimary(bEvaluate);
    }

    int64 ParsePrimary(bool bEvaluate)
    {
        if (Position >= Tokens.Size())
        {
            return Fail("Unexpected end of #if expression");
        }

        if (Accept("("))
        {
            const int64 Value = ParseTernary(bEvaluate);
            if (!Accept(")"))
            {
                return Fail("Expected ')' in #if expression");
            }

            return Value;
        }

        const FShaderToken& Token = Tokens[Position++];
        if (Token.Type != EShaderTokenType::Number)
        {
            return Fail(String::Printf("Unexpected '%s' in #if expression", *Token.Text));
        }

        return ParseInteger(Token.Text);
    }

    int64 ParseInteger(const String& Text)
    {
        int32 End = Text.Length();
        while (End > 0 && (Text[End - 1] == 'u' || Text[End - 1] == 'U' || Text[End - 1] == 'l' || Text[End - 1] == 'L'))
        {
            --End;
        }

        int32 Index = 0;
        uint64 Base = 10;
        if (End > 2 && Text[0] == '0' && (Text[1] == 'x' || Text[1] == 'X'))
        {
            Base  = 16;
            Index = 2;
        }
        else if (End > 1 && Text[0] == '0')
        {
            Base  = 8;
            Index = 1;
        }

        uint64 Value = 0;
        for (; Index < End; ++Index)
        {
            const CHAR Char = Text[Index];

            uint64 Digit = 16;
            if (IsDigit(Char))
            {
                Digit = static_cast<uint64>(Char - '0');
            }
            else if (Char >= 'a' && Char <= 'f')
            {
                Digit = static_cast<uint64>(Char - 'a' + 10);
            }
            else if (Char >= 'A' && Char <= 'F')
            {
                Digit = static_cast<uint64>(Char - 'A' + 10);
            }

            if (Digit >= Base)
            {
                return Fail(String::Printf("'%s' is not an integer, #if only works with integers", *Text));
            }

            Value = Value * Base + Digit;
        }

        return static_cast<int64>(Value);
    }

    TArrayView<const FShaderToken> Tokens;
    int32                          Position;
    String                         ErrorMessage;
};

// ------------------------------------------------------------------------------------------------
// Output
// ------------------------------------------------------------------------------------------------

String FShaderPreprocessorOutput::Render() const
{
    // A gap this small is cheaper to bridge with empty lines than with a #line marker
    constexpr int32 MaxLineGap = 8;

    String Result;
    Result.Reserve(Tokens.Size() * 6);

    int32 CurrentFile  = -1;
    int32 CurrentLine  = 0;
    bool  bLineStart   = true;
    const FShaderToken* Previous = nullptr;

    for (const FShaderToken& Token : Tokens)
    {
        if (Token.FileIndex != CurrentFile || Token.Line < CurrentLine || Token.Line > CurrentLine + MaxLineGap)
        {
            if (!bLineStart)
            {
                Result += '\n';
            }

            const String& FileName = (Token.FileIndex >= 0 && Token.FileIndex < Files.Size()) ? Files[Token.FileIndex] : String();
            Result += String::Printf("#line %d \"%s\"\n", Token.Line, *FileName);

            CurrentFile = Token.FileIndex;
            CurrentLine = Token.Line;
            bLineStart  = true;
            Previous    = nullptr;
        }

        while (CurrentLine < Token.Line)
        {
            Result += '\n';
            ++CurrentLine;
            bLineStart = true;
            Previous   = nullptr;
        }

        if (Token.Type == EShaderTokenType::Directive)
        {
            if (!bLineStart)
            {
                Result += '\n';
                ++CurrentLine;
            }

            Result += Token.Text;
            Result += '\n';
            ++CurrentLine;
            bLineStart = true;
            Previous   = nullptr;
            continue;
        }

        if (Previous && (Token.bLeadingSpace || NeedsSeparator(*Previous, Token)))
        {
            Result += ' ';
        }

        Result += Token.Text;
        bLineStart = false;
        Previous   = &Token;
    }

    if (!bLineStart)
    {
        Result += '\n';
    }

    return Result;
}

// ------------------------------------------------------------------------------------------------
// Lexer
// ------------------------------------------------------------------------------------------------

void FShaderPreprocessor::Tokenize(StringView Source, int32 FileIndex, TArray<FShaderToken>& OutTokens)
{
    // Remove the line continuations up front, remembering where they were so the line numbers stay right
    TArray<CHAR>  Text;
    TArray<int32> Continuations;
    Text.Reserve(Source.Size());

    for (int32 Index = 0; Index < Source.Size(); ++Index)
    {
        const CHAR Char = Source.Data()[Index];
        if (Char == '\0')
        {
            break;
        }
        else if (Char == '\r')
        {
            continue;
        }
        else if (Char == '\\')
        {
            int32 Next = Index + 1;
            if (Next < Source.Size() && Source.Data()[Next] == '\r')
            {
                ++Next;
            }

            if (Next < Source.Size() && Source.Data()[Next] == '\n')
            {
                Continuations.Add(Text.Size());
                Index = Next;
                continue;
            }
        }

        Text.Add(Char);
    }

    const int32 Length = Text.Size();
    const CHAR* Chars  = Text.Data();

    int32 Line              = 1;
    int32 ContinuationIndex = 0;
    bool  bStartOfLine      = true;
    bool  bLeadingSpace     = false;

    int32 Index = 0;
    while (Index < Length)
    {
        while (ContinuationIndex < Continuations.Size() && Continuations[ContinuationIndex] <= Index)
        {
            ++ContinuationIndex;
            ++Line;
        }

        const CHAR Char = Chars[Index];
        if (Char == '\n')
        {
            ++Line;
            ++Index;
            bStartOfLine  = true;
            bLeadingSpace = false;
            continue;
        }

        if (IsHorizontalSpace(Char))
        {
            bLeadingSpace = true;
            ++Index;
            continue;
        }

        if (Char == '/' && Index + 1 < Length && Chars[Index + 1] == '/')
        {
            while (Index < Length && Chars[Index] != '\n')
            {
                ++Index;
            }

            bLeadingSpace = true;
            continue;
        }

        // A block comment is one space, so a line that continues after one is still the same logical line
        if (Char == '/' && Index + 1 < Length && Chars[Index + 1] == '*')
        {
            Index += 2;
            while (Index < Length && !(Chars[Index] == '*' && Index + 1 < Length && Chars[Index + 1] == '/'))
            {
                if (Chars[Index] == '\n')
                {
                    ++Line;
                }

                ++Index;
            }

            Index = (Index < Length) ? Index + 2 : Length;
            bLeadingSpace = true;
            continue;
        }

        const int32 Start = Index;
        EShaderTokenType Type = EShaderTokenType::Other;

        if (IsIdentifierStart(Char))
        {
            Type = EShaderTokenType::Identifier;
            while (Index < Length && IsIdentifierChar(Chars[Index]))
            {
                ++Index;
            }
        }
        else if (IsDigit(Char) || (Char == '.' && Index + 1 < Length && IsDigit(Chars[Index + 1])))
        {
            Type = EShaderTokenType::Number;
            ++Index;

            while (Index < Length)
            {
                const CHAR Current = Chars[Index];
                const CHAR Last    = Chars[Index - 1];
                if (IsIdentifierChar(Current) || Current == '.')
                {
                    ++Index;
                }
                else if ((Current == '+' || Current == '-') && (Last == 'e' || Last == 'E' || Last == 'p' || Last == 'P'))
                {
                    ++Index;
                }
                else
                {
                    break;
                }
            }
        }
        else if (Char == '"' || Char == '\'')
        {
            Type = EShaderTokenType::String;
            ++Index;

            while (Index < Length && Chars[Index] != Char && Chars[Index] != '\n')
            {
                Index += (Chars[Index] == '\\' && Index + 1 < Length) ? 2 : 1;
            }

            if (Index < Length && Chars[Index] == Char)
            {
                ++Index;
            }
        }
        else if (IsPunctuator(Char))
        {
            Type = EShaderTokenType::Punctuator;

            int32 MatchLength = 1;
            for (const CHAR* Punctuator : GMultiCharPunctuators)
            {
                const int32 PunctuatorLength = CString::Strlen(Punctuator);
                if (Index + PunctuatorLength <= Length && CString::Strncmp(Chars + Index, Punctuator, PunctuatorLength) == 0)
                {
                    MatchLength = PunctuatorLength;
                    break;
                }
            }

            Index += MatchLength;
        }
        else
        {
            ++Index;
        }

        FShaderToken& Token = OutTokens.Emplace();
        Token.Text          = String(Chars + Start, Index - Start);
        Token.Type          = Type;
        Token.FileIndex     = FileIndex;
        Token.Line          = Line;
        Token.bLeadingSpace = bLeadingSpace;
        Token.bStartOfLine  = bStartOfLine;

        bStartOfLine  = false;
        bLeadingSpace = false;
    }
}

// ------------------------------------------------------------------------------------------------
// Preprocessor
// ------------------------------------------------------------------------------------------------

FShaderPreprocessor::FShaderPreprocessor(const String& InIncludeDir, const IShaderSourceProvider* InSourceProvider)
    : IncludeDir(InIncludeDir)
    , AdditionalIncludeDirs()
    , SourceProvider(InSourceProvider ? InSourceProvider : &FDiskShaderSourceProvider::Get())
    , Macros()
    , OnceFiles()
    , Conditionals()
    , Output(nullptr)
{
}

FShaderPreprocessor::~FShaderPreprocessor() = default;

void FShaderPreprocessor::AddIncludeDir(const String& Directory)
{
    AdditionalIncludeDirs.Add(Directory);
}

void FShaderPreprocessor::AddDefine(const String& Name, const String& Value)
{
    FMacro Macro;
    Tokenize(StringView(Value), -1, Macro.Body);
    Macros.Add(Name, Macro);
}

bool FShaderPreprocessor::Preprocess(const String& FilePath, StringView Source, FShaderPreprocessorOutput& OutOutput)
{
    Output = &OutOutput;
    Conditionals.Clear();
    OnceFiles.Clear();

    const bool bResult = ProcessFile(FilePath.IsEmpty() ? String("ShaderSource") : FilePath, Source, 0);

    Output = nullptr;
    return bResult;
}

bool FShaderPreprocessor::IsActive() const
{
    return Conditionals.IsEmpty() || Conditionals.Last().bActive;
}

bool FShaderPreprocessor::Error(const FShaderToken& Location, const String& Message)
{
    const bool bHasFile = Location.FileIndex >= 0 && Location.FileIndex < Output->Files.Size();
    Output->Errors += String::Printf("%s(%d): error: %s\n", bHasFile ? *Output->Files[Location.FileIndex] : "ShaderSource", Location.Line, *Message);
    return false;
}

bool FShaderPreprocessor::ProcessFile(const String& FilePath, StringView Source, int32 IncludeDepth)
{
    const int32 FileIndex = Output->Files.Size();
    Output->Files.Add(FilePath);

    TArray<FShaderToken> Tokens;
    Tokenize(Source, FileIndex, Tokens);

    const int32 ConditionalDepth = Conditionals.Size();

    // Text is held back while it has open parentheses, since a macro invocation can span several lines
    TArray<FShaderToken> Pending;
    int32 PendingParentheses = 0;

    const auto FlushPending = [&]()
    {
        PendingParentheses = 0;
        return Pending.IsEmpty() || ExpandTokens(::Move(Pending), Output->Tokens);
    };

    int32 Index = 0;
    while (Index < Tokens.Size())
    {
        int32 LineEnd = Index + 1;
        while (LineEnd < Tokens.Size() && !Tokens[LineEnd].bStartOfLine)
        {
            ++LineEnd;
        }

        const FShaderToken& First = Tokens[Index];
        if (First.bStartOfLine && First.Is("#"))
        {
            if (!FlushPending())
            {
                return false;
            }

            Pending.Clear();

            const TArrayView<const FShaderToken> Directive(Tokens.Data() + Index + 1, LineEnd - Index - 1);
            if (!HandleDirective(Directive, First, FilePath, IncludeDepth))
            {
                return false;
            }
        }
        else if (IsActive())
        {
            for (int32 TokenIndex = Index; TokenIndex < LineEnd; ++TokenIndex)
            {
                const FShaderToken& Token = Tokens[TokenIndex];
                if (Token.Is("("))
                {
                    ++PendingParentheses;
                }
                else if (Token.Is(")"))
                {
                    --PendingParentheses;
                }

                Pending.Add(Token);
            }

            if (PendingParentheses <= 0)
            {
                if (!FlushPending())
                {
                    return false;
                }

                Pending.Clear();
            }
        }

        Index = LineEnd;
    }

    if (!FlushPending())
    {
        return false;
    }

    if (Conditionals.Size() != ConditionalDepth)
    {
        FShaderToken Location;
        Location.FileIndex = FileIndex;
        Location.Line      = Tokens.IsEmpty() ? 1 : Tokens.Last().Line;
        return Error(Location, "Unterminated #if at the end of the file");
    }

    return true;
}

bool FShaderPreprocessor::HandleDirective(TArrayView<const FShaderToken> Tokens, const FShaderToken& HashToken, const String& FilePath, int32 IncludeDepth)
{
    // A lone '#' is the null directive
    if (Tokens.IsEmpty())
    {
        return true;
    }

    const FShaderToken& Name = Tokens[0];
    const TArrayView<const FShaderToken> Arguments = Tokens.SubView(1, Tokens.Size() - 1);

    if (Name.Is("if") || Name.Is("ifdef") || Name.Is("ifndef"))
    {
        FConditional Conditional;
        Conditional.bParentActive = IsActive();

        bool bValue = false;
        if (Conditional.bParentActive)
        {
            if (Name.Is("if"))
            {
                if (!EvaluateCondition(Arguments, Name, bValue))
                {
                    return false;
                }
            }
            else
            {
                if (Arguments.IsEmpty() || Arguments[0].Type != EShaderTokenType::Identifier)
                {
                    return Error(Name, String::Printf("#%s expects a macro name", *Name.Text));
                }

                bValue = (Macros.Find(Arguments[0].Text) != nullptr) == Name.Is("ifdef");
            }
        }

        Conditional.bActive = Conditional.bParentActive && bValue;
        Conditional.bTaken  = Conditional.bActive;
        Conditionals.Add(Conditional);
        return true;
    }

    if (Name.Is("elif") || Name.Is("else") || Name.Is("endif"))
    {
        if (Conditionals.IsEmpty())
        {
            return Error(Name, String::Printf("#%s without #if", *Name.Text));
        }

        FConditional& Conditional = Conditionals.Last();
        if (Name.Is("endif"))
        {
            Conditionals.Pop();
            return true;
        }

        if (Conditional.bSeenElse)
        {
            return Error(Name, String::Printf("#%s after #else", *Name.Text));
        }

        if (Name.Is("else"))
        {
            Conditional.bSeenElse = true;
            Conditional.bActive   = Conditional.bParentActive && !Conditional.bTaken;
            Conditional.bTaken   |= Conditional.bActive;
            return true;
        }

        bool bValue = false;
        if (Conditional.bParentActive && !Conditional.bTaken)
        {
            if (!EvaluateCondition(Arguments, Name, bValue))
            {
                return false;
            }
        }

        // EvaluateCondition cannot add conditionals, so the reference is still valid
        Conditional.bActive = bValue;
        Conditional.bTaken |= bValue;
        return true;
    }

    // Everything else in an inactive block is skipped without being looked at
    if (!IsActive())
    {
        return true;
    }

    if (Name.Is("define"))
    {
        return HandleDefine(Arguments, Name);
    }

    if (Name.Is("undef"))
    {
        if (Arguments.IsEmpty() || Arguments[0].Type != EShaderTokenType::Identifier)
        {
            return Error(Name, "#undef expects a macro name");
        }

        Macros.Remove(Arguments[0].Text);
        return true;
    }

    if (Name.Is("include"))
    {
        return HandleInclude(Arguments, Name, FilePath, IncludeDepth);
    }

    if (Name.Is("error"))
    {
        return Error(Name, String::Printf("#error %s", *JoinTokens(Arguments)));
    }

    if (Name.Is("pragma"))
    {
        if (!Arguments.IsEmpty() && Arguments[0].Is("once"))
        {
            OnceFiles.Add(FilePath);
            return true;
        }

        // Pragmas belong to the compiler, so they are passed through untouched
        FShaderToken& Pragma = Output->Tokens.Emplace(MakeToken(HashToken, "", EShaderTokenType::Directive, false));
        Pragma.Text = String("#pragma ") + JoinTokens(Arguments);
        return true;
    }

    return Error(Name, String::Printf("Unsupported directive #%s", *Name.Text));
}

bool FShaderPreprocessor::HandleDefine(TArrayView<const FShaderToken> Tokens, const FShaderToken& Location)
{
    if (Tokens.IsEmpty() || Tokens[0].Type != EShaderTokenType::Identifier)
    {
        return Error(Location, "#define expects a macro name");
    }

    if (Tokens[0].Is("defined"))
    {
        return Error(Tokens[0], "'defined' cannot be used as a macro name");
    }

    FMacro Macro;

    int32 BodyStart = 1;
    if (Tokens.Size() > 1 && Tokens[1].Is("(") && !Tokens[1].bLeadingSpace)
    {
        Macro.bFunctionLike = true;

        int32 Index = 2;
        if (Index < Tokens.Size() && Tokens[Index].Is(")"))
        {
            ++Index;
        }
        else
        {
            while (true)
            {
                if (Index >= Tokens.Size())
                {
                    return Error(Tokens[0], "Unterminated macro parameter list");
                }

                if (Tokens[Index].Is("..."))
                {
                    return Error(Tokens[Index], "Variadic macros are not supported");
                }

                if (Tokens[Index].Type != EShaderTokenType::Identifier)
                {
                    return Error(Tokens[Index], String::Printf("Expected a parameter name, found '%s'", *Tokens[Index].Text));
                }

                Macro.Parameters.Add(Tokens[Index].Text);
                ++Index;

                if (Index < Tokens.Size() && Tokens[Index].Is(","))
                {
                    ++Index;
                }
                else if (Index < Tokens.Size() && Tokens[Index].Is(")"))
                {
                    ++Index;
                    break;
                }
                else
                {
                    return Error(Tokens[0], "Expected ',' or ')' in the macro parameter list");
                }
            }
        }

        BodyStart = Index;
    }

    for (int32 Index = BodyStart; Index < Tokens.Size(); ++Index)
    {
        FShaderToken& Token = Macro.Body.Emplace(Tokens[Index]);
        Token.bStartOfLine   = false;
        Token.bPasteOperator = Token.Is("##");
    }

    if (!Macro.Body.IsEmpty())
    {
        if (Macro.Body[0].bPasteOperator || Macro.Body.Last().bPasteOperator)
        {
            return Error(Tokens[0], "'##' cannot be at either end of a macro");
        }

        Macro.Body[0].bLeadingSpace = false;
    }

    Macros.Add(Tokens[0].Text, Macro);
    return true;
}

bool FShaderPreprocessor::HandleInclude(TArrayView<const FShaderToken> Tokens, const FShaderToken& Location, const String& FilePath, int32 IncludeDepth)
{
    String IncludeName;
    bool   bSystemInclude = false;

    if (Tokens.Size() == 1 && Tokens[0].Type == EShaderTokenType::String && Tokens[0].Text.StartsWith("\"") && Tokens[0].Text.Length() >= 2)
    {
        IncludeName = Tokens[0].Text.SubString(1, Tokens[0].Text.Length() - 2);
    }
    else if (Tokens.Size() >= 3 && Tokens[0].Is("<") && Tokens.Last().Is(">"))
    {
        bSystemInclude = true;
        for (int32 Index = 1; Index < Tokens.Size() - 1; ++Index)
        {
            IncludeName += Tokens[Index].Text;
        }
    }
    else
    {
        return Error(Location, "#include expects \"FileName\" or <FileName>");
    }

    if (IncludeDepth >= MaxIncludeDepth)
    {
        return Error(Location, String::Printf("Includes are nested more than %d levels deep", MaxIncludeDepth));
    }

    // Resolve relative to the including file first, then the added include directories, then the shared shader directory
    String ResolvedPath;
    const String IncludingDirectory = File::GetDirectoryOf(FilePath);
    if (!bSystemInclude && !IncludingDirectory.IsEmpty())
    {
        const String Candidate = File::CombinePath(IncludingDirectory, IncludeName);
        if (SourceProvider->FileExists(Candidate))
        {
            ResolvedPath = Candidate;
        }
    }

    for (int32 Index = 0; Index < AdditionalIncludeDirs.Size() && ResolvedPath.IsEmpty(); ++Index)
    {
        const String Candidate = File::CombinePath(AdditionalIncludeDirs[Index], IncludeName);
        if (SourceProvider->FileExists(Candidate))
        {
            ResolvedPath = Candidate;
        }
    }

    if (ResolvedPath.IsEmpty())
    {
        const String Candidate = File::CombinePath(IncludeDir, IncludeName);
        if (SourceProvider->FileExists(Candidate))
        {
            ResolvedPath = Candidate;
        }
    }

    if (ResolvedPath.IsEmpty())
    {
        return Error(Location, String::Printf("Cannot find the include '%s'", *IncludeName));
    }

    if (!Output->Dependencies.Contains(ResolvedPath))
    {
        Output->Dependencies.Add(ResolvedPath);
    }

    if (OnceFiles.Contains(ResolvedPath))
    {
        return true;
    }

    TArray<CHAR> Text;
    if (!SourceProvider->ReadFile(ResolvedPath, Text))
    {
        return Error(Location, String::Printf("Failed to read the include '%s'", *ResolvedPath));
    }

    return ProcessFile(ResolvedPath, StringView(Text.Data(), Text.Size()), IncludeDepth + 1);
}

bool FShaderPreprocessor::EvaluateCondition(TArrayView<const FShaderToken> Tokens, const FShaderToken& Location, bool& OutValue)
{
    // 'defined' is resolved before anything is expanded, so the macro it names stays unexpanded
    TArray<FShaderToken> Resolved;
    for (int32 Index = 0; Index < Tokens.Size(); ++Index)
    {
        const FShaderToken& Token = Tokens[Index];
        if (!Token.Is("defined"))
        {
            Resolved.Add(Token);
            continue;
        }

        const bool bParenthesized = Index + 1 < Tokens.Size() && Tokens[Index + 1].Is("(");
        const int32 NameIndex = Index + (bParenthesized ? 2 : 1);
        if (NameIndex >= Tokens.Size() || Tokens[NameIndex].Type != EShaderTokenType::Identifier)
        {
            return Error(Token, "'defined' expects a macro name");
        }

        if (bParenthesized && (NameIndex + 1 >= Tokens.Size() || !Tokens[NameIndex + 1].Is(")")))
        {
            return Error(Token, "Expected ')' after 'defined(Name'");
        }

        const bool bDefined = Macros.Find(Tokens[NameIndex].Text) != nullptr;
        Resolved.Add(MakeToken(Token, bDefined ? "1" : "0", EShaderTokenType::Number, Token.bLeadingSpace));
        Index = NameIndex + (bParenthesized ? 1 : 0);
    }

    TArray<FShaderToken> Expanded;
    if (!ExpandTokens(::Move(Resolved), Expanded))
    {
        return false;
    }

    // Identifiers that are left are not macros. HLSL follows C++, so true and false keep their values.
    for (FShaderToken& Token : Expanded)
    {
        if (Token.Type == EShaderTokenType::Identifier)
        {
            Token.Text = Token.Is("true") ? "1" : "0";
            Token.Type = EShaderTokenType::Number;
        }
    }

    if (Expanded.IsEmpty())
    {
        return Error(Location, String::Printf("#%s without an expression", *Location.Text));
    }

    FConditionParser Parser(TArrayView<const FShaderToken>(Expanded.Data(), Expanded.Size()));

    int64 Value = 0;
    if (!Parser.Parse(Value))
    {
        return Error(Location, Parser.GetError());
    }

    OutValue = Value != 0;
    return true;
}

// ------------------------------------------------------------------------------------------------
// Macro expansion
// ------------------------------------------------------------------------------------------------

bool FShaderPreprocessor::ExpandTokens(TArray<FShaderToken>&& Input, TArray<FShaderToken>& OutTokens)
{
    // Each expansion is pushed as a frame and rescanned. Its macro stays disabled until the frame is used up.
    TArray<FExpansionFrame> Frames;
    Frames.Emplace().Tokens = ::Move(Input);

    const auto PopFrame = [&Frames]()
    {
        if (Frames.Last().Macro)
        {
            Frames.Last().Macro->bDisabled = false;
        }

        Frames.Pop();
    };

    const auto ReadToken = [&](FShaderToken& OutToken)
    {
        while (!Frames.IsEmpty())
        {
            FExpansionFrame& Frame = Frames.Last();
            if (Frame.Position < Frame.Tokens.Size())
            {
                OutToken = Frame.Tokens[Frame.Position++];
                return true;
            }

            PopFrame();
        }

        return false;
    };

    const auto PeekIsOpenParenthesis = [&Frames]()
    {
        for (int32 Index = Frames.Size() - 1; Index >= 0; --Index)
        {
            const FExpansionFrame& Frame = Frames[Index];
            if (Frame.Position < Frame.Tokens.Size())
            {
                return Frame.Tokens[Frame.Position].Is("(");
            }
        }

        return false;
    };

    FShaderToken Token;
    while (ReadToken(Token))
    {
        FMacro* Macro = (Token.Type == EShaderTokenType::Identifier && !Token.bNoExpand) ? Macros.Find(Token.Text) : nullptr;
        if (!Macro)
        {
            OutTokens.Add(Token);
            continue;
        }

        if (Macro->bDisabled)
        {
            Token.bNoExpand = true;
            OutTokens.Add(Token);
            continue;
        }

        TArray<FShaderToken> Replacement;
        if (Macro->bFunctionLike)
        {
            if (!PeekIsOpenParenthesis())
            {
                OutTokens.Add(Token);
                continue;
            }

            // Reading the arguments can use up frames, which enables their macros again like in C
            FShaderToken OpenParenthesis;
            ReadToken(OpenParenthesis);

            TArray<TArray<FShaderToken>> Arguments;
            Arguments.Emplace();

            int32 Depth = 0;
            while (true)
            {
                FShaderToken ArgumentToken;
                if (!ReadToken(ArgumentToken))
                {
                    return Error(Token, String::Printf("Unterminated invocation of the macro '%s'", *Token.Text));
                }

                if (ArgumentToken.Is("("))
                {
                    ++Depth;
                }
                else if (ArgumentToken.Is(")"))
                {
                    if (Depth == 0)
                    {
                        break;
                    }

                    --Depth;
                }
                else if (ArgumentToken.Is(",") && Depth == 0)
                {
                    Arguments.Emplace();
                    continue;
                }

                ArgumentToken.bStartOfLine = false;
                Arguments.Last().Add(ArgumentToken);
            }

            const bool bNoArguments = Macro->Parameters.IsEmpty() && Arguments.Size() == 1 && Arguments[0].IsEmpty();
            if (!bNoArguments && Arguments.Size() != Macro->Parameters.Size())
            {
                return Error(Token, String::Printf("The macro '%s' takes %d arguments but was given %d", *Token.Text, Macro->Parameters.Size(), Arguments.Size()));
            }

            if (!Substitute(*Macro, Arguments, Token, Replacement))
            {
                return false;
            }
        }
        else
        {
            Replacement = Macro->Body;
        }

        // Expanded code reports the line of the invocation
        for (FShaderToken& ReplacementToken : Replacement)
        {
            ReplacementToken.FileIndex      = Token.FileIndex;
            ReplacementToken.Line           = Token.Line;
            ReplacementToken.bPasteOperator = false;
        }

        if (!Replacement.IsEmpty())
        {
            Replacement[0].bLeadingSpace = Token.bLeadingSpace;
        }

        Macro->bDisabled = true;

        FExpansionFrame& Frame = Frames.Emplace();
        Frame.Tokens = ::Move(Replacement);
        Frame.Macro  = Macro;
    }

    return true;
}

bool FShaderPreprocessor::Substitute(const FMacro& Macro, const TArray<TArray<FShaderToken>>& Arguments, const FShaderToken& NameToken, TArray<FShaderToken>& OutTokens)
{
    const auto FindParameter = [&Macro](const FShaderToken& Token)
    {
        if (Token.Type == EShaderTokenType::Identifier)
        {
            for (int32 Index = 0; Index < Macro.Parameters.Size(); ++Index)
            {
                if (Macro.Parameters[Index] == Token.Text)
                {
                    return Index;
                }
            }
        }

        return -1;
    };

    // Arguments are expanded on their own first, unless they are stringized or pasted
    TArray<TArray<FShaderToken>> ExpandedArguments;
    TArray<bool>                 bIsExpanded;
    ExpandedArguments.Resize(Arguments.Size());
    for (int32 Index = 0; Index < Arguments.Size(); ++Index)
    {
        bIsExpanded.Add(false);
    }

    const TArray<FShaderToken>& Body = Macro.Body;

    TArray<FShaderToken> Substituted;
    for (int32 Index = 0; Index < Body.Size(); ++Index)
    {
        const FShaderToken& BodyToken = Body[Index];

        if (BodyToken.Is("#") && Index + 1 < Body.Size() && FindParameter(Body[Index + 1]) >= 0)
        {
            const TArray<FShaderToken>& Argument = Arguments[FindParameter(Body[Index + 1])];

            const String Joined = JoinTokens(TArrayView<const FShaderToken>(Argument.Data(), Argument.Size()));

            String Text = "\"";
            for (int32 CharIndex = 0; CharIndex < Joined.Length(); ++CharIndex)
            {
                if (Joined[CharIndex] == '"' || Joined[CharIndex] == '\\')
                {
                    Text += '\\';
                }

                Text += Joined[CharIndex];
            }

            Text += '"';

            FShaderToken& Stringized = Substituted.Emplace(MakeToken(NameToken, "", EShaderTokenType::String, BodyToken.bLeadingSpace));
            Stringized.Text = Text;
            ++Index;
            continue;
        }

        const int32 Parameter = FindParameter(BodyToken);
        if (Parameter < 0)
        {
            Substituted.Add(BodyToken);
            continue;
        }

        const bool bPasted = (Index > 0 && Body[Index - 1].bPasteOperator) || (Index + 1 < Body.Size() && Body[Index + 1].bPasteOperator);
        if (!bPasted && !bIsExpanded[Parameter])
        {
            TArray<FShaderToken> ArgumentCopy = Arguments[Parameter];
            if (!ExpandTokens(::Move(ArgumentCopy), ExpandedArguments[Parameter]))
            {
                return false;
            }

            bIsExpanded[Parameter] = true;
        }

        const TArray<FShaderToken>& Replacement = bPasted ? Arguments[Parameter] : ExpandedArguments[Parameter];
        if (Replacement.IsEmpty())
        {
            // A placemarker, so pasting with an empty argument keeps the other side
            if (bPasted)
            {
                Substituted.Emplace(MakeToken(NameToken, "", EShaderTokenType::Other, false));
            }

            continue;
        }

        const int32 FirstIndex = Substituted.Size();
        Substituted.Append(Replacement);
        Substituted[FirstIndex].bLeadingSpace = BodyToken.bLeadingSpace;

        for (int32 ReplacementIndex = FirstIndex; ReplacementIndex < Substituted.Size(); ++ReplacementIndex)
        {
            Substituted[ReplacementIndex].bPasteOperator = false;
        }
    }

    for (int32 Index = 0; Index < Substituted.Size(); ++Index)
    {
        const FShaderToken& Token = Substituted[Index];
        if (!Token.bPasteOperator)
        {
            OutTokens.Add(Token);
            continue;
        }

        // HandleDefine rejects '##' at either end, so both sides exist
        FShaderToken& Left  = OutTokens.Last();
        const FShaderToken& Right = Substituted[++Index];

        if (Right.Text.IsEmpty())
        {
            continue;
        }

        if (Left.Text.IsEmpty())
        {
            Left = Right;
            continue;
        }

        const String Pasted = Left.Text + Right.Text;

        TArray<FShaderToken> PastedTokens;
        Tokenize(StringView(Pasted), NameToken.FileIndex, PastedTokens);
        if (PastedTokens.Size() != 1)
        {
            return Error(NameToken, String::Printf("Pasting '%s' and '%s' does not give a valid token", *Left.Text, *Right.Text));
        }

        const bool bLeadingSpace = Left.bLeadingSpace;
        Left = PastedTokens[0];
        Left.bLeadingSpace = bLeadingSpace;
        Left.bStartOfLine  = false;
    }

    // Drop the placemarkers that are left
    for (int32 Index = OutTokens.Size() - 1; Index >= 0; --Index)
    {
        if (OutTokens[Index].Type == EShaderTokenType::Other && OutTokens[Index].Text.IsEmpty())
        {
            OutTokens.RemoveAt(Index);
        }
    }

    return true;
}

const FDiskShaderSourceProvider& FDiskShaderSourceProvider::Get()
{
    static FDiskShaderSourceProvider Instance;
    return Instance;
}

bool FDiskShaderSourceProvider::FileExists(const String& Path) const
{
    return FPlatformFile::IsFile(*Path);
}

bool FDiskShaderSourceProvider::ReadFile(const String& Path, TArray<CHAR>& OutText) const
{
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(Path);
    return FileHandle && File::ReadTextFile(FileHandle.Get(), OutText);
}
