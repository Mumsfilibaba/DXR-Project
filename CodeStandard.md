## Code Standard

#### Contents
- [Code Standard](#code-standard)
    - [Contents](#contents)
  - [If Statements](#if-statements)
  - [Loops](#loops)
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
  - [Platform Specific Code](#platform-specific-code)

### If Statements
* If statements should be written using the following style:

```
if (condition)
{
  // Statements
}
else
{
  // Statements
}
```
* Avoid single-line if-statements. This is to minimize the amount of bugs in the application.

* Conditional (ternary) operator are allowed
```
condition ? true : false;
```

### Loops
* Loops (for, while, do while) should be written using the following style:

```
while (condition)
{
  // Statements
}
```
```
for (int32 Index = 0; Index < SomeNumber; ++Index)
{
  // Statements
}
```
```
do
{
  // Statements
} while (condition)
```

* Avoid single-line loops. This is to minimize the amount of bugs in the application.

### Templates
* Templates should use the following style:
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
* Note that the classname is using capital letter 'T' as prefix

* Note the use of typename instead of class, typename is preferred over class

* When forward declaring a template the following style should be used:

```
// Correct
template<typename InvokableType>
class TFunction;

// Wrong
template<typename T> class TFunction;
```

### Classes
* Classes should be written on using the following style:
```
class FMyClass
{
  // Private constructor and destructor
  FMyClass();
  ~FMyClass();

public:
  // Public functions and variable here ...

protected:
  // Protected functions and variable here ...

private:
  // Protected functions and variable here ...
};
```
* Classes use 'F' as prefix, with the exception of core value types (see [Core Value Types](#core-value-types))

* Note the order of the access modifers

* Note that the constructor is kept on top even though the private accessor is on the bottom. This is to increase readability. Try to keep constructors in the top. 

* Header files should be included in compilation units (.cpp). Only reason to include in the header is when a class/struct is using the type more than just a pointer or reference, such as calling a function, accessing a variable or declaring the type directly. This means that forward declarations has to and should be used.

* Classes that are meant to be used outside the engine- project, and are not header-only should use the export macro before the class-name
```
class CORE_API MyClass
{
};
```

* The export macro depend on the engine module it belongs to for example
```
// Core engine module
#define CORE_API ...

// RHI layer
#define RHI_API ...

// etc.
```

### Structs
* Structs should be used when all members are public

* Structs use 'F' as prefix (except core value types, see [Core Value Types](#core-value-types)), for example

```
struct FMyStruct
{
  int32 x;
  int32 y;
};
```

### Core Value Types
* Core string, math, and atomic *value* types are used **without** the `F` prefix and keep their bare name. These are the small, ubiquitous value types in `Runtime/Core/` (strings, vectors, matrices, quaternion, plane, atomic aliases), used so pervasively that the prefix only adds noise.

```
// Correct - core value types are prefix-free
String   Name;
Vector3  Position;
Matrix4  ViewProjection;
AtomicInt32 RefCount;
```

* Examples: `String`, `StringView`, `CString`, `Vector2/3/4`, `IntVector2/3/4`, `Matrix2/3/4`, `Matrix3x4`, `Quaternion`, `Plane`, `AtomicBool`, `AtomicInt32`.

* Non-core gameplay/engine structs and classes still use the `F` prefix (for example `FCameraConstants`, `FViewportRegion`, `FRectangle`). The color types `FColor`/`FFloatColor`/`FFloatColor16` also keep the `F` prefix. Only the core value types listed above drop it.

* The platform-abstraction / SIMD layer keeps the `F` prefix (for example `FPlatformString`, `FPlatformMath`, `FGenericPlatformVectorMath`, `FInt128`, `FFloat128`).

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
  template<typename T>
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
| Static-only "namespace" struct/class | Drop `F`, use a plural/semantic name | `FBitHelper` -> `Bits`, `FMemory` -> `Memory`, `FCommandLine` -> `CommandLine` |
| Class template (helpers, but still a template) | `T` prefix | `FInlineStorage` -> `TInlineStorage` |
| Stateless interface (polymorphic or static-only) | `I` prefix | `FModuleInterface` -> `IModule`, `EGenericInputMapper` -> `IPlatformInputMapper` |
| `struct E { static const ... };` (constants masquerading as an enum) | Drop the prefix, treat as a namespace | `EKeys` -> `Keys` |
| `struct E { enum Type { ... }; };` (enum-scoping idiom) | Keep `E` | `EKeyName`, `EShaderVisibility` (unchanged) |

### Interfaces
* An interface contains no state (i.e no variables). That, and not pure virtuality, is what makes a type an interface. State belongs in the implementations, so an interface that needs to expose it declares an accessor and lets each implementation hold the member.

* Interfaces should use the capital letter 'I' as prefix.

* All functions pure virtual is the usual form, and the one to reach for by default:

```
struct IMyInterface
{
  virtual ~IMyInterface() = default;
  
  virtual void Func() = 0;
}
```

* It is not the only form. Two variations are still interfaces and still take the `I` prefix:
  * A function may keep a default body where the default is meaningful rather than a placeholder. `IPlatformApplicationMessageHandler` returns `false` from every handler, meaning "not handled, keep propagating", so an implementation only overrides the events it cares about.
  * The functions may be `static`, which makes the interface a compile-time one: implementations hide the stubs instead of overriding them, and a typedef such as `FPlatformInputMapper` picks the implementation, so calls resolve at compile time. `IPlatformInputMapper` and `IPlatformApplicationMisc` work this way. This case takes precedence over the static-only rule in [Static-Only Classes](#static-only-classes), which drops the prefix: those types name a namespace of helpers, an interface names a contract that platforms implement.

* Interfaces should use a virtual destructor if the interface will be deleted as the instance-type.

* Prefer structs over classes. Since everything most likeley will be public anyway.

### Enums
* Enums should use the capital letter 'E' as prefix

* `enum class` is the default and should be used unless implicit conversion to an integer is required. Concretely, struct-wrap is needed only for:
  * Array indexing or array sizing (`Arr[EFoo::Bar]`, `T Arr[EFoo::Count]`).
  * Arithmetic on enum values (`EFoo::A + N`, `Type - EFoo::A`).
  * Bitwise shifts (`EFoo::A << N`, `EFoo::A >> N`).
  * Bitwise OR/AND on values without `ENUM_CLASS_OPERATORS`.
  * Other APIs that take an integer where `enum class` would require an explicit cast at every call site.

  Cases that look like they need int conversion but actually do *not* (use `enum class`):
  * Template non-type parameters (`template<EFoo F>` works fine for `enum class`).
  * Tag dispatch (a parameter of `enum class` type is just a type tag).
  * Bitfield members (`enum class` as a bitfield is valid C++11+).
  * Equality and ordering comparisons.

* Enum classes with flags can use the 'ENUM_CLASS_OPERATORS' macro to make operations such as 'and', 'or' etc to work.

* Example of the default `enum class` form:
```
enum class EMyEnum : uint8
{
  Car   = 0,
  Apple = 1,
};
ENUM_CLASS_OPERATORS(EMyEnum);
```

* When implicit conversion to an integer is required, the enum must be wrapped in a struct that exposes an inner `enum Type`. This makes the enum a typed name while still allowing the inner enumerators to convert to integers:
```
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
```
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
```
enum class EMyEnum : uint8
{
    /** Described by a D3D12_UNORDERED_ACCESS_VIEW_DESC, created via CreateUnorderedAccessView */
    Standard = 0,

    /** Described by a feedback/paired resource pair, created via CreateSamplerFeedbackUnorderedAccessView */
    SamplerFeedback = 1,
};
```

* Enumerators that are self-explanatory are left undocumented. Runs of undocumented enumerators stay packed together with their values column-aligned:
```
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
* Unions use 'F' as prefix, for example

```
union FMyUnion
{
  int32 x;
  float y;
};
```

### Virtual
* Should be used when runtime polymorphism is needed

* Prefeer functions that can be determined at compile-time due to performance

### Naming Conventions
* Variables normaly no not use prefixes and should use the following style
```
// Correct
int* MyPointer  = nullptr;

// Wrong
int* pMyPointer = nullptr;
```

* This applies to classes and structues as well
```
class FMyClass
{
private:
  int  MyInteger = 0;
  int* Pointer   = nullptr;
 };
```

* Global variables should have 'G' as a prefix:
```
bool GMyGlobal;
```

* The 'G' prefix marks file-scope globals only. A singleton whose instance is a `static` member of its own class is not a global, so it drops the prefix and takes the type name without the `F` (for example `FEngine::Engine`, `FTaskGraph::TaskGraph`, `FShaderCompiler::ShaderCompiler`). Where that would only repeat a qualifier the enclosing class already supplies, keep the shorter stem instead (`FRHICommandListExecutor::CommandListExecutor`).
```
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
```
constexpr uint64 PSO_KEY_BINDLESS_BIT = uint64(1) << 32;
constexpr int32  MAX_SHADOW_CASCADES  = 4;
```
* Prefer `constexpr` over `#define` for value constants so the type is preserved and the symbol is visible to the debugger. Use `#define` only for preprocessor switches, include guards, or token pasting where a value constant cannot do the job.

* Class / struct member constants (`static constexpr` inside a type) and local `constexpr` variables follow the usual PascalCase / member naming rules — only file or namespace scope is UPPER_SNAKE_CASE.
* Mathematical components such as x, y, z etc. should **NOT** be capitalized.
```
struct FMyMathStruct
{
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};
```

* Functions use pascalcase. Same with parameters and local variables.
```
void FunctionsLookLikeThis(int FirstAParameter)
{
  int ALocalVariable = FirstAParameter;
}
```

* Memberfunctions also use pascalcase. Parameters with same name as a member variable should use the 'In' or 'New' prefix:
```
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

### Platform Specific Code
* Platform specific code should be kept in seperate directories with the platform name. `PlatformInterface` holds the interfaces the platforms implement, and `Platform` holds the headers that select between them
```
CoreApplication/PlatformInterface/
CoreApplication/Platform/
CoreApplication/Mac/
CoreApplication/Windows/
```
* Classes specific to platform should be prefixed with platform-name

* The interface is named for the concept, prefixed with `IPlatform`, and follows the rules in [Interfaces](#interfaces): no state, and its functions either pure virtual or static
```
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
```
// PlatformApplication.h

#pragma once
#if PLATFORM_WINDOWS
  #include "CoreApplication/Windows/WindowsApplication.h"
  typedef FWindowsApplication FPlatformApplication;
#elif PLATFORM_MACOS
  #include "CoreApplication/Mac/MacApplication.h"
  typedef FMacApplication FPlatformApplication;
#else
  #error No platform defined
#endif
```

* The fallback branch is an `#error` when the interface cannot be instantiated, which is the case whenever its functions are pure virtual. A compile-time interface can be named there instead, since its stubs are callable, so `PlatformInputMapper.h` and `PlatformApplicationMisc.h` typedef `IPlatformInputMapper` and `IPlatformApplicationMisc` in that branch.
