## Code Standard

This document is the authority on how code in the project is written. It covers naming, formatting, file layout, and the small conventions that repeat across modules: the things you cannot infer reliably from a single file or from typical C++ style guides alone.

The engine spans several platforms and graphics APIs. A shared standard keeps diffs readable, makes review faster, and lets you open an unfamiliar module without re-learning local habits. When a rule and the surrounding code disagree, follow the rule for new or touched code. Match the surrounding code only when you are not changing that area.

The sections below are grouped by topic. Prefer the most specific rule that applies. For example, use **Classes** over general naming, or **File Layout** when deciding where a helper belongs in a `.cpp` file.

### Contents
- [Brace Style](#brace-style)
- [If Statements](#if-statements)
- [Loops](#loops)
- [Function Signatures](#function-signatures)
- [General Code](#general-code)
- [Templates](#templates)
- [Classes](#classes)
- [Structs](#structs)
- [Core Value Types](#core-value-types)
- [Static-Only Classes](#static-only-classes)
- [Interfaces](#interfaces)
- [Enums](#enums)
- [Union](#union)
- [Virtual](#virtual)
- [Naming Conventions](#naming-conventions)
- [File Layout](#file-layout)
- [Platform Specific Code](#platform-specific-code)

### Brace Style
* The project uses **Allman style** braces. The opening `{` goes on its own line, aligned with the statement it belongs to: an `if`, a `for`, a function name, a `class`, and so on. The closing `}` lines up with that same statement.
* `else`, `else if`, and `catch` follow the same rule. Each keyword starts on its own line, then the opening brace on the next line.

```cpp
if (condition)
{
    // Statements
}
else if (otherCondition)
{
    // Statements
}
else
{
    // Statements
}
```

### If Statements
If statements follow the Allman brace style:

```cpp
if (condition)
{
  // Statements
}
else
{
  // Statements
}
```

* Avoid single-line if-statements. This is to minimize the amount of bugs.

* Conditional (ternary) operators are allowed.

```cpp
condition ? true : false;
```

* Group unconditional statements together. When some steps depend on optional state, put each optional case in its own `if` block rather than interleaving optional and unconditional code.

* Separate optional blocks from unconditional code with blank lines. Put a blank line before the first optional block, between consecutive optional blocks, and after the last optional block before the next unconditional statement.

Good:

```cpp
PassBuilder.ReadTexture(Context.GBufferAlbedo, ERHIResourceState::NonPixelShaderResource);
PassBuilder.ReadTexture(Context.GBufferNormal, ERHIResourceState::NonPixelShaderResource);

if (Context.RayTracingOutput)
{
    PassBuilder.ReadTexture(Context.RayTracingOutput, ERHIResourceState::NonPixelShaderResource);
}

if (Context.IntegrationLUT)
{
    PassBuilder.ReadTexture(Context.IntegrationLUT, ERHIResourceState::NonPixelShaderResource);
}

PassBuilder.WriteTexture(Context.SceneTarget, ERHIResourceState::UnorderedAccess);
```

Bad:

```cpp
PassBuilder.ReadTexture(Context.GBufferAlbedo, ERHIResourceState::NonPixelShaderResource);
if (Context.RayTracingOutput)
{
    PassBuilder.ReadTexture(Context.RayTracingOutput, ERHIResourceState::NonPixelShaderResource);
}
PassBuilder.ReadTexture(Context.GBufferNormal, ERHIResourceState::NonPixelShaderResource);
if (Context.IntegrationLUT)
{
    PassBuilder.ReadTexture(Context.IntegrationLUT, ERHIResourceState::NonPixelShaderResource);
}
PassBuilder.WriteTexture(Context.SceneTarget, ERHIResourceState::UnorderedAccess);
```

### Loops
Loops (`for`, `while`, `do while`) follow the same Allman brace style:

```cpp
while (condition)
{
  // Statements
}
```

```cpp
for (int32 Index = 0; Index < SomeNumber; ++Index)
{
  // Statements
}
```

```cpp
do
{
  // Statements
} while (condition)
```

* Avoid single-line loops. This is to minimize the amount of bugs.

### Function Signatures
* A signature that would run past roughly 140 columns is written with one parameter per line. The types are padded into a column of their own and so are the default values, which lets the parameter list be read top to bottom:

```cpp
NODISCARD static FRHITextureDesc CreateTexture1D(
    EFormat                       InFormat,
    uint32                        InWidth,
    uint32                        InNumMipLevels,
    ETextureUsageFlags            InUsageFlags,
    const FClearValue&            InClearValue   = FClearValue(),
    ERHIResourceStateTrackingMode InTrackingMode = ERHIResourceStateTrackingMode::Manual)
{
}
```

* The opening parenthesis ends the name line, the parameters are indented one level, and the closing parenthesis stays on the last parameter together with any trailing `noexcept` or `const`.

```cpp
bool IsSomeConditionMet(
    ESomeMode InMode,
    int32     InCount) const
{
}

void SwapValues(
    int32& OutLeft,
    int32& OutRight) noexcept
{
}
```

* Constructors are written the same way, and their initializer list keeps its usual indentation below the signature. Each member initializer stays on one line and is not broken across lines, even when it runs past roughly 140 columns.

```cpp
FVulkanMemoryManager::FVulkanMemoryManager(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , BufferAllocator(InDevice, static_cast<uint64>(CVarBufferAllocatorPageSize.GetValue()) * 1024ull * 1024ull, BUFFER_MIN_BLOCK, static_cast<uint64>(CVarBufferAllocatorMaxSuballocationSize.GetValue()) * 1024ull * 1024ull)
    , UploadMemoryTypeIndex(0)
{
}
```

* A signature that fits within roughly 140 columns stays on one line when it stands alone. Overloads and factory helpers on the same type share one wrapping style: if any member of the family would need the multi-line parameter layout above, write every member that way, including shorter siblings that could fit on one line by themselves.

```cpp
NODISCARD static FRHITextureDesc CreateTexture1D(
    EFormat                       InFormat,
    uint32                        InWidth,
    uint32                        InNumMipLevels,
    ETextureUsageFlags            InUsageFlags,
    const FClearValue&            InClearValue   = FClearValue(),
    ERHIResourceStateTrackingMode InTrackingMode = ERHIResourceStateTrackingMode::Manual);

NODISCARD static FRHITextureDesc CreateTexture2DArray(
    EFormat                       InFormat,
    uint32                        InWidth,
    uint32                        InHeight,
    uint32                        InArraySlices,
    uint32                        InNumMipLevels,
    uint32                        InNumSamples,
    ETextureUsageFlags            InUsageFlags,
    const FClearValue&            InClearValue   = FClearValue(),
    ERHIResourceStateTrackingMode InTrackingMode = ERHIResourceStateTrackingMode::Manual);
```

* The same ~140-column limit applies to string literals. Break long text across adjacent `"..."` tokens and the compiler concatenates them into one literal. Keep the call and opening quote on one line, indent continuation lines, and leave any format arguments outside the string:

```cpp
LOG_WARNING("[RayTracingReflections]: Renderer.RayTracing.SER=1 requested but Shader Execution Reordering is unsupported on this backend. "
    "Falling back to the non-SER reflection path");

LOG_ERROR("PointLightShadows multi-pass render requires PointLightShadowMapFaceDSVs[%u], but it was not resolved",
    ArrayIndex);
```


* Braced initializer lists on assignment put both braces on their own lines so the list can grow without reformatting the assignment:

```cpp
PSODesc.HitGroups =
{
    FRHIRayTracingHitGroupInfo("HitGroup", ERayTracingHitGroupType::Triangles, { ClosestHitShader.Get() })
};
```

* This applies to braced initializer lists with one or more elements. It does not apply to empty default initialization such as `= {}` or `= { 0 }`.

```cpp
TArray<FItem> Items =
{
    ItemA,
    ItemB,
};

VkBufferCreateInfo BufferCreateInfo = {};
VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = {};
SomeStruct Desc = { 0 };
```

### General Code
* Put a blank line between a run of function-call statements and the next local variable declaration.

Good:

```cpp
FPassResources Resolved = Context.CreatePassResources(PassResources);
SyncPassResourcesToFrameResources(Resolved, *Context.FrameResources);

FRHITexture* ResolvedOutput = OutputTarget ? PassResources.Get(OutputGraphTexture) : PassResources.Get(OutputGraphTexture);
Record(PassCommandList, *Context.FrameResources, ResolvedOutput, bOutputSRGB);
```

Bad:

```cpp
FPassResources Resolved = Context.CreatePassResources(PassResources);
SyncPassResourcesToFrameResources(Resolved, *Context.FrameResources);
FRHITexture* ResolvedOutput = OutputTarget ? PassResources.Get(OutputGraphTexture) : PassResources.Get(OutputGraphTexture);
Record(PassCommandList, *Context.FrameResources, ResolvedOutput, bOutputSRGB);
```

### Templates

Templates should use the following style. The class name uses a capital `T` prefix, and `typename` is preferred over `class` for type parameters.

```
template<typename T>
class TMyClass
{
};

template<typename T>
void SomeFunc()
{
}
```

When forward declaring a template the following style should be used:

```
// Correct
template<typename InvokableType>
class TFunction;

// Wrong
template<typename T> class TFunction;
```

### Classes
* Classes should be written using the following style:

```cpp
class FMyClass
{
public:
    enum class ESomeMode : uint8
    {
        Default,
        Other,
    };

    struct FSomeOptions
    {
        int32 SomeCount = 0;
    };

    static constexpr int32 MaxSomeCount = 128;

    static bool IsSomeModeValid(ESomeMode InMode);

public:
    FMyClass();
    ~FMyClass();

    void SomeFunction();
    bool IsSomeConditionMet() const;

protected:
    static bool SomeProtectedStaticFunction();

    void SomeProtectedFunction();

    int32 SomeProtectedVariable = 0;

private:
    static FMyClass* Instance;

    bool SomePrivateFunction();

    int32 SomeVariable = 0;
    bool  bSomeFlag    = false;
};
```

* Within each access section (`public`, `protected`, `private`), members are ordered as follows:

  1. Nested enums, structs, and typedefs.
  2. Static member functions and static member variables.
  3. Constructors, destructor, and other instance member functions.
  4. Instance member variables.

  Nested types stay with the static group. In the `public` section only, repeat `public` between the static members and the instance members so the two groups are easy to scan apart.

* Class sections themselves are ordered `public`, then `protected`, then `private`.

* Classes use 'F' as prefix, with the exception of core value types (see [Core Value Types](#core-value-types)).

* Include headers in `.cpp` files. A header includes another header only when the full type is needed beyond a pointer or reference. Otherwise forward-declare.

* Classes that are meant to be used outside the engine- project, and are not header-only should use the export macro before the class-name.

```cpp
class CORE_API FMyClass
{
};
```

* The export macro depends on the engine module it belongs to, for example:

```cpp
// Core engine module
#define CORE_API ...

// RHI layer
#define RHI_API ...

// etc.
```

### Structs
* Structs should be used when all members are public.

* Structs use 'F' as prefix (except core value types, see [Core Value Types](#core-value-types)), for example:

```cpp
struct FMyStruct
{
  int32 x;
  int32 y;
};
```

### Core Value Types
* Core string, math, and atomic `value` types are used **without** the `F` prefix and keep their bare name. These are the small, ubiquitous value types in `Runtime/Core/` (strings, vectors, matrices, quaternion, plane, atomic aliases).

```cpp
// Correct - core value types are prefix-free
String   Name;
Vector3  Position;
Matrix4  ViewProjection;
AtomicInt32 RefCount;
```

* Examples: `String`, `StringView`, `CString`, `Vector2/3/4`, `IntVector2/3/4`, `Matrix2/3/4`, `Matrix3x4`, `Quaternion`, `Plane`, `AtomicBool`, `AtomicInt32`.

* Non-core gameplay/engine structs and classes still use the `F` prefix (for example `FCameraConstants`, `FViewportRegion`, `FRectangle`). The color types `FColor`/`FFloatColor`/`FFloatColor16` also keep the `F` prefix. Only the core value types listed above drop it.

* The platform-abstraction / SIMD layer keeps the `F` prefix (for example `FPlatformString`, `FPlatformMath`, `FPlatformVectorMathSSE`, `FPlatformVectorMathNEON`, `FInt128`, `FFloat128`). The interfaces those implementations satisfy are named by the [Interfaces](#interfaces) rule instead, so `IPlatformString` and `IPlatformVectorMath` take the `I` prefix while the `FPlatformX` typedefs that select between them keep the `F`.

* Underlying templates keep their `T` prefix (`TString`, `TStringView`, `TStaticString`, `TCString`, `TAtomicInt`, `TAtomicEnum`); only the `F` aliases over them lose the prefix. Interfaces keep `I` and enums keep `E` as usual.

### Static-Only Classes
* A struct/class that contains only static members (no instance state, no virtuals) acts as a namespace. Drop the `F` prefix and give it a plural or semantic name that reads like a namespace at the call site.

```
// Correct - reads like a namespace
struct Memory
{
  static void* Malloc(uint64 Size);
};

struct Bits
{
  template< typename T >
  static constexpr T ReverseBits(T Value);
};

Memory::Malloc(Size);
Bits::ReverseBits(Value);
```

* This applies to utility/helper "namespace" types only. Types with instance state or that are singletons (for example `FConsoleManager`, `FModuleManager`) keep the `F` prefix.

* Class templates keep the `T` prefix even when they only hold helpers, because the `T` prefix communicates that the type is a template (for example `TInlineStorage`).

* A struct that fakes a namespace of constants with `static const` members (rather than an `enum`) follows the same rule - drop the `E`/`F` prefix and treat it as a namespace (for example `Keys` exposing `Keys::W`). This is distinct from the enum-scoping idiom in [Enums](#enums), which keeps its `E` prefix.

* The summary table:

| Category | Prefix rule | Example |
| --- | --- | --- |
| Static-only "namespace" struct/class | Drop `F`, use a plural/semantic name | `Bits`, `Memory`, `CommandLine` |
| Class template (helpers, but still a template) | `T` prefix | `TInlineStorage` |
| Stateless interface (polymorphic or static-only) | `I` prefix | `IModule`, `IPlatformInputMapper` |
| `struct E { static const ... };` (constants masquerading as an enum) | Drop the prefix, treat as a namespace | `Keys` |
| `struct E { enum Type { ... }; };` (enum-scoping idiom) | Keep `E` | `EKeyName`, `EShaderVisibility` |

### Interfaces
* An interface contains no state (i.e no variables). State belongs in the implementations.

* The rule is about per-instance state. A `static` member is allowed where it is one piece of process-wide machinery that every implementation shares rather than something each instance carries.

* Interfaces should use the capital letter 'I' as prefix.

* An interface's functions take one of three forms. Pure virtual is the default. Every implementation supplies the behavior:

```cpp
struct IMyInterface
{
  virtual ~IMyInterface() = default;
  
  virtual void Func() = 0;
};
```

* A virtual function may provide a meaningful default body instead. The default does real work rather than acting as a placeholder. For example, `IPlatformApplicationMessageHandler` returns `false` from every handler, so an implementation overrides only the events it cares about.

```cpp
struct IPlatformApplicationMessageHandler
{
  virtual ~IPlatformApplicationMessageHandler() = default;

  virtual bool OnKeyDown(EKeyboardKeyName::Type KeyCode, bool bIsRepeat, FModifierKeyState ModifierKeyState)
  {
    return false;
  }

  virtual bool OnMouseMove(int32 MouseX, int32 MouseY)
  {
    return false;
  }
};

struct FMyMessageHandler : IPlatformApplicationMessageHandler
{
  virtual bool OnKeyDown(EKeyboardKeyName::Type KeyCode, bool bIsRepeat, FModifierKeyState ModifierKeyState) override
  {
    return true;
  }
};
```

* Static functions are the third form. They define a compile-time platform contract: implementations hide the stubs, a typedef such as `FPlatformInputMapper` selects the platform, and calls resolve at compile time. `IPlatformInputMapper` and `IPlatformApplicationMisc` work this way. This form takes precedence over the static-only naming rule in [Static-Only Classes](#static-only-classes).

```cpp
struct IPlatformInputMapper
{
  static FORCEINLINE EKeyboardKeyName::Type GetKeyCodeFromScanCode(uint32 ScanCode)
  {
    return EKeyboardKeyName::Unknown;
  }
};

class FWindowsInputMapper final : public IPlatformInputMapper
{
public:
  static FORCEINLINE EKeyboardKeyName::Type GetKeyCodeFromScanCode(uint32 ScanCode)
  {
    return KeyCodeFromScanCodeTable[ScanCode];
  }
};

// PlatformInputMapper.h
#if PLATFORM_WINDOWS
  #include "CoreApplication/Windows/WindowsInputMapper.h"
  typedef FWindowsInputMapper FPlatformInputMapper;
#else
  #include "CoreApplication/PlatformInterface/IPlatformInputMapper.h"
  typedef IPlatformInputMapper FPlatformInputMapper;
#endif

EKeyboardKeyName::Type Key = FPlatformInputMapper::GetKeyCodeFromScanCode(ScanCode);
```

* Interfaces should use a virtual destructor if the interface will be deleted as the instance-type.

* Prefer structs over classes. Since everything most likeley will be public anyway.

### Enums
* Enums should use the capital letter 'E' as prefix.

* `enum class` is the default and should be used unless implicit conversion to an integer is required. Struct-wrap the enum when it must behave as an integer:
  * Array indexing or array sizing.

```cpp
struct ESlot
{
  enum Type : uint8
  {
    A = 0,
    B,
    Count,
  };
};

int32 Table[ESlot::Count];
Table[ESlot::B] = 1;
```

  * Arithmetic on enum values.

```cpp
struct ESlot
{
  enum Type : uint8
  {
    A = 0,
    B,
    Count,
  };
};

ESlot::Type Next = ESlot::B + 1;
int32 Offset = Index - ESlot::A;
```

  * Bitwise shifts.

```cpp
struct ESlot
{
  enum Type : uint8
  {
    A = 0,
    B,
    Count,
  };
};

uint32 Mask = 1u << ESlot::B;
```

  * Bitwise OR/AND on values without `ENUM_CLASS_OPERATORS`.

```cpp
struct EAccess
{
  enum Type : uint8
  {
    None  = 0,
    Read  = 1 << 0,
    Write = 1 << 1,
  };
};

EAccess::Type Access = EAccess::Read | EAccess::Write;
```

  * Other APIs that take an integer where `enum class` would require an explicit cast at every call site.

```cpp
struct ESlot
{
  enum Type : uint8
  {
    A = 0,
    B,
    Count,
  };
};

void BindSlot(int32 Slot);
BindSlot(ESlot::B);
```

* Cases that look like they need int conversion but actually do *not* (use `enum class`):
  * Template non-type parameters (an enum type as the template parameter) work fine with `enum class`.
  * Tag dispatch. The enum value tells the function which path to take, but is not used as an integer itself.

```cpp
enum class EFillMode : uint8
{
  Zero,
  Pattern,
};

void FillBuffer(uint8* Dst, int32 Size, EFillMode Mode, uint8 Pattern = 0)
{
  if (Mode == EFillMode::Zero)
  {
    Memory::Memzero(Dst, Size);
  }
  else
  {
    Memory::Memset(Dst, Pattern, Size);
  }
}

FillBuffer(Buffer, Size, EFillMode::Zero);
```

  * Bitfield members (`enum class` as a bitfield is valid C++11+).
  * Equality and ordering comparisons.

* Enum classes with flags can use the 'ENUM_CLASS_OPERATORS' macro to make operations such as 'and', 'or' etc to work.

* Example of the default `enum class` form:

```cpp
enum class EMyEnum : uint8
{
  Car   = 0,
  Apple = 1,
};
ENUM_CLASS_OPERATORS(EMyEnum);
```

* When implicit conversion to an integer is required, the enum must be wrapped in a struct that exposes an inner `enum Type`. This makes the enum a typed name while still allowing the inner enumerators to convert to integers:

```cpp
struct EMyEnum
{
  enum Type : uint8
  {
    Something0 = 0,
    Something1 = 1,
  };
};
```

* Note that enumerators in the wrapped form do **not** repeat the enum name as a prefix - the wrapping struct already provides the namespace. Use `EMyEnum::Something0` at the call site, and `EMyEnum::Type` when naming the type of a variable, parameter, return value or template non-type parameter.

#### Enum Formatting
* The underlying type is always explicit. Pick the smallest type that holds every value, which is `uint8` for most enums. Flag enums need enough bits for the highest flag, so an enum reaching `FLAG(14)` needs `uint16` rather than `uint8`.

* A value is spelled out when the value itself carries information: flags (`FLAG(n)`, `(1 << n)`), a mapping onto an external API or file format, a deliberate gap, or a first enumerator anchored somewhere other than the start. Numbering a plain sequence is allowed but never required, since the numbers only restate the position:

```cpp
// Fine - the positions are arbitrary, so the numbers would add nothing
enum class EActivePipeline : uint8
{
    Graphics,
    Compute,
    Meshlet,
    RayTracing,
};

// Spelled out because the values mirror D3D12_HEAP_TYPE
enum class EHeapType : uint8
{
    Default  = 1,
    Upload   = 2,
    ReadBack = 3,
};
```

* Whichever form an enum already uses, stay with it. Do not add numbers to an unnumbered sequence, and do not strip them from a numbered one.

* An enumerator that needs documenting gets a `/** */` comment on the line above, never a trailing `//`. The documented enumerator is separated from its neighbours by a blank line:

```cpp
enum class EMyEnum : uint8
{
    /** Described by a D3D12_UNORDERED_ACCESS_VIEW_DESC, created via CreateUnorderedAccessView */
    Standard = 0,

    /** Described by a feedback/paired resource pair, created via CreateSamplerFeedbackUnorderedAccessView */
    SamplerFeedback = 1,
};
```

* Enumerators that are self-explanatory are left undocumented. Runs of undocumented enumerators stay packed together with their values column-aligned:

```cpp
enum class EMyFlags : uint8
{
    None = 0,

    /** The transition starts here and must be completed by a matching EndOnly */
    BeginOnly = FLAG(0),

    /** Completes a transition opened by a matching BeginOnly */
    EndOnly = FLAG(1),

    Texture = FLAG(2),
    Buffer  = FLAG(3),
};
```

* The last enumerator carries a trailing comma.

* Enum bodies are indented with 4 spaces (8 when the enum is nested inside a type).

### Union
* Unions use 'F' as prefix, for example:

```cpp
union FMyUnion
{
  int32 x;
  float y;
};
```

### Virtual
* Should be used when runtime polymorphism is needed.

* Prefer functions that can be determined at compile-time due to performance.

### Naming Conventions
* For the most part, variables use PascalCase. They normally do not use prefixes and should use the following style:

```cpp
// Correct
int* MyPointer  = nullptr;

// Wrong
int* pMyPointer = nullptr;
```

* This applies to classes and structures as well.

```cpp
class FMyClass
{
private:
  int  MyInteger = 0;
  int* Pointer   = nullptr;
 };
```

* Global variables should have 'G' as a prefix:

```cpp
bool GMyGlobal;
```

* The `G` prefix marks file-scope globals. For a singleton whose instance is a `static` member of its own class, a common pattern is to name the instance after the type without the `F` prefix (for example `FEngine::Engine`, `FTaskGraph::TaskGraph`, `FShaderCompiler::ShaderCompiler`).

```cpp
class FShaderCompiler
{
public:
  static FShaderCompiler& Get()
  {
    return *ShaderCompiler;
  }

private:
  static FShaderCompiler* ShaderCompiler;
};
```

* Global / namespace-scope / file-scope constants — `constexpr` values defined outside any class or function body — use `UPPER_SNAKE_CASE`:

```cpp
constexpr uint64 PSO_KEY_BINDLESS_BIT = uint64(1) << 32;
constexpr int32  MAX_SHADOW_CASCADES  = 4;
```

* Prefer `constexpr` over `#define` for value constants so the type is preserved and the symbol is visible to the debugger. Use `#define` only for preprocessor switches, include guards, or token pasting where a value constant cannot do the job.

* Class / struct member constants (`static constexpr` inside a type) and local `constexpr` variables follow the usual PascalCase / member naming rules. Only file or namespace scope uses `UPPER_SNAKE_CASE`.
* Mathematical components such as x, y, z etc. should **NOT** be capitalized.

```cpp
struct FMyMathStruct
{
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};
```

* Functions use pascalcase. Same with parameters and local variables.

```cpp
void SetBufferSize(int BufferSize)
{
  int AlignedSize = BufferSize;
}
```

* Memberfunctions also use pascalcase. Parameters with same name as a member variable should use the 'In' or 'New' prefix:

```cpp
class FMyClass
{
public:
  void MyMemberFunction(int InVariable)
  {
    Variable = InVariable;
  }

private:
  int Variable = 0;
};
```

* A function that builds and returns a value is named `Create`. The exceptions are the `Core` container helpers modelled on the standard library, `MakeShared`, `MakeUnique`, `MakePair` and `MakeArrayView`, and functions that mirror a platform API call, such as `MakeResident`.

```cpp
// Correct
FRHITextureSubresourceRange CreateMip(uint32 MipLevel);

// Wrong
FRHITextureSubresourceRange MakeMip(uint32 MipLevel);
```

### File Layout
* A `.cpp` is read from the top, so everything that belongs to the file rather than to the class comes first, in this order: includes, console variables and commands, enums, structs, `static` helper functions, then the rest of the file.
* A function or type used only in one `.cpp` belongs in that block rather than next to whichever function uses it, and a helper function is `static` so that it stays in the translation unit. A constant that exists only for one helper travels with it.
* The order gives way only where a declaration is needed earlier: a console variable whose default value names an enum has to be declared below that enum.

```cpp
// MyPass.cpp

#include "Renderer/MyPass.h"

bool GMyPassEnabled = true;
static FAutoConsoleVariableRef CVarMyPassEnabled( ... );

enum class EMyPassMode : uint8
{
  Off,
  On,
};

struct FMyPassConstantsHLSL
{
  Matrix4 Transform;
  uint32  Mode;
};

class FMyPassCS
{
  DECLARE_SHADER_TYPE(FMyPassCS, EShaderStage::Compute);
};

IMPLEMENT_SHADER_TYPE(FMyPassCS, "Shaders/MyPass.hlsl", "Main", EShaderModel::SM_6_2);

constexpr uint32 TILE_SIZE = 16;

static uint32 GetDispatchSize(uint32 Extent)
{
  return Math::DivideByMultiple(Extent, TILE_SIZE);
}

void FMyPass::Record(FRHICommandList& CommandList)
{
  // Statements
}
```

### Platform Specific Code
* Platform specific code should be kept in seperate directories with the platform name. `PlatformInterface` holds the interfaces the platforms implement, and `Platform` holds the headers that select between them.

```cpp
Core/PlatformInterface/
Core/Platform/
Core/Mac/
Core/Windows/

CoreApplication/PlatformInterface/
CoreApplication/Platform/
CoreApplication/Mac/
CoreApplication/Windows/
```

* Classes specific to platform should be prefixed with platform-name.

* The interface is named for the concept, prefixed with `IPlatform`, and follows the rules in [Interfaces](#interfaces): no state, and its functions either pure virtual or static.

```cpp
struct IPlatformApplication
{
  virtual ~IPlatformApplication() = default;

  virtual void Func() = 0;
};

class FMacApplication final : public IPlatformApplication
{
public:
  virtual void Func() override final;
};

class FWindowsApplication final : public IPlatformApplication
{
public:
  virtual void Func() override final;
};
```

* Platform classes should be accompanied with a Platform-header like this:

```cpp
// PlatformApplication.h

#pragma once
#if PLATFORM_WINDOWS
  #include "CoreApplication/Windows/WindowsApplication.h"
  typedef FWindowsApplication FPlatformApplication;
#elif PLATFORM_MACOS
  #include "CoreApplication/Mac/MacApplication.h"
  typedef FMacApplication FPlatformApplication;
#else
  #include "CoreApplication/PlatformInterface/IPlatformApplication.h"
  typedef IPlatformApplication FPlatformApplication;
#endif
```

* The fallback branch names the interface: include its header and typedef it, so `FPlatformX` resolves to a type in every branch. This holds for both kinds of interface. A compile-time one is usable there as it stands, since its stubs are callable, which is how `PlatformInputMapper.h` and `PlatformMisc.h` work. A pure virtual one still typedefs cleanly, because naming an abstract type is legal and only instantiating one is not, so an unported platform fails where it first tries to construct the type rather than at the include. `PlatformApplication.h` and `PlatformThread.h` are of that second kind. That division also makes adding a new platform easier: an in-progress port can compile cleanly even when implementation is still incomplete, and gaps show up at first use rather than blocking the whole build at once.
