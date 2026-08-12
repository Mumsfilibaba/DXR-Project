@echo off
REM ----------------------------------------------------------------------------
REM  Compiles the material pass shaders across every combination of the material
REM  layout axes and reports an aggregated pass/fail result. Returns a non-zero
REM  exit code if any permutation failed to compile so it can be used in
REM  automation. Windows counterpart to RunShaderTests.command.
REM
REM  Nothing else in the repo compiles a shader outside a running engine:
REM  RunCompileTests builds C++ only, and FShaderCache compiles on demand through
REM  dxcompiler at runtime, so without this the only check on a shader edit is a
REM  GUI run that happens to request the permutation you broke.
REM
REM  This sweeps a curated subset rather than the whole permutation space. The
REM  full space is a few thousand permutations across the six material passes,
REM  and neither the counts nor the per-pass ShouldCompilePermutation rules can be
REM  restated in a batch file without going stale. What is swept here is the axes
REM  that change how a material's textures are declared and sampled:
REM
REM    ENABLE_PARALLAX_MAPPING   ENABLE_ALPHA_MASK      ENABLE_NORMAL_MAPPING
REM    ENABLE_PARALLAX_CLIPPING  ENABLE_DOUBLE_SIDED    ENABLE_BINDLESS
REM    USE_UNJITTERED_CAMERA     shadow pass kinds
REM
REM  VERTEX_ATTRIBUTES is derived rather than swept, mirroring
REM  CreateDepthOnlyAttributes in CommonShaderPermutations.h, and the invalid
REM  clipping-without-parallax combination is skipped the way
REM  RemapMaterialPermutation folds it away. The authoritative cross-check that
REM  this subset still covers what it should is the set of static_asserts on
REM  FPermutation::PermutationCount in the per-pass shader rules.
REM
REM  Usage:
REM    RunShaderTests.bat [options]
REM
REM  Options:
REM    --no-pause          Never wait for a keypress before closing.
REM    --backend <name>    Sweep one backend: d3d12, vulkan, metal or all.
REM                        Defaults to d3d12 and metal.
REM    --shader <name>     Sweep one shader entry, e.g. "BasePassPS". Repeatable.
REM    --list              List the shader entries and exit without compiling.
REM    --dxc <path>        Use a specific dxc executable.
REM
REM  The window pauses at the end (on success or failure) so results stay
REM  readable when launched interactively. For automation, pass --no-pause or
REM  set TESTS_NO_PAUSE=1 to skip the pause.
REM ----------------------------------------------------------------------------
setlocal EnableDelayedExpansion

REM  This script sits one level below the repo root; resolve it to a full path
REM  so logged paths do not carry a "..\" through every message.
for %%I in ("%~dp0..") do set "ROOT=%%~fI\"
set "LOG=%ROOT%ShaderResults.log"
set "SHADER_DIR=%ROOT%Assets\Shaders"

set "NO_PAUSE=0"
set "LIST_ONLY=0"
set "DXC="
set "BACKENDS="
set "ONLY_SHADERS="
set "RC=0"

REM --- Parse arguments -------------------------------------------------------
:ParseArgs
if "%~1"=="" goto AfterArgs
if /i "%~1"=="--no-pause" (
    set "NO_PAUSE=1"
) else if /i "%~1"=="--list" (
    set "LIST_ONLY=1"
) else if /i "%~1"=="--backend" (
    if "%~2"=="" (
        echo [ERROR] --backend requires a value.
        set "RC=2" & goto Finish
    )
    set "BACKENDS=%~2"
    shift
) else if /i "%~1"=="--dxc" (
    if "%~2"=="" (
        echo [ERROR] --dxc requires a path.
        set "RC=2" & goto Finish
    )
    set "DXC=%~2"
    shift
) else if /i "%~1"=="--shader" (
    if "%~2"=="" (
        echo [ERROR] --shader requires a name.
        set "RC=2" & goto Finish
    )
    set "ONLY_SHADERS=!ONLY_SHADERS! %~2"
    shift
) else (
    echo [ERROR] Unexpected argument: %~1
    set "RC=2" & goto Finish
)
shift
goto ParseArgs

:AfterArgs
if defined TESTS_NO_PAUSE set "NO_PAUSE=1"

REM --- Bundled compiler first, then PATH, mirroring ResolveDxcExecutable -----
if not defined DXC (
    if exist "%ROOT%ThirdParty\DXC\bin\dxc.exe" (
        set "DXC=%ROOT%ThirdParty\DXC\bin\dxc.exe"
    ) else (
        for %%I in (dxc.exe) do set "DXC=%%~$PATH:I"
    )
)

if not defined DXC (
    echo [ERROR] No dxc executable found. Expected ThirdParty\DXC\bin\dxc.exe or dxc.exe on PATH.
    set "RC=1" & goto Finish
)
if not exist "%DXC%" (
    echo [ERROR] dxc executable not found: %DXC%
    set "RC=1" & goto Finish
)

REM --- Metal needs the SPIRV-Cross pass too ----------------------------------
REM  Stopping at SPIR-V would call a shader good that cannot reach a Metal device,
REM  because SPIRV-Cross rejects constructs the emitted MSL version does not have.
REM  The version here must stay in step with ConvertSpirvToMetalShader.
set "SPIRV_CROSS="
for %%I in (spirv-cross.exe) do set "SPIRV_CROSS=%%~$PATH:I"
set "MSL_VERSION=20300"
set "SPIRV_TEMP=%TEMP%\DXRShaderTest.spv"
set "MSL_TEMP=%TEMP%\DXRShaderTest.metal"

if not defined BACKENDS set "BACKENDS=d3d12 metal"
if /i "%BACKENDS%"=="all" set "BACKENDS=d3d12 vulkan metal"

for %%B in (%BACKENDS%) do (
    if /i not "%%B"=="d3d12" if /i not "%%B"=="vulkan" if /i not "%%B"=="metal" (
        echo [ERROR] Unknown backend: %%B ^(expected d3d12, vulkan, metal or all^)
        set "RC=2" & goto Finish
    )
)

REM ---------------------------------------------------------------------------
REM  Shader entries. Each is name|source|entry|stage|axes, where axes names the
REM  permutation dimensions the entry actually responds to. Sources are relative
REM  to Assets\Shaders and match the IMPLEMENT_SHADER_TYPE declarations.
REM
REM  Axis tokens:
REM    material    parallax, clipping, alpha mask and double-sided
REM    translucency ENABLE_TRANSLUCENT and ENABLE_REFRACTION
REM    normalmap   ENABLE_NORMAL_MAPPING
REM    bindless    ENABLE_BINDLESS
REM    unjitter    USE_UNJITTERED_CAMERA
REM    depthattr   derive VERTEX_ATTRIBUTES from parallax and alpha mask
REM    pointkind   POINTLIGHT_PASS_KIND over multi, single and geometry
REM    cascadekind CASCADE_PASS_KIND over multi, single, geometry and view-instanced
REM    gsonly      restrict the pass kind to the geometry-shader variant
REM    raytracing  skip the entry on backends that report no ray tracing support
REM    once        compile a single permutation, with none of the material defines
REM    cubemap     ENABLE_CUBE_MAP, for the BC6H variant that reads a cube face
REM    encodeonly  BC7_ENCODE_ONLY, for the BC7 pass that packs blocks into a texture
REM ---------------------------------------------------------------------------
set "ENTRIES="
call :AddEntry "BasePassVS|BasePass.hlsl|VSMain|vs|material normalmap bindless"
call :AddEntry "BasePassPS|BasePass.hlsl|PSMain|ps|material normalmap bindless"
call :AddEntry "ForwardPassVS|ForwardPass.hlsl|VSMain|vs|material translucency bindless"
call :AddEntry "ForwardPassPS|ForwardPass.hlsl|PSMain|ps|material translucency bindless"
call :AddEntry "PrePassVS|PrePass.hlsl|VSMain|vs|material bindless unjitter depthattr"
call :AddEntry "PrePassPS|PrePass.hlsl|PSMain|ps|material bindless unjitter depthattr"
call :AddEntry "SelectionIDVS|EditorSelectionID.hlsl|VSMain|vs|material unjitter depthattr"
call :AddEntry "SelectionIDPS|EditorSelectionID.hlsl|PSMain|ps|material unjitter depthattr"
call :AddEntry "PointLightShadowVS|Shadows\PointLightShadows.hlsl|Point_VSMain|vs|material bindless depthattr pointkind"
call :AddEntry "PointLightShadowGS|Shadows\PointLightShadows.hlsl|Point_GSMain|gs|material bindless depthattr pointkind gsonly"
call :AddEntry "PointLightShadowPS|Shadows\PointLightShadows.hlsl|Point_PSMain|ps|material bindless depthattr pointkind"
call :AddEntry "CascadeShadowVS|Shadows\CascadedShadows.hlsl|Cascade_VSMain|vs|material bindless depthattr cascadekind"
call :AddEntry "CascadeShadowGS|Shadows\CascadedShadows.hlsl|Cascade_GSMain|gs|material bindless depthattr cascadekind gsonly"
call :AddEntry "CascadeShadowPS|Shadows\CascadedShadows.hlsl|Cascade_PSMain|ps|material bindless depthattr cascadekind"
call :AddEntry "ClosestHit|ClosestHit.hlsl|ClosestHit|lib|bindless raytracing"
call :AddEntry "InlineReflections|InlineReflections.hlsl|Main|cs|alwaysbindless raytracing"
call :AddEntry "BlockCompressBC1|BlockCompression\BlockCompressionBC1.hlsl|Main|cs|once"
call :AddEntry "BlockCompressBC2|BlockCompression\BlockCompressionBC2.hlsl|Main|cs|once"
call :AddEntry "BlockCompressBC3|BlockCompression\BlockCompressionBC3.hlsl|Main|cs|once"
call :AddEntry "BlockCompressBC4|BlockCompression\BlockCompressionBC4.hlsl|Main|cs|once"
call :AddEntry "BlockCompressBC5|BlockCompression\BlockCompressionBC5.hlsl|Main|cs|once"
call :AddEntry "BlockCompressBC6H|BlockCompression\BlockCompressionBC6H.hlsl|Main|cs|once cubemap"
call :AddEntry "BC7TryMode456|BlockCompression\BlockCompressionBC7.hlsl|TryMode456CS|cs|once"
call :AddEntry "BC7TryMode137|BlockCompression\BlockCompressionBC7.hlsl|TryMode137CS|cs|once"
call :AddEntry "BC7TryMode02|BlockCompression\BlockCompressionBC7.hlsl|TryMode02CS|cs|once"
call :AddEntry "BC7EncodeBlock|BlockCompression\BlockCompressionBC7.hlsl|EncodeBlockCS|cs|once encodeonly"
call :AddEntry "CubeMapGen|CubeMapGen.hlsl|Main|cs|once"
call :AddEntry "GenerateMipsTex2D|GenerateMipsTex2D.hlsl|Main|cs|once"
call :AddEntry "GenerateMipsTexCube|GenerateMipsTexCube.hlsl|Main|cs|once"
call :AddEntry "IrradianceGen|IrradianceGen.hlsl|Main|cs|once"
call :AddEntry "SpecularIrradianceGen|SpecularIrradianceGen.hlsl|Main|cs|once"
call :AddEntry "PackMaterialParams|PackMaterialParams.hlsl|Main|cs|once"
call :AddEntry "BakeAlpha|BakeAlpha.hlsl|Main|cs|once"

if "%LIST_ONLY%"=="1" (
    echo Shader entries:
    for %%E in (!ENTRIES!) do (
        for /f "tokens=1-5 delims=|" %%a in ("%%~E") do echo   %%a  %%b  %%c  %%e
    )
    set "RC=0" & goto Finish
)

set /a TOTAL=0
set /a FAILED=0

REM --- Start each run from a clean log ---------------------------------------
del /q "%LOG%" 2>nul

echo ------------------------------------------------------------
echo  Shader permutation sweep
echo ------------------------------------------------------------
echo  Compiler: %DXC%
echo  Backends: %BACKENDS%
echo.

echo Compiler: %DXC%>> "%LOG%"
echo Backends: %BACKENDS%>> "%LOG%"

call :Now START_TIME

for %%B in (%BACKENDS%) do call :RunBackend "%%B"

call :Now END_TIME
set /a ELAPSED=END_TIME-START_TIME
if !ELAPSED! lss 0 set /a ELAPSED+=8640000
set /a MINS=ELAPSED/6000
set /a SECS=ELAPSED/100-MINS*60

echo.
echo ------------------------------------------------------------
echo  Shader summary: !TOTAL! permutations compiled, !FAILED! failed ^(!MINS!m !SECS!s^).
echo  Full output written to: %LOG%
echo ------------------------------------------------------------
echo Shader summary: !TOTAL! compiled, !FAILED! failed.>> "%LOG%"

if !TOTAL! equ 0 (
    echo [ERROR] Nothing was compiled. Check --shader against the entry names.
    set "RC=1" & goto Finish
)

if !FAILED! gtr 0 (
    echo One or more permutations FAILED to compile.
    set "RC=1" & goto Finish
)

echo All permutations compiled successfully.
set "RC=0" & goto Finish

REM --- Pause (when interactive) and exit with the saved result code ---------
:Finish
if "%NO_PAUSE%"=="0" (
    echo.
    pause
)
exit /b %RC%

REM --- Appends one quoted entry to the entry list --------------------------
REM   %1 = name|source|entry|stage|axes
:AddEntry
set "ENTRIES=!ENTRIES! "%~1""
goto :eof

REM --- Sweeps every entry for one backend ----------------------------------
REM   %1 = backend name
:RunBackend
set "BACKEND=%~1"

REM  Mirrors BuildCompileDefines and BuildSpirvCompileArguments in
REM  Runtime\RHI\ShaderCompiler.cpp. MIN16FLOAT_AVAILABLE follows
REM  RHI.ShaderCompiler.MapMin16FloatToFloat, which defaults to true, so the
REM  SPIR-V backends map the min16float family onto float.
set "BACKEND_ARGS=-D SHADER_BACKEND_D3D12=(1) -D SHADER_BACKEND_VULKAN=(2) -D SHADER_BACKEND_METAL=(3)"

set "SPIRV_ARGS=-spirv -fspv-target-env=vulkan1.2 -fspv-reduce-load-size -fvk-use-dx-layout"
set "SPIRV_ARGS=!SPIRV_ARGS! -fvk-bind-resource-heap 0 31 -fvk-bind-sampler-heap 1 31 -fvk-bind-counter-heap 16 31"
set "SPIRV_DEFINES=-D min16float=float -D min16float2=float2 -D min16float3=float3 -D min16float4=float4 -D MIN16FLOAT_AVAILABLE=(0)"

REM  Mirrors RHI::bSupportsBindless and RHI::bSupportsRayTracing. MetalRHI sets neither,
REM  so both keep the false they are given in RHICore.cpp.
set "MSL_CHECK="
if /i "%BACKEND%"=="metal" (
    set "BACKEND_SUPPORTS_BINDLESS=0"
    set "BACKEND_SUPPORTS_RAYTRACING=0"
    if defined SPIRV_CROSS set "MSL_CHECK=1"
) else (
    set "BACKEND_SUPPORTS_BINDLESS=1"
    set "BACKEND_SUPPORTS_RAYTRACING=1"
)

if /i "%BACKEND%"=="d3d12" (
    set "BACKEND_ARGS=!BACKEND_ARGS! -D SHADER_BACKEND=SHADER_BACKEND_D3D12 -D MIN16FLOAT_AVAILABLE=(1)"
) else if /i "%BACKEND%"=="vulkan" (
    set "BACKEND_ARGS=!BACKEND_ARGS! -D SHADER_BACKEND=SHADER_BACKEND_VULKAN !SPIRV_DEFINES! !SPIRV_ARGS!"
) else (
    set "BACKEND_ARGS=!BACKEND_ARGS! -D SHADER_BACKEND=SHADER_BACKEND_METAL !SPIRV_DEFINES! !SPIRV_ARGS!"
)

echo ============================================================
echo  Backend: %BACKEND%
echo ============================================================

if /i "%BACKEND%"=="metal" if not defined SPIRV_CROSS (
    echo [WARNING] spirv-cross not found on PATH, so this stops at SPIR-V and does not
    echo           check the translation to MSL that the engine performs at load time.
)

set /a BACKEND_START_TOTAL=TOTAL
set /a BACKEND_START_FAILED=FAILED

for %%E in (!ENTRIES!) do (
    for /f "tokens=1-5 delims=|" %%a in ("%%~E") do (
        set "SKIP=0"
        if defined ONLY_SHADERS (
            set "SKIP=1"
            for %%S in (!ONLY_SHADERS!) do if /i "%%S"=="%%a" set "SKIP=0"
        )

        if "!SKIP!"=="0" (
            if not exist "%SHADER_DIR%\%%b" (
                echo [ERROR] Shader source not found: %SHADER_DIR%\%%b
                set /a FAILED+=1
            ) else (
                call :SweepEntry "%%a" "%%b" "%%c" "%%d" "%%e"
            )
        )
    )
)

set /a BACKEND_TOTAL=TOTAL-BACKEND_START_TOTAL
set /a BACKEND_FAILED=FAILED-BACKEND_START_FAILED
echo [RESULT] %BACKEND%: !BACKEND_TOTAL! compiled, !BACKEND_FAILED! failed.
goto :eof

REM --- Expands one shader entry over the axes it responds to ---------------
REM   %1 = name, %2 = source, %3 = entry point, %4 = stage, %5 = axes
:SweepEntry
set "NAME=%~1"
set "SOURCE=%~2"
set "ENTRY=%~3"
set "STAGE=%~4"
set "AXES= %~5 "

REM  Bindless needs SM 6.6 for ResourceDescriptorHeap, matching
REM  FBindless::ModifyCompilationEnvironment.
set "BASE_MODEL=6_2"
if /i "%STAGE%"=="lib" set "BASE_MODEL=6_3"

REM  Mirrors ShouldCompilePermutation, which culls anything the device cannot support
REM  before it ever reaches a compiler. Metal reports neither bindless nor ray tracing,
REM  so sweeping those here would report failures the engine never asks for.
if not "!AXES:raytracing=!"=="!AXES!" if "%BACKEND_SUPPORTS_RAYTRACING%"=="0" goto :eof

if not "!AXES:alwaysbindless=!"=="!AXES!" (
    if "%BACKEND_SUPPORTS_BINDLESS%"=="0" goto :eof
    call :CompileOne "%NAME%" "%SOURCE%" "%ENTRY%" "%STAGE%_6_6" "default" ""
    goto :eof
)

REM  The compute entries answer to none of the material axes, so each one compiles once per value
REM  of the single dimension it does name.
if not "!AXES:once=!"=="!AXES!" (
    set "CUBE_VALUES=none"
    if not "!AXES:cubemap=!"=="!AXES!" set "CUBE_VALUES=0 1"

    for %%V in (!CUBE_VALUES!) do (
        set "DEFINES="
        set "DESC=default"

        REM  The cube-map variant reads a TextureCube and writes an array slice, so it is its own compile.
        if not "%%V"=="none" (
            set "DEFINES=-D ENABLE_CUBE_MAP=(%%V)"
            set "DESC=cubemap=%%V"
        )

        REM  BC7 splits its mode search, which hands candidates on through a buffer, from the pass
        REM  that packs blocks into the destination texture.
        if not "!AXES:encodeonly=!"=="!AXES!" (
            set "DEFINES=!DEFINES! -D BC7_ENCODE_ONLY=(1)"
            set "DESC=encodeonly=1"
        )

        call :CompileOne "!NAME!" "!SOURCE!" "!ENTRY!" "!STAGE!_!BASE_MODEL!" "!DESC!" "!DEFINES!"
    )

    goto :eof
)

set "BINDLESS_VALUES=0"
if not "!AXES:bindless=!"=="!AXES!" if "%BACKEND_SUPPORTS_BINDLESS%"=="1" set "BINDLESS_VALUES=0 1"

set "PASS_KIND_DEFINE="
set "PASS_KINDS=none"
if not "!AXES:pointkind=!"=="!AXES!" (
    set "PASS_KIND_DEFINE=POINTLIGHT_PASS_KIND"
    set "PASS_KINDS=1 2 3"
    if not "!AXES:gsonly=!"=="!AXES!" set "PASS_KINDS=3"
) else if not "!AXES:cascadekind=!"=="!AXES!" (
    set "PASS_KIND_DEFINE=CASCADE_PASS_KIND"
    set "PASS_KINDS=1 2 3 4"
    if not "!AXES:gsonly=!"=="!AXES!" set "PASS_KINDS=3"
)

set "UNJITTER_VALUES=0"
if not "!AXES:unjitter=!"=="!AXES!" set "UNJITTER_VALUES=0 1"

set "NORMAL_VALUES=0"
if not "!AXES:normalmap=!"=="!AXES!" set "NORMAL_VALUES=0 1"

set "ALPHA_VALUES=0"
set "SIDED_VALUES=0"
if not "!AXES:material=!"=="!AXES!" (
    set "ALPHA_VALUES=0 1"
    set "SIDED_VALUES=0 1"
)

set "TRANSLUCENT_VALUES=0"
if not "!AXES:translucency=!"=="!AXES!" set "TRANSLUCENT_VALUES=0 1"

for %%P in (0 1) do (
    REM  RemapMaterialPermutation clears clipping when parallax is off, so the
    REM  clipping-without-parallax half of the space never reaches a compiler.
    set "CLIPPING_VALUES=0"
    if "%%P"=="1" set "CLIPPING_VALUES=0 1"

    for %%C in (!CLIPPING_VALUES!) do (
    for %%A in (!ALPHA_VALUES!) do (
    for %%S in (!SIDED_VALUES!) do (
    for %%N in (!NORMAL_VALUES!) do (
    for %%L in (!BINDLESS_VALUES!) do (
    for %%U in (!UNJITTER_VALUES!) do (
    for %%K in (!PASS_KINDS!) do (
    for %%T in (!TRANSLUCENT_VALUES!) do (

        REM  Both of these mirror the way RemapMaterialPermutation clears clipping without parallax.
        REM  Translucency forces the alpha-mask cutout off, because the two read the same opacity,
        REM  and its absence forces refraction off. An empty set skips the combination outright.
        set "REFRACTION_VALUES=0"
        if "%%T"=="1" set "REFRACTION_VALUES=0 1"
        if "%%T"=="1" if "%%A"=="1" set "REFRACTION_VALUES="

        for %%R in (!REFRACTION_VALUES!) do (

        set "MODEL=!BASE_MODEL!"
        if "%%L"=="1" set "MODEL=6_6"

        set "DEFINES=-D ENABLE_PARALLAX_MAPPING=(%%P) -D ENABLE_PARALLAX_CLIPPING=(%%C)"
        set "DEFINES=!DEFINES! -D ENABLE_ALPHA_MASK=(%%A) -D ENABLE_DOUBLE_SIDED=(%%S) -D ENABLE_BINDLESS=(%%L)"
        set "DESC=par=%%P clip=%%C alpha=%%A sided=%%S bindless=%%L"

        if not "!AXES:translucency=!"=="!AXES!" (
            set "DEFINES=!DEFINES! -D ENABLE_TRANSLUCENT=(%%T) -D ENABLE_REFRACTION=(%%R)"
            set "DESC=!DESC! translucent=%%T refract=%%R"
        )

        if not "!AXES:normalmap=!"=="!AXES!" (
            set "DEFINES=!DEFINES! -D ENABLE_NORMAL_MAPPING=(%%N)"
            set "DESC=!DESC! normal=%%N"
        )

        if not "!AXES:unjitter=!"=="!AXES!" (
            set "DEFINES=!DEFINES! -D USE_UNJITTERED_CAMERA=(%%U)"
            set "DESC=!DESC! unjitter=%%U"
        )

        REM  Mirrors CreateDepthOnlyAttributes: Position always, TexCoord0 when
        REM  either parallax or the alpha mask needs UVs, TangentBasis for parallax.
        if not "!AXES:depthattr=!"=="!AXES!" (
            set /a ATTRS=1
            if "%%P"=="1" set /a ATTRS^|=6
            if "%%A"=="1" set /a ATTRS^|=4
            set "DEFINES=!DEFINES! -D VERTEX_ATTRIBUTES=(!ATTRS!)"
            set "DESC=!DESC! attrs=!ATTRS!"
        )

        if defined PASS_KIND_DEFINE (
            set "DEFINES=!DEFINES! -D !PASS_KIND_DEFINE!=(%%K)"
            set "DESC=!DESC! kind=%%K"
        )

        call :CompileOne "!NAME!" "!SOURCE!" "!ENTRY!" "!STAGE!_!MODEL!" "!DESC!" "!DEFINES!"
    )
    )
    )
    )
    )
    )
    )
    )
    )
)
goto :eof

REM --- Compiles one permutation and tallies the result ---------------------
REM   %1 = name, %2 = source, %3 = entry, %4 = profile, %5 = description, %6 = defines
:CompileOne
set /a TOTAL+=1

set "ARGS=-T %~4 -E %~3 -HV 2021 -WX -O3 -all-resources-bound -Gfa -I "%SHADER_DIR%" !BACKEND_ARGS! %~6"

REM  The Metal backend needs the SPIR-V on disk so SPIRV-Cross can take a second pass at it.
if defined MSL_CHECK set "ARGS=!ARGS! -Fo "%SPIRV_TEMP%""

set "STEP_FAILED=0"
"%DXC%" %ARGS% "%SHADER_DIR%\%~2" > "%TEMP%\dxrshadertest.txt" 2>&1
if errorlevel 1 set "STEP_FAILED=1"

REM  FShaderCompiler::ConvertSpirvToMetalShader runs this same translation at
REM  load time, so a shader that stops here never reaches a Metal device.
if "!STEP_FAILED!"=="0" if defined MSL_CHECK (
    "%SPIRV_CROSS%" --msl --msl-version %MSL_VERSION% "%SPIRV_TEMP%" --output "%MSL_TEMP%" > "%TEMP%\dxrshadertest.txt" 2>&1
    if errorlevel 1 set "STEP_FAILED=1"
)

if "!STEP_FAILED!"=="1" (
    set /a FAILED+=1
    echo [FAIL] %BACKEND% %~1 %~5
    echo ----- FAIL: %BACKEND% ^| %~1 ^| %~5 ----->> "%LOG%"
    echo   "%DXC%" %ARGS% "%SHADER_DIR%\%~2">> "%LOG%"
    type "%TEMP%\dxrshadertest.txt" >> "%LOG%"
    echo.>> "%LOG%"
) else (
    echo ----- PASS: %BACKEND% ^| %~1 ^| %~5 ----->> "%LOG%"
)

del /q "%TEMP%\dxrshadertest.txt" 2>nul
goto :eof

REM --- Stores the current time as hundredths of a second since midnight ----
REM   %1 = name of the variable to write
:Now
setlocal
set "T=%TIME: =0%"
set /a "V=(((1%T:~0,2%-100)*60+(1%T:~3,2%-100))*60+(1%T:~6,2%-100))*100+(1%T:~9,2%-100)"
endlocal & set "%~1=%V%"
goto :eof
