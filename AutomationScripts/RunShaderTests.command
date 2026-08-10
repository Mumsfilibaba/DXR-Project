#!/usr/bin/env bash
# ----------------------------------------------------------------------------
#  Compiles the material pass shaders across every combination of the material
#  layout axes and reports an aggregated pass/fail result. Returns a non-zero
#  exit code if any permutation failed to compile so it can be used in
#  automation. macOS counterpart to RunShaderTests.bat.
#
#  Nothing else in the repo compiles a shader outside a running engine:
#  RunCompileTests builds C++ only, and FShaderCache compiles on demand through
#  libdxcompiler at runtime, so without this the only check on a shader edit is
#  a GUI run that happens to request the permutation you broke.
#
#  This sweeps a curated subset rather than the whole permutation space. The
#  full space is a few thousand permutations across the six material passes,
#  and neither the counts nor the per-pass ShouldCompilePermutation rules can be
#  restated in shell without going stale. What is swept here is the axes that
#  change how a material's textures are declared and sampled:
#
#    ENABLE_PARALLAX_MAPPING   ENABLE_ALPHA_MASK      ENABLE_NORMAL_MAPPING
#    ENABLE_PARALLAX_CLIPPING  ENABLE_DOUBLE_SIDED    ENABLE_BINDLESS
#    USE_UNJITTERED_CAMERA     shadow pass kinds
#
#  VERTEX_ATTRIBUTES is derived rather than swept, mirroring
#  CreateDepthOnlyAttributes in CommonShaderPermutations.h, and the invalid
#  clipping-without-parallax combination is skipped the way
#  RemapMaterialPermutation folds it away. The authoritative cross-check that
#  this subset still covers what it should is the set of static_asserts on
#  FPermutation::PermutationCount in the per-pass shader rules.
#
#  Usage:
#    RunShaderTests.command [options]
#
#  Options:
#    --no-pause          Never wait for a keypress before closing.
#    --backend <name>    Sweep one backend: d3d12, vulkan, metal or all.
#                        Defaults to d3d12 and metal.
#    --shader <name>     Sweep one shader entry, e.g. "BasePassPS". Repeatable.
#    --list              List the shader entries and exit without compiling.
#    --dxc <path>        Use a specific dxc executable.
#
#  The window pauses at the end (on success or failure) so results stay
#  readable when launched interactively. For automation, pass --no-pause or
#  set TESTS_NO_PAUSE=1 to skip the pause.
# ----------------------------------------------------------------------------

export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"

ROOT=$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )
cd "$ROOT"

LOG="${ROOT}/ShaderResults.log"
SHADER_DIR="${ROOT}/Assets/Shaders"

NO_PAUSE=0
LIST_ONLY=0
DXC=""
BACKENDS=""
ONLY_SHADERS=""
EXPECT_VALUE=""

TOTAL=0
FAILED=0
SKIPPED=0

for arg in "$@"; do
    if [ -n "$EXPECT_VALUE" ]; then
        case "$arg" in
            --*)
                echo "[ERROR] $EXPECT_VALUE requires a value, got: $arg"
                exit 2
                ;;
        esac

        case "$EXPECT_VALUE" in
            --backend) BACKENDS="$arg" ;;
            --dxc)     DXC="$arg" ;;
            --shader)  ONLY_SHADERS="${ONLY_SHADERS} ${arg}" ;;
        esac

        EXPECT_VALUE=""
        continue
    fi

    case "$arg" in
        --no-pause)
            NO_PAUSE=1
            ;;
        --list)
            LIST_ONLY=1
            ;;
        --backend|--dxc|--shader)
            EXPECT_VALUE="$arg"
            ;;
        *)
            echo "[ERROR] Unexpected argument: $arg"
            exit 2
            ;;
    esac
done

if [ -n "$EXPECT_VALUE" ]; then
    echo "[ERROR] $EXPECT_VALUE requires a value."
    exit 2
fi

if [ -n "$TESTS_NO_PAUSE" ]; then
    NO_PAUSE=1
fi

pause_if_needed() {
    if [ "$NO_PAUSE" -eq 0 ] && [ -t 0 ]; then
        echo
        echo "Press any key to close..."
        read -n 1 -s
    fi
}

# --- Bundled compiler first, then PATH, mirroring ResolveDxcExecutable ------
if [ -z "$DXC" ]; then
    if [ -x "${ROOT}/ThirdParty/DXC/bin/dxc" ]; then
        DXC="${ROOT}/ThirdParty/DXC/bin/dxc"
    else
        DXC=$( command -v dxc 2>/dev/null )
    fi
fi

if [ -z "$DXC" ] || [ ! -x "$DXC" ]; then
    echo "[ERROR] No dxc executable found."
    echo "        ThirdParty/DXC/bin ships libdxcompiler.dylib on macOS but no CLI, so dxc has"
    echo "        to come from PATH. Install it (for example into /usr/local/bin) or pass --dxc."
    pause_if_needed
    exit 1
fi

# --- Metal needs the SPIRV-Cross pass too ----------------------------------
#  Stopping at SPIR-V would call a shader good that cannot reach a Metal device,
#  because SPIRV-Cross rejects constructs the emitted MSL version does not have.
#  The version here must stay in step with ConvertSpirvToMetalShader.
SPIRV_CROSS=$( command -v spirv-cross 2>/dev/null )
MSL_VERSION="20300"
SPIRV_TEMP="${TMPDIR:-/tmp}/DXRShaderTest.spv"
MSL_TEMP="${TMPDIR:-/tmp}/DXRShaderTest.metal"

case "$BACKENDS" in
    "")               BACKENDS="d3d12 metal" ;;
    all)              BACKENDS="d3d12 vulkan metal" ;;
    d3d12|vulkan|metal) ;;
    *)
        echo "[ERROR] Unknown backend: $BACKENDS (expected d3d12, vulkan, metal or all)"
        pause_if_needed
        exit 2
        ;;
esac

# ---------------------------------------------------------------------------
#  Shader entries. Each is name|source|entry|stage|axes, where axes names the
#  permutation dimensions the entry actually responds to. Sources are relative
#  to Assets/Shaders and match the IMPLEMENT_SHADER_TYPE declarations.
#
#  Axis tokens:
#    material   parallax, clipping, alpha mask and double-sided
#    translucency ENABLE_TRANSLUCENT and ENABLE_REFRACTION
#    normalmap  ENABLE_NORMAL_MAPPING
#    bindless   ENABLE_BINDLESS
#    unjitter   USE_UNJITTERED_CAMERA
#    depthattr  derive VERTEX_ATTRIBUTES from parallax and alpha mask
#    pointkind  POINTLIGHT_PASS_KIND over multi, single and geometry
#    cascadekind CASCADE_PASS_KIND over multi, single, geometry and view-instanced
#    gsonly     restrict the pass kind to the geometry-shader variant
#    raytracing skip the entry on backends that report no ray tracing support
# ---------------------------------------------------------------------------
ENTRIES="\
BasePassVS|BasePass.hlsl|VSMain|vs|material normalmap bindless
BasePassPS|BasePass.hlsl|PSMain|ps|material normalmap bindless
ForwardPassVS|ForwardPass.hlsl|VSMain|vs|material translucency bindless
ForwardPassPS|ForwardPass.hlsl|PSMain|ps|material translucency bindless
PrePassVS|PrePass.hlsl|VSMain|vs|material bindless unjitter depthattr
PrePassPS|PrePass.hlsl|PSMain|ps|material bindless unjitter depthattr
SelectionIDVS|EditorSelectionID.hlsl|VSMain|vs|material unjitter depthattr
SelectionIDPS|EditorSelectionID.hlsl|PSMain|ps|material unjitter depthattr
PointLightShadowVS|Shadows/PointLightShadows.hlsl|Point_VSMain|vs|material bindless depthattr pointkind
PointLightShadowGS|Shadows/PointLightShadows.hlsl|Point_GSMain|gs|material bindless depthattr pointkind gsonly
PointLightShadowPS|Shadows/PointLightShadows.hlsl|Point_PSMain|ps|material bindless depthattr pointkind
CascadeShadowVS|Shadows/CascadedShadows.hlsl|Cascade_VSMain|vs|material bindless depthattr cascadekind
CascadeShadowGS|Shadows/CascadedShadows.hlsl|Cascade_GSMain|gs|material bindless depthattr cascadekind gsonly
CascadeShadowPS|Shadows/CascadedShadows.hlsl|Cascade_PSMain|ps|material bindless depthattr cascadekind
ClosestHit|ClosestHit.hlsl|ClosestHit|lib|bindless raytracing
InlineReflections|InlineReflections.hlsl|Main|cs|alwaysbindless raytracing"

has_axis() {
    case " $2 " in
        *" $1 "*) return 0 ;;
        *)        return 1 ;;
    esac
}

if [ $LIST_ONLY -eq 1 ]; then
    echo "Shader entries:"
    while IFS='|' read -r NAME SOURCE ENTRY STAGE AXES; do
        [ -z "$NAME" ] && continue
        printf '  %-20s %-38s %-16s %s\n' "$NAME" "$SOURCE" "$ENTRY" "$AXES"
    done <<< "$ENTRIES"
    exit 0
fi

# --- Compiles one permutation and tallies the result -----------------------
compile_one() {
    NAME="$1"
    SOURCE="$2"
    ENTRY="$3"
    PROFILE="$4"
    DESC="$5"
    shift 5

    TOTAL=$((TOTAL + 1))

    ARGS=(-T "$PROFILE" -E "$ENTRY" -HV 2021 -WX -O3 -all-resources-bound -Gfa -I "$SHADER_DIR")
    ARGS+=("${BACKEND_ARGS[@]}")

    for DEFINE in "$@"; do
        ARGS+=(-D "$DEFINE")
    done

    # The Metal backend needs the SPIR-V on disk so SPIRV-Cross can take a second pass at it.
    if [ -n "$MSL_CHECK" ]; then
        ARGS+=(-Fo "$SPIRV_TEMP")
    fi

    OUTPUT=$( "$DXC" "${ARGS[@]}" "${SHADER_DIR}/${SOURCE}" 2>&1 )
    RESULT=$?

    if [ $RESULT -eq 0 ] && [ -n "$MSL_CHECK" ]; then
        # FShaderCompiler::ConvertSpirvToMetalShader runs this same translation at
        # load time, so a shader that stops here never reaches a Metal device.
        OUTPUT=$( "$SPIRV_CROSS" --msl --msl-version "$MSL_VERSION" "$SPIRV_TEMP" --output "$MSL_TEMP" 2>&1 )
        RESULT=$?
    fi

    if [ $RESULT -ne 0 ]; then
        FAILED=$((FAILED + 1))
        echo "[FAIL] ${BACKEND} ${NAME} ${DESC}"
        {
            echo "----- FAIL: ${BACKEND} | ${NAME} | ${DESC} -----"
            echo "  ${DXC} ${ARGS[*]} ${SHADER_DIR}/${SOURCE}"
            echo "$OUTPUT"
            echo
        } >> "$LOG"
    else
        echo "----- PASS: ${BACKEND} | ${NAME} | ${DESC} -----" >> "$LOG"
    fi
}

# --- Expands one shader entry over the axes it responds to -----------------
sweep_entry() {
    NAME="$1"
    SOURCE="$2"
    ENTRY="$3"
    STAGE="$4"
    AXES="$5"

    # Bindless needs SM 6.6 for ResourceDescriptorHeap, matching
    # FBindless::ModifyCompilationEnvironment.
    BASE_MODEL="6_2"
    if [ "$STAGE" = "lib" ]; then
        BASE_MODEL="6_3"
    fi

    # Mirrors ShouldCompilePermutation, which culls anything the device cannot support
    # before it ever reaches a compiler. Metal reports neither bindless nor ray tracing,
    # so sweeping those here would report failures the engine never asks for.
    if has_axis raytracing "$AXES" && [ "$BACKEND_SUPPORTS_RAYTRACING" -eq 0 ]; then
        return
    fi

    if has_axis alwaysbindless "$AXES"; then
        if [ "$BACKEND_SUPPORTS_BINDLESS" -eq 0 ]; then
            return
        fi

        compile_one "$NAME" "$SOURCE" "$ENTRY" "${STAGE}_6_6" "default"
        return
    fi

    BINDLESS_VALUES="0"
    if has_axis bindless "$AXES" && [ "$BACKEND_SUPPORTS_BINDLESS" -eq 1 ]; then
        BINDLESS_VALUES="0 1"
    fi

    PASS_KINDS="none"
    PASS_KIND_DEFINE=""
    if has_axis pointkind "$AXES"; then
        PASS_KIND_DEFINE="POINTLIGHT_PASS_KIND"
        PASS_KINDS="1 2 3"
        if has_axis gsonly "$AXES"; then
            PASS_KINDS="3"
        fi
    elif has_axis cascadekind "$AXES"; then
        PASS_KIND_DEFINE="CASCADE_PASS_KIND"
        PASS_KINDS="1 2 3 4"
        if has_axis gsonly "$AXES"; then
            PASS_KINDS="3"
        fi
    fi

    UNJITTER_VALUES="0"
    if has_axis unjitter "$AXES"; then
        UNJITTER_VALUES="0 1"
    fi

    NORMAL_VALUES="0"
    if has_axis normalmap "$AXES"; then
        NORMAL_VALUES="0 1"
    fi

    ALPHA_VALUES="0"
    SIDED_VALUES="0"
    if has_axis material "$AXES"; then
        ALPHA_VALUES="0 1"
        SIDED_VALUES="0 1"
    fi

    TRANSLUCENT_VALUES="0"
    if has_axis translucency "$AXES"; then
        TRANSLUCENT_VALUES="0 1"
    fi

    for PARALLAX in 0 1; do
        # RemapMaterialPermutation clears clipping when parallax is off, so the
        # clipping-without-parallax half of the space never reaches a compiler.
        CLIPPING_VALUES="0"
        if [ "$PARALLAX" -eq 1 ]; then
            CLIPPING_VALUES="0 1"
        fi

        for CLIPPING in $CLIPPING_VALUES; do
        for ALPHA in $ALPHA_VALUES; do
        for SIDED in $SIDED_VALUES; do
        for NORMAL in $NORMAL_VALUES; do
        for BINDLESS in $BINDLESS_VALUES; do
        for UNJITTER in $UNJITTER_VALUES; do
        for PASS_KIND in $PASS_KINDS; do
        for TRANSLUCENT in $TRANSLUCENT_VALUES; do

            # Both of these mirror the way RemapMaterialPermutation clears clipping without parallax.
            # Translucency forces the alpha-mask cutout off, because the two read the same opacity,
            # and its absence forces refraction off.
            if [ "$TRANSLUCENT" -eq 1 ] && [ "$ALPHA" -eq 1 ]; then
                continue
            fi

            REFRACTION_VALUES="0"
            if [ "$TRANSLUCENT" -eq 1 ]; then
                REFRACTION_VALUES="0 1"
            fi

        for REFRACTION in $REFRACTION_VALUES; do

            MODEL="$BASE_MODEL"
            if [ "$BINDLESS" -eq 1 ]; then
                MODEL="6_6"
            fi

            DEFINES=(
                "ENABLE_PARALLAX_MAPPING=($PARALLAX)"
                "ENABLE_PARALLAX_CLIPPING=($CLIPPING)"
                "ENABLE_ALPHA_MASK=($ALPHA)"
                "ENABLE_DOUBLE_SIDED=($SIDED)"
                "ENABLE_BINDLESS=($BINDLESS)"
            )

            DESC="par=$PARALLAX clip=$CLIPPING alpha=$ALPHA sided=$SIDED bindless=$BINDLESS"

            if has_axis translucency "$AXES"; then
                DEFINES+=("ENABLE_TRANSLUCENT=($TRANSLUCENT)")
                DEFINES+=("ENABLE_REFRACTION=($REFRACTION)")
                DESC="$DESC translucent=$TRANSLUCENT refract=$REFRACTION"
            fi

            if has_axis normalmap "$AXES"; then
                DEFINES+=("ENABLE_NORMAL_MAPPING=($NORMAL)")
                DESC="$DESC normal=$NORMAL"
            fi

            if has_axis unjitter "$AXES"; then
                DEFINES+=("USE_UNJITTERED_CAMERA=($UNJITTER)")
                DESC="$DESC unjitter=$UNJITTER"
            fi

            # Mirrors CreateDepthOnlyAttributes: Position always, TexCoord0 when
            # either parallax or the alpha mask needs UVs, TangentBasis for parallax.
            if has_axis depthattr "$AXES"; then
                ATTRS=1
                if [ "$PARALLAX" -eq 1 ] || [ "$ALPHA" -eq 1 ]; then
                    ATTRS=$((ATTRS | 4))
                fi
                if [ "$PARALLAX" -eq 1 ]; then
                    ATTRS=$((ATTRS | 2))
                fi

                DEFINES+=("VERTEX_ATTRIBUTES=($ATTRS)")
                DESC="$DESC attrs=$ATTRS"
            fi

            if [ -n "$PASS_KIND_DEFINE" ]; then
                DEFINES+=("${PASS_KIND_DEFINE}=(${PASS_KIND})")
                DESC="$DESC kind=$PASS_KIND"
            fi

            compile_one "$NAME" "$SOURCE" "$ENTRY" "${STAGE}_${MODEL}" "$DESC" "${DEFINES[@]}"

        done
        done
        done
        done
        done
        done
        done
        done
        done
    done
}

# --- Start each run from a clean log ---------------------------------------
rm -f "$LOG"

DXC_VERSION=$( "$DXC" --version 2>&1 | tr '\n' ' ' )

echo "------------------------------------------------------------"
echo " Shader permutation sweep"
echo "------------------------------------------------------------"
echo " Compiler: $DXC"
echo " Version:  $DXC_VERSION"
echo " MSL:      ${SPIRV_CROSS:-<not found, SPIR-V only>}"
echo " Backends: $BACKENDS"
echo

{
    echo "Compiler: $DXC"
    echo "Version:  $DXC_VERSION"
    echo "MSL:      ${SPIRV_CROSS:-<not found, SPIR-V only>}"
    echo "Backends: $BACKENDS"
    echo
} >> "$LOG"

START=$(date +%s)

for BACKEND in $BACKENDS; do
    # Mirrors BuildCompileDefines and BuildSpirvCompileArguments in
    # Runtime/RHI/ShaderCompiler.cpp. MIN16FLOAT_AVAILABLE follows
    # RHI.ShaderCompiler.MapMin16FloatToFloat, which defaults to true, so the
    # SPIR-V backends map the min16float family onto float.
    BACKEND_ARGS=(
        -D "SHADER_BACKEND_D3D12=(1)"
        -D "SHADER_BACKEND_VULKAN=(2)"
        -D "SHADER_BACKEND_METAL=(3)"
    )

    MSL_CHECK=""
    if [ "$BACKEND" = "metal" ] && [ -n "$SPIRV_CROSS" ]; then
        MSL_CHECK="1"
    fi

    # Mirrors RHI::bSupportsBindless and RHI::bSupportsRayTracing. MetalRHI sets neither,
    # so both keep the false they are given in RHICore.cpp.
    if [ "$BACKEND" = "metal" ]; then
        BACKEND_SUPPORTS_BINDLESS=0
        BACKEND_SUPPORTS_RAYTRACING=0
    else
        BACKEND_SUPPORTS_BINDLESS=1
        BACKEND_SUPPORTS_RAYTRACING=1
    fi

    case "$BACKEND" in
        d3d12)
            BACKEND_ARGS+=(-D "SHADER_BACKEND=SHADER_BACKEND_D3D12" -D "MIN16FLOAT_AVAILABLE=(1)")
            ;;
        vulkan|metal)
            if [ "$BACKEND" = "vulkan" ]; then
                BACKEND_ARGS+=(-D "SHADER_BACKEND=SHADER_BACKEND_VULKAN")
            else
                BACKEND_ARGS+=(-D "SHADER_BACKEND=SHADER_BACKEND_METAL")
            fi

            BACKEND_ARGS+=(
                -D "min16float=float"
                -D "min16float2=float2"
                -D "min16float3=float3"
                -D "min16float4=float4"
                -D "MIN16FLOAT_AVAILABLE=(0)"
                -spirv
                -fspv-target-env=vulkan1.2
                -fspv-reduce-load-size
                -fvk-use-dx-layout
                -fvk-bind-resource-heap 0 31
                -fvk-bind-sampler-heap 1 31
                -fvk-bind-counter-heap 16 31
            )
            ;;
    esac

    echo "============================================================"
    echo " Backend: ${BACKEND}"
    echo "============================================================"

    if [ "$BACKEND" = "metal" ] && [ -z "$SPIRV_CROSS" ]; then
        echo "[WARNING] spirv-cross not found on PATH, so this stops at SPIR-V and does not"
        echo "          check the translation to MSL that the engine performs at load time."
    fi

    BACKEND_START_TOTAL=$TOTAL
    BACKEND_START_FAILED=$FAILED

    while IFS='|' read -r NAME SOURCE ENTRY STAGE AXES; do
        [ -z "$NAME" ] && continue

        if [ -n "$ONLY_SHADERS" ]; then
            case " $ONLY_SHADERS " in
                *" $NAME "*) ;;
                *) SKIPPED=$((SKIPPED + 1)); continue ;;
            esac
        fi

        if [ ! -f "${SHADER_DIR}/${SOURCE}" ]; then
            echo "[ERROR] Shader source not found: ${SHADER_DIR}/${SOURCE}"
            FAILED=$((FAILED + 1))
            continue
        fi

        sweep_entry "$NAME" "$SOURCE" "$ENTRY" "$STAGE" "$AXES"
    done <<< "$ENTRIES"

    BACKEND_TOTAL=$((TOTAL - BACKEND_START_TOTAL))
    BACKEND_FAILED=$((FAILED - BACKEND_START_FAILED))
    echo "[RESULT] ${BACKEND}: ${BACKEND_TOTAL} compiled, ${BACKEND_FAILED} failed."
done

ELAPSED=$(( $(date +%s) - START ))
MINS=$((ELAPSED / 60))
SECS=$((ELAPSED % 60))

echo
echo "------------------------------------------------------------"
echo " Shader summary: $TOTAL permutations compiled, $FAILED failed (${MINS}m ${SECS}s)."
echo " Full output written to: $LOG"
echo "------------------------------------------------------------"
echo "Shader summary: $TOTAL compiled, $FAILED failed." >> "$LOG"

if [ $TOTAL -eq 0 ]; then
    echo "[ERROR] Nothing was compiled. Check --shader against the entry names."
    pause_if_needed
    exit 1
fi

if [ $FAILED -gt 0 ]; then
    echo "One or more permutations FAILED to compile."
    RC=1
else
    echo "All permutations compiled successfully."
    RC=0
fi

pause_if_needed
exit $RC
