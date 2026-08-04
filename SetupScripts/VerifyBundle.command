#!/usr/bin/env bash
# ----------------------------------------------------------------------------
#  Checks that the macOS .app is self-contained, so it can be launched or moved
#  without DYLD_LIBRARY_PATH. Every engine dylib must carry an @rpath install
#  name, reference nothing outside the bundle or the OS, keep a clean runpath,
#  and every dylib the engine dlopen's must actually be inside the bundle.
#
#    VerifyBundle.command [configuration] [options]
#
#  Options:
#    --no-pause       Never wait for a keypress before closing.
#    --suffix <name>  Verify the bundle in Build/bin/<config>-macosx-x64-<name>
#                     instead of the unsuffixed one, matching the --suffix
#                     passed to Compile_Xcode.command.
#
#  Defaults to the "Development Editor" configuration. The check is purely
#  static, so unlike a launch test it also works over SSH.
# ----------------------------------------------------------------------------

# A non-interactive ssh session never runs path_helper, so /usr/local/bin is
# absent and every Homebrew tool is invisible.
export PATH="/usr/local/bin:$PATH"

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"

ROOT=$( cd "${DIR}/.." && pwd )

CONFIG="Development Editor"
SUFFIX=""
NO_PAUSE=0
POSITIONAL=0
EXPECT_VALUE=""

for arg in "$@"; do
    if [ -n "$EXPECT_VALUE" ]; then
        case "$arg" in
            --*)
                echo "[ERROR] $EXPECT_VALUE requires a value, got: $arg"
                exit 2
                ;;
        esac

        SUFFIX="$arg"
        EXPECT_VALUE=""
        continue
    fi

    case "$arg" in
        --no-pause)
            NO_PAUSE=1
            ;;
        --suffix)
            EXPECT_VALUE="--suffix"
            ;;
        *)
            if [ $POSITIONAL -eq 0 ]; then
                CONFIG="$arg"
                POSITIONAL=1
            else
                echo "[ERROR] Unexpected argument: $arg"
                exit 2
            fi
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

BIN="${ROOT}/Build/bin/${CONFIG}-macosx-x64"
if [ -n "$SUFFIX" ]; then
    BIN="${BIN}-${SUFFIX}"
fi

if [ ! -d "$BIN" ]; then
    echo "[SKIP] No build output for configuration '${CONFIG}'."
    pause_if_needed
    exit 0
fi

APP=""
for CANDIDATE in "$BIN"/*.app; do
    if [ -d "$CANDIDATE" ]; then
        APP="$CANDIDATE"
        break
    fi
done

if [ -z "$APP" ]; then
    echo "[SKIP] No .app bundle in ${BIN}."
    pause_if_needed
    exit 0
fi

FRAMEWORKS="${APP}/Contents/Frameworks"
EXECUTABLE="${APP}/Contents/MacOS/$( basename "$APP" .app )"

FAILURES=0

fail() {
    echo "[FAIL] $*"
    FAILURES=$(( FAILURES + 1 ))
}

echo "------------------------------------------------------------"
echo " Verifying $( basename "$APP" ) (${CONFIG})..."
echo "------------------------------------------------------------"

if [ ! -x "$EXECUTABLE" ]; then
    fail "Bundle executable not found: ${EXECUTABLE}"
    echo
    echo " ${FAILURES} problem(s) found."
    pause_if_needed
    exit 1
fi

# Anything else resolves through a path baked in at link time, which breaks once the bundle moves
is_allowed_dependency() {
    case "$1" in
        @rpath/*|/usr/lib/*|/System/*) return 0 ;;
        *)                             return 1 ;;
    esac
}

# The install name is what every dependent records, so an absolute one there defeats the runpath
# no matter how the dependent was linked
check_install_name() {
    local LIBRARY="$1"
    local LEAF
    LEAF=$( basename "$LIBRARY" )

    local INSTALL_NAME
    INSTALL_NAME=$( otool -D "$LIBRARY" | tail -n +2 )

    if [ "$INSTALL_NAME" != "@rpath/${LEAF}" ]; then
        fail "${LEAF} has install name '${INSTALL_NAME}', expected '@rpath/${LEAF}'"
    fi
}

check_dependencies() {
    local BINARY="$1"
    local LEAF
    LEAF=$( basename "$BINARY" )

    local DEPENDENCY
    while read -r DEPENDENCY; do
        if ! is_allowed_dependency "$DEPENDENCY"; then
            fail "${LEAF} depends on '${DEPENDENCY}', which lives outside the bundle"
        fi
    done < <( otool -L "$BINARY" | tail -n +2 | awk '{ print $1 }' )
}

check_runpath() {
    local BINARY="$1"
    local LEAF
    LEAF=$( basename "$BINARY" )

    local RUNPATH
    while read -r RUNPATH; do
        case "$RUNPATH" in
            *\"*)
                fail "${LEAF} has a quote-mangled LC_RPATH: ${RUNPATH}"
                ;;
            /usr/lib*|/System/*)
                ;;
            /*)
                fail "${LEAF} has an absolute LC_RPATH: ${RUNPATH}"
                ;;
        esac
    done < <( otool -l "$BINARY" | grep -A2 LC_RPATH | sed -nE 's/^ *path (.*) \(offset [0-9]+\)$/\1/p' )
}

check_rpath_dependencies_present() {
    local BINARY="$1"
    local LEAF
    LEAF=$( basename "$BINARY" )

    local DEPENDENCY
    while read -r DEPENDENCY; do
        case "$DEPENDENCY" in
            @rpath/*)
                local NEEDED="${DEPENDENCY#@rpath/}"
                if [ "$NEEDED" != "$LEAF" ] && [ ! -f "${FRAMEWORKS}/${NEEDED}" ]; then
                    fail "${LEAF} needs ${NEEDED}, which is missing from Contents/Frameworks"
                fi
                ;;
        esac
    done < <( otool -L "$BINARY" | tail -n +2 | awk '{ print $1 }' )
}

for LIBRARY in "$BIN"/*.dylib; do
    [ -f "$LIBRARY" ] || continue

    check_install_name "$LIBRARY"
    check_dependencies "$LIBRARY"
    check_runpath "$LIBRARY"

    # Runtime modules are dlopen'd rather than linked, so no other check covers them
    LEAF=$( basename "$LIBRARY" )
    if [ ! -f "${FRAMEWORKS}/${LEAF}" ]; then
        fail "${LEAF} is built but missing from Contents/Frameworks"
    fi
done

check_dependencies "$EXECUTABLE"
check_runpath "$EXECUTABLE"
check_rpath_dependencies_present "$EXECUTABLE"

# dlopen'd by ShaderCompiler.cpp, so it appears in no binary's dependency list
if [ ! -f "${FRAMEWORKS}/libdxcompiler.dylib" ]; then
    fail "libdxcompiler.dylib is missing from Contents/Frameworks"
fi

for LIBRARY in "$FRAMEWORKS"/*.dylib; do
    [ -f "$LIBRARY" ] || continue
    check_rpath_dependencies_present "$LIBRARY"
done

echo
echo "--- codesign ---"
codesign --verify --verbose=2 "$APP" 2>&1

echo
echo "------------------------------------------------------------"
if [ $FAILURES -eq 0 ]; then
    echo " Bundle is self-contained."
else
    echo " ${FAILURES} problem(s) found."
fi
echo "------------------------------------------------------------"

pause_if_needed

if [ $FAILURES -eq 0 ]; then
    exit 0
fi

exit 1
